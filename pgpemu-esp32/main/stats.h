#ifndef STATS_H
#define STATS_H

#include <stdbool.h>
#include <stdint.h>

void stats_get_runtime();

void increment_caught(uint16_t conn_id);
void increment_fled(uint16_t conn_id);
void increment_spin(uint16_t conn_id);

typedef struct {
    uint16_t caught;
    uint16_t fled;
    uint16_t spin;
    // esp_timer timestamp of the last catch, 0 = never
    int64_t last_caught_us;
} Stats;

typedef struct {
    uint16_t conn_id;
    Stats stats;
} StatsForConn;

// read-only snapshot; returns false (and a zeroed *out) if conn_id has no stats yet
bool get_stats_snapshot(uint16_t conn_id, Stats* out);
// totals across all connections since boot
void get_stats_totals(uint32_t* caught, uint32_t* fled, uint32_t* spin);

#endif /* STATS_H */
