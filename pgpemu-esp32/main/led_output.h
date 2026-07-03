#ifndef LED_OUTPUT_H
#define LED_OUTPUT_H

#include <stdbool.h>

/**
 * Initialize LED output on the Heltec WiFi Kit 32 onboard LED (GPIO 25, active-high).
 * Sets the pin as output and initializes the LED to ON (HIGH).
 * Call this during system initialization.
 */
void init_led_output(void);

/**
 * Set LED advertising state.
 * @param enabled true to turn LED ON (HIGH), false to turn LED OFF (LOW)
 */
void set_led_advertising(bool enabled);

/**
 * Get current LED advertising state.
 * @return true if LED is ON, false if LED is OFF
 */
bool get_led_advertising(void);

#endif /* LED_OUTPUT_H */
