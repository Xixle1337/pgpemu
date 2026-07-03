#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

/* debug features (OLED debug page + power monitor raw telemetry) are
   compile-time gated; set to 0 for release builds */
#define OLED_ENABLE_DEBUG_PAGE 1

void init_oled_display(void);

/* report user activity (button press): resets the display sleep timer and
   wakes the panel; returns true if the display was off and has just woken
   (callers should then swallow the press instead of acting on it) */
bool oled_activity(void);

void oled_set_device_name(const char* name);
void oled_set_connections(int count);
void oled_set_advertising(bool adv);
/* push an event into the global ring (Status + Log pages); attributed to
   the client owning conn_id, dropped if the connection is unknown */
void oled_set_event(uint16_t conn_id, const char* event);
void oled_shutdown(const char* msg);
void oled_next_display(void);
/* show remaining seconds until long-press shutdown as a big digit
   (>0 = show, <=0 = back to the normal page) */
void oled_set_shutdown_countdown(int remaining);
/* overlay with small caption + auto-scaled big text; duration_ms <= 0 keeps
   it until oled_clear_banner() (the shutdown countdown outranks banners) */
void oled_show_banner(const char* caption, const char* big_text, int duration_ms);
void oled_clear_banner(void);

#endif /* OLED_DISPLAY_H */
