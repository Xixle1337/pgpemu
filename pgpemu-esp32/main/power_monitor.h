#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include "oled_display.h"

#include <stdbool.h>
#include <stdint.h>

void power_monitor_init(void);
void power_get_status(int* mv_out, int* pct_out);

#if OLED_ENABLE_DEBUG_PAGE
/* debug telemetry for the OLED debug page: last raw sample, filtered
   value, raw min/max since boot */
void power_get_debug(int* raw_out, int* filt_out, int* min_out, int* max_out);
/* copies the newest raw samples (5s interval) into buf, oldest first;
   returns the number of samples written (<= max_len) */
int power_get_history(int16_t* buf, int max_len);
#endif

#endif /* POWER_MONITOR_H */
