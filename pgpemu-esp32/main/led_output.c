#include "led_output.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "log_tags.h"

// On original ESP32, GPIO 6-11 are reserved for internal SPI flash and
// GPIO 2 is a strapping pin without an LED on this board.
// The Heltec WiFi Kit 32 onboard white LED is on GPIO 25.
static const int GPIO_LED_ADVERTISING = GPIO_NUM_25;
static bool led_state = false;

static const char* LED_OUTPUT_TAG = "led_output";

void init_led_output(void) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << GPIO_LED_ADVERTISING);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);
    gpio_set_level(GPIO_LED_ADVERTISING, 1);
    led_state = true;

    ESP_LOGI(LED_OUTPUT_TAG, "initialized LED on GPIO %d", GPIO_LED_ADVERTISING);
}

void set_led_advertising(bool enabled) {
    gpio_set_level(GPIO_LED_ADVERTISING, enabled ? 1 : 0);
    led_state = enabled;
}

bool get_led_advertising(void) {
    return led_state;
}
