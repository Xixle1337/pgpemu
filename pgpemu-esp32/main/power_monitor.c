#include "power_monitor.h"

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "mutex_helpers.h"

/* Heltec WiFi Kit 32 v1/v2: VBAT via 1:2 voltage divider on GPIO37 (ADC1_CH1). */
#define VBAT_ADC_UNIT ADC_UNIT_1
#define VBAT_ADC_CHAN ADC_CHANNEL_1 /* GPIO37 */
#define VBAT_ADC_ATTEN ADC_ATTEN_DB_12
#define VBAT_DIVIDER 2

static const char* TAG = "power";

static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;
static bool cali_enabled = false;

static SemaphoreHandle_t mutex = NULL;
static int vbat_mv = 0;

#if OLED_ENABLE_DEBUG_PAGE
/* Debug telemetry: raw (unfiltered) samples for the OLED debug page.
   128 entries x 5s interval = ~10.7 min of history. */
#define POWER_HIST_LEN 128
static int raw_last_mv = 0;
static int raw_min_mv = 0;
static int raw_max_mv = 0;
static int16_t hist[POWER_HIST_LEN];
static int hist_len = 0;
static int hist_pos = 0;

/* caller must hold mutex */
static void debug_record(int raw) {
    raw_last_mv = raw;
    if (raw < raw_min_mv)
        raw_min_mv = raw;
    if (raw > raw_max_mv)
        raw_max_mv = raw;
    hist[hist_pos] = (int16_t)raw;
    hist_pos = (hist_pos + 1) % POWER_HIST_LEN;
    if (hist_len < POWER_HIST_LEN)
        hist_len++;
}

static void debug_init(int first_mv) {
    raw_min_mv = raw_max_mv = first_mv;
    debug_record(first_mv);
}
#else
#define debug_record(raw) ((void)0)
#define debug_init(mv) ((void)0)
#endif

/* LiPo voltage → percent (interpolated).
   Calibrated to THIS board's measured scale: the ESP32 browns out at a
   measured ~3550 mV, so that is 0% (the ADC also overreads ~5% near the
   top of its range, so treat these values as display units, not true mV). */
static int voltage_to_percent(int mv) {
    static const struct {
        int mv;
        int pct;
    } map[] = {
        { 4200, 100 },
        { 4100, 90 },
        { 4000, 78 },
        { 3900, 62 },
        { 3800, 45 },
        { 3700, 28 },
        { 3650, 20 },
        { 3600, 12 },
        { 3575, 6 },
        { 3550, 0 },
    };
    const int n = sizeof(map) / sizeof(map[0]);
    if (mv >= map[0].mv)
        return 100;
    for (int i = 0; i < n - 1; i++) {
        if (mv >= map[i + 1].mv) {
            int range_mv = map[i].mv - map[i + 1].mv;
            int range_pct = map[i].pct - map[i + 1].pct;
            return map[i + 1].pct + ((mv - map[i + 1].mv) * range_pct) / range_mv;
        }
    }
    return 0;
}

static int do_read_mv(void) {
    /* 16 samples averaged — reduces ADC noise and BLE-induced voltage spikes */
    int sum = 0;
    for (int i = 0; i < 16; i++) {
        int raw = 0;
        adc_oneshot_read(adc_handle, VBAT_ADC_CHAN, &raw);
        sum += raw;
    }
    int raw_avg = sum / 16;
    int mv_half = 0;
    if (cali_enabled) {
        adc_cali_raw_to_voltage(cali_handle, raw_avg, &mv_half);
    } else {
        mv_half = (raw_avg * 2900) / 4095;
    }
    return mv_half * VBAT_DIVIDER;
}

static void sample_task(void* pv) {
    (void)pv;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        int bat_mv = do_read_mv();
        int filt_mv = 0;
        if (mutex_acquire_blocking(mutex)) {
            /* IIR low-pass: alpha=0.25 — suppresses BLE-load voltage dips */
            vbat_mv = (vbat_mv * 3 + bat_mv) / 4;
            filt_mv = vbat_mv;
            debug_record(bat_mv);
            mutex_release(mutex);
        }
        ESP_LOGD(TAG, "raw=%dmV filt=%dmV pct=%d%%", bat_mv, filt_mv, voltage_to_percent(filt_mv));
    }
}

void power_monitor_init(void) {
    mutex = xSemaphoreCreateMutex();

    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = VBAT_ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = VBAT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, VBAT_ADC_CHAN, &chan_cfg));

    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = VBAT_ADC_UNIT,
        .atten = VBAT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    cali_enabled = (adc_cali_create_scheme_line_fitting(&cali_cfg, &cali_handle) == ESP_OK);
    ESP_LOGI(TAG, "calibration %s", cali_enabled ? "enabled" : "disabled");

    /* initial synchronous read so display shows real values immediately */
    vbat_mv = do_read_mv();
    debug_init(vbat_mv);
    ESP_LOGI(TAG, "vbat=%dmV pct=%d%% ready", vbat_mv, voltage_to_percent(vbat_mv));

    /* 4096: headroom for ESP_LOG formatting */
    xTaskCreate(sample_task, "power_mon", 4096, NULL, 2, NULL);
}

void power_get_status(int* mv_out, int* pct_out) {
    int mv = 0;
    if (mutex_acquire_blocking(mutex)) {
        mv = vbat_mv;
        mutex_release(mutex);
    }
    if (mv_out)
        *mv_out = mv;
    if (pct_out)
        *pct_out = voltage_to_percent(mv);
}

#if OLED_ENABLE_DEBUG_PAGE
void power_get_debug(int* raw_out, int* filt_out, int* min_out, int* max_out) {
    if (mutex_acquire_blocking(mutex)) {
        if (raw_out)
            *raw_out = raw_last_mv;
        if (filt_out)
            *filt_out = vbat_mv;
        if (min_out)
            *min_out = raw_min_mv;
        if (max_out)
            *max_out = raw_max_mv;
        mutex_release(mutex);
    }
}

int power_get_history(int16_t* buf, int max_len) {
    int n = 0;
    if (mutex_acquire_blocking(mutex)) {
        n = (hist_len < max_len) ? hist_len : max_len;
        /* copy the newest n samples, oldest first */
        int start = (hist_pos - n + POWER_HIST_LEN) % POWER_HIST_LEN;
        for (int i = 0; i < n; i++) {
            buf[i] = hist[(start + i) % POWER_HIST_LEN];
        }
        mutex_release(mutex);
    }
    return n;
}
#endif
