#include "button_input.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "log_tags.h"
#include "oled_display.h"
#include "pgp_gap.h"
#include "pgp_handshake_multi.h"
#include "settings.h"

// GPIO 9 is reserved for SPI flash on original ESP32.
// GPIO 0 is the PRG button on the Heltec WiFi Kit 32.
static const int CONFIG_GPIO_INPUT_BUTTON1 = GPIO_NUM_0;

/* gesture windows, measured from button-down (debounce time included) */
#define BUTTON_DEBOUNCE_MS 200
#define BUTTON_POLL_MS 100
#define HOLD_TOGGLE_ADV_MS 2000 /* >=2s: toggle advertising */
#define HOLD_DEAD_ZONE_MS 4000  /* >=4s: shutdown countdown shown, release aborts */
#define HOLD_SHUTDOWN_MS 8000   /* >=8s: deep sleep */

typedef enum {
    BUTTON_EVENT_BUTTON1 = 0,
} button_event_type_t;

typedef struct {
    button_event_type_t type;
    uint32_t gpio_num;
} button_event_t;

static void button_input_task(void* pvParameters);
static QueueHandle_t button_input_queue;

int get_button_gpio() {
    return CONFIG_GPIO_INPUT_BUTTON1;
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t)arg;
    button_event_t event = { 0 };

    if (gpio_num == CONFIG_GPIO_INPUT_BUTTON1) {
        event.type = BUTTON_EVENT_BUTTON1;
    }
    event.gpio_num = gpio_num;

    xQueueSendFromISR(button_input_queue, &event, NULL);
}

void init_button_input() {
    button_input_queue = xQueueCreate(1, sizeof(button_event_t));

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1 << CONFIG_GPIO_INPUT_BUTTON1);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(CONFIG_GPIO_INPUT_BUTTON1, gpio_isr_handler, (void*)CONFIG_GPIO_INPUT_BUTTON1);

    /* 4096: newlib printf via UART VFS needs >2k stack (same as the other logging tasks) */
    xTaskCreate(button_input_task, "button_input", 4096, NULL, 15, NULL);
}

static void do_shutdown(void) {
    ESP_LOGI(BUTTON_INPUT_TAG, "long press -> shutdown");
    pgp_advertise_stop();
    oled_shutdown("Goodbye!");
    /* wait for release: entering deep sleep while GPIO0 is still low would
       satisfy the ext0 wake condition immediately (reboot instead of off),
       and GPIO0 low during the wake reset straps the chip into download mode */
    while (gpio_get_level(CONFIG_GPIO_INPUT_BUTTON1) == 0) {
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
    vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));
    esp_sleep_enable_ext0_wakeup(CONFIG_GPIO_INPUT_BUTTON1, 0);
    esp_deep_sleep_start();
}

static void button_input_task(void* pvParameters) {
    button_event_t button_event;

    ESP_LOGI(BUTTON_INPUT_TAG, "task start");

    while (true) {
        if (xQueueReceive(button_input_queue, &button_event, portMAX_DELAY)) {
            if (button_event.type == BUTTON_EVENT_BUTTON1) {
                ESP_LOGV(BUTTON_INPUT_TAG, "button1 down");

                /* wakes a sleeping display; that press then only wakes */
                bool display_was_off = oled_activity();

                /* debounce */
                vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));

                /* tap released during debounce → short press */
                if (gpio_get_level(CONFIG_GPIO_INPUT_BUTTON1) != 0) {
                    if (!display_was_off) {
                        ESP_LOGD(BUTTON_INPUT_TAG, "button1 tap -> next display");
                        oled_next_display();
                    }
                    continue;
                }

                /* poll until release or shutdown hold time reached; from
                   HOLD_TOGGLE_ADV_MS on, show what releasing would do; from
                   HOLD_DEAD_ZONE_MS on, show the seconds left until shutdown */
                int held_ms = BUTTON_DEBOUNCE_MS;
                int shown_remaining = 0;
                bool hint_shown = false;
                while (held_ms < HOLD_SHUTDOWN_MS && gpio_get_level(CONFIG_GPIO_INPUT_BUTTON1) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
                    held_ms += BUTTON_POLL_MS;
                    if (!hint_shown && held_ms >= HOLD_TOGGLE_ADV_MS) {
                        bool adv_target = !get_setting(&global_settings.advertising_enabled);
                        oled_show_banner("Release to toggle:", adv_target ? "ADV ON" : "ADV OFF", 0);
                        hint_shown = true;
                    }
                    if (held_ms >= HOLD_DEAD_ZONE_MS) {
                        int remaining = (HOLD_SHUTDOWN_MS - held_ms + 999) / 1000;
                        if (remaining != shown_remaining) {
                            shown_remaining = remaining;
                            oled_set_shutdown_countdown(remaining);
                        }
                    }
                }

                if (held_ms >= HOLD_SHUTDOWN_MS) {
                    do_shutdown();
                    /* never reached */
                }

                if (shown_remaining > 0) {
                    /* released during the countdown -> abort, back to normal page */
                    ESP_LOGD(BUTTON_INPUT_TAG, "shutdown aborted");
                    oled_set_shutdown_countdown(0);
                    oled_clear_banner();
                }

                if (held_ms < HOLD_TOGGLE_ADV_MS) {
                    if (!display_was_off) {
                        ESP_LOGD(BUTTON_INPUT_TAG, "button1 short press -> next display");
                        oled_next_display();
                    }
                } else if (held_ms < HOLD_DEAD_ZONE_MS) {
                    ESP_LOGD(BUTTON_INPUT_TAG, "button1 medium press -> toggle advertising");
                    if (!toggle_setting(&global_settings.advertising_enabled)) {
                        ESP_LOGE(BUTTON_INPUT_TAG, "failed to toggle advertising");
                        oled_clear_banner();
                        continue;
                    }
                    bool adv_enabled = get_setting(&global_settings.advertising_enabled);
                    if (adv_enabled) {
                        ESP_LOGI(BUTTON_INPUT_TAG, "button1 -> advertising enabled");
                        pgp_advertise();
                    } else {
                        ESP_LOGI(BUTTON_INPUT_TAG, "button1 -> advertising disabled");
                        pgp_advertise_stop();
                    }
                    /* confirmation splash, then back to the normal page */
                    oled_show_banner("Advertising", adv_enabled ? "ON" : "OFF", 1500);
                }
                /* HOLD_DEAD_ZONE_MS..HOLD_SHUTDOWN_MS: countdown was aborted above */
            }
        }
    }

    vTaskDelete(NULL);
}
