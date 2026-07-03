#include "stats.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "log_tags.h"
#include "nvs.h"

#include <stdint.h>

#define MAX_CONNECTIONS CONFIG_BT_ACL_CONNECTIONS

static StatsForConn stats[MAX_CONNECTIONS];
static size_t stats_len = 0;

StatsForConn* get_conn_entry(uint16_t conn_id) {
    for (size_t i = 0; i < stats_len; i++) {
        if (stats[i].conn_id == conn_id) {
            return &stats[i];
        }
    }
    if (stats_len < MAX_CONNECTIONS) {
        stats[stats_len].conn_id = conn_id;
        stats[stats_len].stats = (Stats){ 0 };
        stats_len++;

        ESP_LOGI(STATS_TAG, "new entry for conn_id %d added, current len %d", conn_id, stats_len);

        return &stats[stats_len - 1];
    }

    ESP_LOGE(STATS_TAG, "no stat found for conn_id %d or impossible to store a new entry", conn_id);

    return NULL;
}

void delete_conn_entry(uint16_t conn_id) {
    for (size_t i = 0; i < stats_len; i++) {
        if (stats[i].conn_id == conn_id) {
            stats[i] = stats[stats_len - 1];
            stats_len--;
        }
    }
}

void increment_caught(uint16_t conn_id) {
    StatsForConn* s = get_conn_entry(conn_id);
    if (s) {
        s->stats.caught++;
        s->stats.last_caught_us = esp_timer_get_time();
    }
}

bool get_stats_snapshot(uint16_t conn_id, Stats* out) {
    *out = (Stats){ 0 };
    for (size_t i = 0; i < stats_len; i++) {
        if (stats[i].conn_id == conn_id) {
            *out = stats[i].stats;
            return true;
        }
    }
    return false;
}

void get_stats_totals(uint32_t* caught, uint32_t* fled, uint32_t* spin) {
    uint32_t c = 0, f = 0, s = 0;
    for (size_t i = 0; i < stats_len; i++) {
        c += stats[i].stats.caught;
        f += stats[i].stats.fled;
        s += stats[i].stats.spin;
    }
    if (caught)
        *caught = c;
    if (fled)
        *fled = f;
    if (spin)
        *spin = s;
}

void increment_fled(uint16_t conn_id) {
    StatsForConn* s = get_conn_entry(conn_id);
    if (s) {
        s->stats.fled++;
    }
}

void increment_spin(uint16_t conn_id) {
    StatsForConn* s = get_conn_entry(conn_id);
    if (s) {
        s->stats.spin++;
    }
}

void stats_get_runtime() {
    if (stats_len == 0) {
        ESP_LOGI(STATS_TAG, "no stats found");

        return;
    }

    for (size_t i = 0; i < stats_len; i++) {
        ESP_LOGI(STATS_TAG,
            "---STATS---\n"
            "Connection %d:\n"
            "- Caught: %d\n"
            "- Fled: %d\n"
            "- Spin: %d",
            stats[i].conn_id,
            stats[i].stats.caught,
            stats[i].stats.fled,
            stats[i].stats.spin);
    }
}
