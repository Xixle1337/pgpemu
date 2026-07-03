#include "oled_display.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "mutex_helpers.h"
#include "pgp_handshake_multi.h"
#include "power_monitor.h"
#include "stats.h"

#include <stdio.h>
#include <string.h>

#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define OLED_I2C_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

/* refresh task tick; setters never draw, they only mark state dirty and the
   refresh task renders + flushes, so BLE callbacks are never blocked on I2C */
#define OLED_TICK_MS 250
#define OLED_ROTATE_TICKS (3000 / OLED_TICK_MS)   /* client rotation */
#define OLED_PERIODIC_TICKS (1000 / OLED_TICK_MS) /* power/debug pages */
#define OLED_AGO_TICKS (60000 / OLED_TICK_MS)     /* "Last C: Xm ago" row */

/* display sleep: after OLED_SLEEP_TIMEOUT_S without a button press a big
   5..1 countdown is shown for OLED_SLEEP_COUNTDOWN_S, then the panel turns
   off (~10 uA vs ~5-8 mA); any button press wakes it (see oled_activity) */
#define OLED_SLEEP_TIMEOUT_S (20 * 60)
#define OLED_SLEEP_COUNTDOWN_S 5

static const char* TAG = "oled";

/* ---------- 5x7 font (ASCII 0x20..0x7E) ---------- */
static const uint8_t font5x7[][5] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x5F, 0x00, 0x00 },
    { 0x00, 0x07, 0x00, 0x07, 0x00 },
    { 0x14, 0x7F, 0x14, 0x7F, 0x14 },
    { 0x24, 0x2A, 0x7F, 0x2A, 0x12 },
    { 0x23, 0x13, 0x08, 0x64, 0x62 },
    { 0x36, 0x49, 0x55, 0x22, 0x50 },
    { 0x00, 0x05, 0x03, 0x00, 0x00 },
    { 0x00, 0x1C, 0x22, 0x41, 0x00 },
    { 0x00, 0x41, 0x22, 0x1C, 0x00 },
    { 0x08, 0x2A, 0x1C, 0x2A, 0x08 },
    { 0x08, 0x08, 0x3E, 0x08, 0x08 },
    { 0x00, 0x50, 0x30, 0x00, 0x00 },
    { 0x08, 0x08, 0x08, 0x08, 0x08 },
    { 0x00, 0x30, 0x30, 0x00, 0x00 },
    { 0x20, 0x10, 0x08, 0x04, 0x02 },
    { 0x3E, 0x51, 0x49, 0x45, 0x3E },
    { 0x00, 0x42, 0x7F, 0x40, 0x00 },
    { 0x42, 0x61, 0x51, 0x49, 0x46 },
    { 0x21, 0x41, 0x45, 0x4B, 0x31 },
    { 0x18, 0x14, 0x12, 0x7F, 0x10 },
    { 0x27, 0x45, 0x45, 0x45, 0x39 },
    { 0x3C, 0x4A, 0x49, 0x49, 0x30 },
    { 0x01, 0x71, 0x09, 0x05, 0x03 },
    { 0x36, 0x49, 0x49, 0x49, 0x36 },
    { 0x06, 0x49, 0x49, 0x29, 0x1E },
    { 0x00, 0x36, 0x36, 0x00, 0x00 },
    { 0x00, 0x56, 0x36, 0x00, 0x00 },
    { 0x00, 0x08, 0x14, 0x22, 0x41 },
    { 0x14, 0x14, 0x14, 0x14, 0x14 },
    { 0x41, 0x22, 0x14, 0x08, 0x00 },
    { 0x02, 0x01, 0x51, 0x09, 0x06 },
    { 0x30, 0x49, 0x79, 0x41, 0x3E },
    { 0x7E, 0x11, 0x11, 0x11, 0x7E },
    { 0x7F, 0x49, 0x49, 0x49, 0x36 },
    { 0x3E, 0x41, 0x41, 0x41, 0x22 },
    { 0x7F, 0x41, 0x41, 0x22, 0x1C },
    { 0x7F, 0x49, 0x49, 0x49, 0x41 },
    { 0x7F, 0x09, 0x09, 0x09, 0x01 },
    { 0x3E, 0x41, 0x49, 0x49, 0x7A },
    { 0x7F, 0x08, 0x08, 0x08, 0x7F },
    { 0x00, 0x41, 0x7F, 0x41, 0x00 },
    { 0x20, 0x40, 0x41, 0x3F, 0x01 },
    { 0x7F, 0x08, 0x14, 0x22, 0x41 },
    { 0x7F, 0x40, 0x40, 0x40, 0x40 },
    { 0x7F, 0x02, 0x04, 0x02, 0x7F },
    { 0x7F, 0x04, 0x08, 0x10, 0x7F },
    { 0x3E, 0x41, 0x41, 0x41, 0x3E },
    { 0x7F, 0x09, 0x09, 0x09, 0x06 },
    { 0x3E, 0x41, 0x51, 0x21, 0x5E },
    { 0x7F, 0x09, 0x19, 0x29, 0x46 },
    { 0x46, 0x49, 0x49, 0x49, 0x31 },
    { 0x01, 0x01, 0x7F, 0x01, 0x01 },
    { 0x3F, 0x40, 0x40, 0x40, 0x3F },
    { 0x1F, 0x20, 0x40, 0x20, 0x1F },
    { 0x3F, 0x40, 0x38, 0x40, 0x3F },
    { 0x63, 0x14, 0x08, 0x14, 0x63 },
    { 0x07, 0x08, 0x70, 0x08, 0x07 },
    { 0x61, 0x51, 0x49, 0x45, 0x43 },
    { 0x00, 0x7F, 0x41, 0x41, 0x00 },
    { 0x02, 0x04, 0x08, 0x10, 0x20 },
    { 0x00, 0x41, 0x41, 0x7F, 0x00 },
    { 0x04, 0x02, 0x01, 0x02, 0x04 },
    { 0x40, 0x40, 0x40, 0x40, 0x40 },
    { 0x00, 0x01, 0x02, 0x04, 0x00 },
    { 0x20, 0x54, 0x54, 0x54, 0x78 },
    { 0x7F, 0x48, 0x44, 0x44, 0x38 },
    { 0x38, 0x44, 0x44, 0x44, 0x20 },
    { 0x38, 0x44, 0x44, 0x48, 0x7F },
    { 0x38, 0x54, 0x54, 0x54, 0x18 },
    { 0x08, 0x7E, 0x09, 0x01, 0x02 },
    { 0x08, 0x14, 0x54, 0x54, 0x3C },
    { 0x7F, 0x08, 0x04, 0x04, 0x78 },
    { 0x00, 0x44, 0x7D, 0x40, 0x00 },
    { 0x20, 0x40, 0x44, 0x3D, 0x00 },
    { 0x7F, 0x10, 0x28, 0x44, 0x00 },
    { 0x00, 0x41, 0x7F, 0x40, 0x00 },
    { 0x7C, 0x04, 0x18, 0x04, 0x78 },
    { 0x7C, 0x08, 0x04, 0x04, 0x78 },
    { 0x38, 0x44, 0x44, 0x44, 0x38 },
    { 0x7C, 0x14, 0x14, 0x14, 0x08 },
    { 0x08, 0x14, 0x14, 0x18, 0x7C },
    { 0x7C, 0x08, 0x04, 0x04, 0x08 },
    { 0x48, 0x54, 0x54, 0x54, 0x20 },
    { 0x04, 0x3F, 0x44, 0x40, 0x20 },
    { 0x3C, 0x40, 0x40, 0x20, 0x7C },
    { 0x1C, 0x20, 0x40, 0x20, 0x1C },
    { 0x3C, 0x40, 0x30, 0x40, 0x3C },
    { 0x44, 0x28, 0x10, 0x28, 0x44 },
    { 0x0C, 0x50, 0x50, 0x50, 0x3C },
    { 0x44, 0x64, 0x54, 0x4C, 0x44 },
    { 0x00, 0x08, 0x36, 0x41, 0x00 },
    { 0x00, 0x00, 0x7F, 0x00, 0x00 },
    { 0x00, 0x41, 0x36, 0x08, 0x00 },
    { 0x08, 0x04, 0x08, 0x10, 0x08 },
};

typedef enum {
    OLED_MODE_STATUS = 0, /* glanceable: big state, battery, newest event */
    OLED_MODE_STATS,
    OLED_MODE_LOG,
    OLED_MODE_POWER,
#if OLED_ENABLE_DEBUG_PAGE
    OLED_MODE_DEBUG,
#endif
    OLED_MODE_COUNT,
} oled_mode_t;

#if OLED_ENABLE_DEBUG_PAGE
#define MODE_NEEDS_PERIODIC_REDRAW(m) ((m) == OLED_MODE_POWER || (m) == OLED_MODE_DEBUG)
#else
#define MODE_NEEDS_PERIODIC_REDRAW(m) ((m) == OLED_MODE_POWER)
#endif

/* ---------- framebuffer + panel handle ---------- */
/* fb and panel are only touched by init and the refresh task (and by
   oled_shutdown after the refresh task has been parked) */
static uint8_t fb[OLED_WIDTH * OLED_HEIGHT / 8];
static esp_lcd_panel_handle_t panel = NULL;

/* ---------- display state, guarded by state_mutex ---------- */
static SemaphoreHandle_t state_mutex = NULL;
static oled_mode_t display_mode = OLED_MODE_STATUS;
static char dev_name[22] = "pgpemu";
static int connections = 0;
static bool advertising = false;
static int client_slot = 0; /* rotation counter over occupied slots */

/* global event ring (newest last), feeds the Status and Log pages */
#define EVENT_LOG_LEN 6
typedef struct {
    char text[18];
    int slot; /* client slot, for the 1-4 prefix on the Log page */
    int64_t at_us;
} event_entry_t;
static event_entry_t events[EVENT_LOG_LEN];
static int event_count = 0;
static bool dirty = true;
static bool shutting_down = false;
static bool display_off = false;
static int64_t last_activity_us = 0;
static int shutdown_countdown = 0; /* >0: seconds shown until long-press shutdown */

/* banner overlay: small caption + auto-scaled big text (e.g. "ADV OFF") */
static bool banner_active = false;
static char banner_caption[22];
static char banner_big[12];
static int64_t banner_expires_us = 0; /* 0 = sticky until cleared */

/* ---------- drawing helpers ---------- */
static void fb_clear(void) {
    memset(fb, 0, sizeof(fb));
}

static void fb_pixel(int x, int y) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT)
        return;
    fb[x + (y / 8) * OLED_WIDTH] |= (1 << (y % 8));
}

static void fb_char(int x, int y, char c) {
    if (c < 0x20 || c > 0x7E)
        return;
    const uint8_t* g = font5x7[(uint8_t)c - 0x20];
    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 8; row++) {
            if (g[col] & (1 << row))
                fb_pixel(x + col, y + row);
        }
    }
}

static void fb_str(int x, int y, const char* s) {
    while (*s) {
        fb_char(x, y, *s++);
        x += 6;
    }
}

/* scaled glyph: each font pixel becomes a scale x scale block */
static void fb_char_big(int x, int y, char c, int scale) {
    if (c < 0x20 || c > 0x7E)
        return;
    const uint8_t* g = font5x7[(uint8_t)c - 0x20];
    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 8; row++) {
            if (g[col] & (1 << row)) {
                for (int dx = 0; dx < scale; dx++)
                    for (int dy = 0; dy < scale; dy++)
                        fb_pixel(x + col * scale + dx, y + row * scale + dy);
            }
        }
    }
}

static void fb_hline(int y) {
    for (int x = 0; x < OLED_WIDTH; x++)
        fb_pixel(x, y);
}

static void flush(void) {
    if (panel)
        esp_lcd_panel_draw_bitmap(panel, 0, 0, OLED_WIDTH, OLED_HEIGHT, fb);
}

/* centered big string at the largest scale that fits (capped at max_scale) */
static void fb_str_big_centered(int y, const char* s, int max_scale) {
    int len = (int)strlen(s);
    if (len == 0)
        return;
    int scale = OLED_WIDTH / (6 * len);
    if (scale > max_scale)
        scale = max_scale;
    if (scale < 1)
        scale = 1;
    int x = (OLED_WIDTH - (6 * len - 1) * scale) / 2;
    for (int i = 0; i < len; i++) {
        fb_char_big(x + i * 6 * scale, y, s[i], scale);
    }
}

/* compact "now" / "5m" / "3h" / "2d" age string */
static void fmt_ago(int64_t at_us, char* buf, size_t len) {
    int64_t sec = (esp_timer_get_time() - at_us) / 1000000LL;
    if (sec < 60) {
        snprintf(buf, len, "now");
    } else if (sec < 3600) {
        snprintf(buf, len, "%dm", (int)(sec / 60));
    } else if (sec < 86400) {
        snprintf(buf, len, "%dh", (int)(sec / 3600));
    } else {
        snprintf(buf, len, "%dd", (int)(sec / 86400));
    }
}

/* ---------- full render from current state (no flush) ---------- */

/* page 1: device name + battery, big connection state, newest event */
static void redraw_status(void) {
    fb_clear();

    fb_str(0, 0, dev_name);
    int mv = 0, pct = 0;
    power_get_status(&mv, &pct);
    char bat_buf[8];
    snprintf(bat_buf, sizeof(bat_buf), "%d%%", pct);
    fb_str(OLED_WIDTH - (int)strlen(bat_buf) * 6, 0, bat_buf);
    fb_hline(9);

    /* wording chosen to fit 9 chars -> scale 2 */
    const char* state;
    if (connections > 0) {
        state = "CONNECTED";
    } else if (advertising) {
        state = "SEARCHING";
    } else {
        state = "IDLE";
    }
    fb_str_big_centered(20, state, 2);

    /* newest event across all clients + age */
    if (event_count > 0) {
        const event_entry_t* ev = &events[(event_count - 1) % EVENT_LOG_LEN];
        fb_str(0, 48, ev->text);
        char ago[8];
        fmt_ago(ev->at_us, ago, sizeof(ago));
        fb_str(OLED_WIDTH - (int)strlen(ago) * 6, 48, ago);
    }
}

/* page 3: event history, newest first, with client prefix */
static void redraw_log(void) {
    fb_clear();
    fb_str(0, 0, "Log");
    fb_hline(9);

    int n = (event_count < EVENT_LOG_LEN) ? event_count : EVENT_LOG_LEN;
    if (n > 5)
        n = 5;
    for (int i = 0; i < n; i++) {
        const event_entry_t* ev = &events[(event_count - 1 - i) % EVENT_LOG_LEN];
        int y = 12 + i * 10;
        char row[24];
        snprintf(row, sizeof(row), "%c:%.14s", (char)('1' + ev->slot), ev->text);
        fb_str(0, y, row);
        char ago[8];
        fmt_ago(ev->at_us, ago, sizeof(ago));
        fb_str(OLED_WIDTH - (int)strlen(ago) * 6, y, ago);
    }
}

static void redraw_stats(void) {
    fb_clear();

    /* collect occupied client slots; before BT init connections == 0, so the
       (then still uninitialized) slot table is never touched */
    uint16_t conn_ids[CONFIG_BT_ACL_CONNECTIONS];
    int n_occ = 0;
    if (connections > 0) {
        for (int i = 0; i < get_max_connections() && n_occ < CONFIG_BT_ACL_CONNECTIONS; i++) {
            client_state_t* entry = get_client_state_entry_by_idx(i);
            if (entry) {
                conn_ids[n_occ] = entry->conn_id;
                n_occ++;
            }
        }
    }

    /* Row 0: title left, [k/n] right */
    fb_str(0, 0, "Stats");
    Stats st = { 0 };
    if (n_occ > 0) {
        int k = client_slot % n_occ;
        get_stats_snapshot(conn_ids[k], &st);
        char slot_buf[7] = { '[', (char)('1' + k), '/', (char)('0' + n_occ), ']', '\0' };
        fb_str(OLED_WIDTH - 5 * 6, 0, slot_buf);
    } else {
        fb_str(OLED_WIDTH - 3 * 6, 0, "[0]");
    }
    fb_hline(9);

    /* Row 1: per-client stats */
    char stats_buf[32];
    snprintf(stats_buf, sizeof(stats_buf), "C:%u F:%u S:%u", st.caught, st.fled, st.spin);
    fb_str(0, 11, stats_buf);
    fb_hline(20);

    /* Row 2: per-client last catch */
    char ago_buf[32];
    if (st.last_caught_us == 0) {
        snprintf(ago_buf, sizeof(ago_buf), "Last C: --");
    } else {
        int64_t sec = (esp_timer_get_time() - st.last_caught_us) / 1000000LL;
        int min = (int)(sec / 60);
        int hr = min / 60;
        if (sec < 60) {
            snprintf(ago_buf, sizeof(ago_buf), "Last C: <1m ago");
        } else if (hr == 0) {
            snprintf(ago_buf, sizeof(ago_buf), "Last C: %dm ago", min);
        } else {
            snprintf(ago_buf, sizeof(ago_buf), "Last C: %dh%02dm", hr, min % 60);
        }
    }
    fb_str(0, 22, ago_buf);
    fb_hline(31);

    /* Row 3: totals across all clients */
    uint32_t total_c = 0, total_f = 0, total_s = 0;
    get_stats_totals(&total_c, &total_f, &total_s);
    char tot_buf[32];
    snprintf(tot_buf,
        sizeof(tot_buf),
        "All: C:%lu F:%lu S:%lu",
        (unsigned long)total_c,
        (unsigned long)total_f,
        (unsigned long)total_s);
    fb_str(0, 36, tot_buf);
}

static void fb_bar(int x, int y, int w, int h, int pct) {
    for (int i = x; i < x + w; i++) {
        fb_pixel(i, y);
        fb_pixel(i, y + h - 1);
    }
    for (int i = y; i < y + h; i++) {
        fb_pixel(x, i);
        fb_pixel(x + w - 1, i);
    }
    int fill = ((w - 2) * pct) / 100;
    for (int px = x + 1; px < x + 1 + fill; px++)
        for (int py = y + 1; py < y + h - 1; py++)
            fb_pixel(px, py);
}

static void redraw_power(void) {
    fb_clear();

    fb_str(0, 0, "Power");
    fb_hline(9);

    int mv = 0, pct = 0;
    power_get_status(&mv, &pct);

    char volt_buf[16];
    snprintf(volt_buf, sizeof(volt_buf), "%d.%02dV", mv / 1000, (mv % 1000) / 10);
    fb_str(0, 11, volt_buf);
    fb_hline(20);

    char pct_buf[8];
    snprintf(pct_buf, sizeof(pct_buf), "%d%%", pct);
    fb_str(0, 22, pct_buf);

    fb_bar(0, 33, OLED_WIDTH, 10, pct);
}

#if OLED_ENABLE_DEBUG_PAGE
static void redraw_debug(void) {
    fb_clear();

    int64_t up_min = esp_timer_get_time() / 60000000LL;
    char buf[24];
    if (up_min < 60) {
        snprintf(buf, sizeof(buf), "Debug  up %dm", (int)up_min);
    } else {
        snprintf(buf, sizeof(buf), "Debug  up %dh%02dm", (int)(up_min / 60), (int)(up_min % 60));
    }
    fb_str(0, 0, buf);
    fb_hline(9);

    int raw = 0, filt = 0, min = 0, max = 0;
    power_get_debug(&raw, &filt, &min, &max);

    snprintf(buf, sizeof(buf), "raw %4d  flt %4d", raw, filt);
    fb_str(0, 11, buf);
    snprintf(buf, sizeof(buf), "min %4d  max %4d", min, max);
    fb_str(0, 22, buf);
    fb_hline(31);

    /* sparkline of raw samples (newest right), auto-scaled; static buffer
       is safe — only the refresh task renders */
    static int16_t hist_buf[OLED_WIDTH];
    int n = power_get_history(hist_buf, OLED_WIDTH);
    if (n < 2) {
        return;
    }
    int lo = hist_buf[0], hi = hist_buf[0];
    for (int i = 1; i < n; i++) {
        if (hist_buf[i] < lo)
            lo = hist_buf[i];
        if (hist_buf[i] > hi)
            hi = hist_buf[i];
    }
    if (hi - lo < 40) { /* min. 40mV span so noise doesn't fill the graph */
        int mid = (hi + lo) / 2;
        lo = mid - 20;
        hi = mid + 20;
    }
    int prev_y = -1;
    for (int i = 0; i < n; i++) {
        int x = OLED_WIDTH - n + i;
        int y = (OLED_HEIGHT - 1) - ((hist_buf[i] - lo) * 29) / (hi - lo);
        if (prev_y >= 0) {
            int a = (y < prev_y) ? y : prev_y;
            int b = (y < prev_y) ? prev_y : y;
            for (int yy = a; yy <= b; yy++)
                fb_pixel(x, yy);
        } else {
            fb_pixel(x, y);
        }
        prev_y = y;
    }
}
#endif

/* big centered single digit with a small caption on top */
static void redraw_big_countdown(const char* caption, int remaining) {
    fb_clear();
    fb_str(0, 0, caption);
    /* digit is 5*6=30 wide, 7*6=42 high -> centered */
    fb_char_big((OLED_WIDTH - 30) / 2, 16, (char)('0' + remaining), 6);
}

/* caption on top, big text centered below at the largest scale that fits */
static void redraw_banner(void) {
    fb_clear();
    fb_str(0, 0, banner_caption);

    int len = (int)strlen(banner_big);
    if (len == 0)
        return;
    int scale = OLED_WIDTH / (6 * len);
    if (scale > 6)
        scale = 6;
    if (scale < 1)
        scale = 1;
    int width = (6 * len - 1) * scale;
    int y = 12 + (46 - 7 * scale) / 2;
    int x = (OLED_WIDTH - width) / 2;
    for (int i = 0; i < len; i++) {
        fb_char_big(x + i * 6 * scale, y, banner_big[i], scale);
    }
}

static void redraw(void) {
    if (shutdown_countdown > 0) {
        redraw_big_countdown("Shutdown in", shutdown_countdown);
        return;
    }
    if (banner_active) {
        redraw_banner();
        return;
    }
    switch (display_mode) {
    case OLED_MODE_STATS:
        redraw_stats();
        break;
    case OLED_MODE_LOG:
        redraw_log();
        break;
    case OLED_MODE_POWER:
        redraw_power();
        break;
#if OLED_ENABLE_DEBUG_PAGE
    case OLED_MODE_DEBUG:
        redraw_debug();
        break;
#endif
    case OLED_MODE_STATUS: /* fall through */
    default:
        redraw_status();
        break;
    }
}

/* ---------- refresh task ---------- */

static void oled_refresh_task(void* pv) {
    (void)pv;
    int ticks = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(OLED_TICK_MS));

        bool flush_needed = false;
        bool panel_off = false;
        if (mutex_acquire_blocking(state_mutex)) {
            if (!shutting_down && !display_off) {
                ticks++;
                int idle_s = (int)((esp_timer_get_time() - last_activity_us) / 1000000LL);
                if (idle_s >= OLED_SLEEP_TIMEOUT_S + OLED_SLEEP_COUNTDOWN_S) {
                    display_off = true;
                    panel_off = true;
                    dirty = true; /* full redraw on wake */
                } else if (idle_s >= OLED_SLEEP_TIMEOUT_S && shutdown_countdown == 0) {
                    int remaining = OLED_SLEEP_TIMEOUT_S + OLED_SLEEP_COUNTDOWN_S - idle_s;
                    redraw_big_countdown("Display off in", remaining);
                    flush_needed = true;
                } else {
                    if (banner_active && banner_expires_us != 0 && esp_timer_get_time() > banner_expires_us) {
                        banner_active = false;
                        dirty = true;
                    }
                    if (connections > 1 && ticks % OLED_ROTATE_TICKS == 0) {
                        client_slot++;
                        dirty = true;
                    }
                    if (MODE_NEEDS_PERIODIC_REDRAW(display_mode) && ticks % OLED_PERIODIC_TICKS == 0) {
                        dirty = true;
                    }
                    if (ticks % OLED_AGO_TICKS == 0) {
                        /* minute tick: age strings and battery percent */
                        dirty = true;
                    }
                    if (dirty) {
                        redraw();
                        dirty = false;
                        flush_needed = true;
                    }
                }
            }
            mutex_release(state_mutex);
        }
        /* I2C outside the mutex so setters never wait on the bus */
        if (panel_off && panel) {
            esp_lcd_panel_disp_on_off(panel, false);
            ESP_LOGI(TAG, "display sleeping");
        }
        if (flush_needed) {
            flush();
        }
    }
}

/* ---------- public API ---------- */

void init_oled_display(void) {
    state_mutex = xSemaphoreCreateMutex();

    gpio_config_t rst_cfg = {
        .pin_bit_mask = (1ULL << OLED_RST),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&rst_cfg);
    gpio_set_level(OLED_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(OLED_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = OLED_SDA,
        .scl_io_num = OLED_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_i2c_config_t io_cfg = {
        .dev_addr = OLED_I2C_ADDR,
        .scl_speed_hz = 400000,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus, &io_cfg, &io));

    esp_lcd_panel_dev_config_t panel_cfg = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io, &panel_cfg, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    ESP_LOGI(TAG, "display ready");
    last_activity_us = esp_timer_get_time();
    /* refresh task not started yet, safe to render + flush directly */
    redraw();
    flush();
    xTaskCreate(oled_refresh_task, "oled_refresh", 4096, NULL, 3, NULL);
}

void oled_show_banner(const char* caption, const char* big_text, int duration_ms) {
    if (mutex_acquire_blocking(state_mutex)) {
        snprintf(banner_caption, sizeof(banner_caption), "%s", caption);
        snprintf(banner_big, sizeof(banner_big), "%s", big_text);
        banner_active = true;
        banner_expires_us = (duration_ms > 0) ? esp_timer_get_time() + (int64_t)duration_ms * 1000 : 0;
        dirty = true;
        mutex_release(state_mutex);
    }
}

void oled_clear_banner(void) {
    if (mutex_acquire_blocking(state_mutex)) {
        if (banner_active) {
            banner_active = false;
            dirty = true;
        }
        mutex_release(state_mutex);
    }
}

void oled_set_shutdown_countdown(int remaining) {
    if (mutex_acquire_blocking(state_mutex)) {
        shutdown_countdown = (remaining > 0) ? remaining : 0;
        dirty = true;
        mutex_release(state_mutex);
    }
}

bool oled_activity(void) {
    bool was_off = false;
    if (mutex_acquire_blocking(state_mutex)) {
        last_activity_us = esp_timer_get_time();
        if (display_off) {
            display_off = false;
            was_off = true;
        }
        dirty = true;
        mutex_release(state_mutex);
    }
    if (was_off && panel) {
        /* refresh task is idle while display_off, safe to touch the panel here */
        esp_lcd_panel_disp_on_off(panel, true);
        ESP_LOGI(TAG, "display wake");
    }
    return was_off;
}

void oled_set_device_name(const char* name) {
    if (mutex_acquire_blocking(state_mutex)) {
        snprintf(dev_name, sizeof(dev_name), "%s", name);
        dirty = true;
        mutex_release(state_mutex);
    }
}

void oled_set_connections(int count) {
    if (mutex_acquire_blocking(state_mutex)) {
        connections = count;
        if (count == 0) {
            client_slot = 0;
        }
        dirty = true;
        mutex_release(state_mutex);
    }
}

void oled_set_advertising(bool adv) {
    if (mutex_acquire_blocking(state_mutex)) {
        advertising = adv;
        dirty = true;
        mutex_release(state_mutex);
    }
}

void oled_set_event(uint16_t conn_id, const char* event) {
    int slot = get_client_slot(conn_id);
    if (slot < 0) {
        /* unknown connection: drop instead of misattributing the event */
        return;
    }
    if (mutex_acquire_blocking(state_mutex)) {
        event_entry_t* ev = &events[event_count % EVENT_LOG_LEN];
        snprintf(ev->text, sizeof(ev->text), "%s", event);
        ev->slot = slot;
        ev->at_us = esp_timer_get_time();
        event_count++;
        dirty = true;
        mutex_release(state_mutex);
    }
}

void oled_shutdown(const char* msg) {
    if (mutex_acquire_blocking(state_mutex)) {
        shutting_down = true;
        mutex_release(state_mutex);
    }
    /* let an in-flight refresh flush finish before touching the framebuffer */
    vTaskDelay(pdMS_TO_TICKS(2 * OLED_TICK_MS));

    fb_clear();
    int msglen = (int)strlen(msg);
    fb_str((OLED_WIDTH - msglen * 6) / 2, 20, msg);
    fb_str(10, 34, "Shutting down...");
    flush();
    vTaskDelay(pdMS_TO_TICKS(2000));
    if (panel)
        esp_lcd_panel_disp_on_off(panel, false);
}

void oled_next_display(void) {
    if (mutex_acquire_blocking(state_mutex)) {
        display_mode = (oled_mode_t)((display_mode + 1) % OLED_MODE_COUNT);
        dirty = true;
        mutex_release(state_mutex);
    }
}
