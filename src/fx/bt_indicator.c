/**
 * Copyright (c) 2024 Kuba Birecki
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_rgb_fx_bt_indicator

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/rgb_fx.h>

#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/rgb_fx.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct fx_bt_indicator_config {
    struct zmk_color_hsl *colors_hsl;
    struct zmk_color_rgb *colors_rgb;
    size_t *pixel_map;
    size_t pixel_map_size;
    size_t *bg_map;
    size_t bg_map_size;
    uint8_t blending_mode;
    bool no_blink;
    uint8_t profile_idx;
};

struct bt_status {
    bool is_active;
    uint8_t profile_idx;
    bool is_bonded;
    bool is_connected;
};

/**
 * Maintains the BT connection state for the purpose of the indicator effects.
 */
static struct bt_status bt_indicator_status = {
    .is_active = false,
    .profile_idx = 0,
    .is_bonded = false,
    .is_connected = false,
};

/**
 * Whether the indicator is active. To determine the blinking state.
 * BT profiles are mutually exclusice hence why there's no need for dedicated timers
 * and data isolation.
 */
static bool indicator_on = false;

static void fx_bt_indicator_tick_handler(struct k_timer *timer) {
    indicator_on = !indicator_on;
    zmk_rgb_fx_request_frames(1);
}

K_TIMER_DEFINE(bt_indicator_tick, fx_bt_indicator_tick_handler, NULL);

static int fx_bt_indicator_on_state_changed(const zmk_event_t *event_t) {
    struct zmk_endpoint_instance active_endpoint = zmk_endpoints_selected();

    zmk_rgb_fx_request_frames(1);

    // None of this works becasue as of 12/2024, ZMK keyboards are ALWAYS advertising
    // unless it's explicitly disabled using the disconnect behavior.
    //
    // https://github.com/zmkfirmware/zmk/pull/1638

    if (active_endpoint.transport != ZMK_TRANSPORT_BLE) {
        // BLE is inactive

        LOG_DBG("BLE IS NOT ACTIVE");

        bt_indicator_status.is_active = false;
        bt_indicator_status.profile_idx = 0;
        bt_indicator_status.is_bonded = false;
        bt_indicator_status.is_connected = false;

        indicator_on = false;

        return 0;
    }

    bt_indicator_status.is_active = true;
    bt_indicator_status.profile_idx = active_endpoint.ble.profile_index;
    bt_indicator_status.is_bonded = !zmk_ble_active_profile_is_open();
    bt_indicator_status.is_connected = zmk_ble_active_profile_is_connected();

    indicator_on = true;

    if (bt_indicator_status.is_connected) {
        // Connection established, flash once for 3s
        LOG_DBG("CONNECTED");

        k_timer_start(&bt_indicator_tick, K_MSEC(3000), K_NO_WAIT);
        return 0;
    }

    if (bt_indicator_status.is_bonded) {
        LOG_DBG("CONNECTING");
        // Reconnecting, flash repeatedly at a fast frequency.
        k_timer_start(&bt_indicator_tick, K_MSEC(200), K_MSEC(200));
        return 0;
    }

    LOG_DBG("PAIRING");

    // Pairing, flash every second for one second.
    k_timer_start(&bt_indicator_tick, K_MSEC(1000), K_MSEC(1000));
    return 0;
}

ZMK_LISTENER(fx_bt_indicator, fx_bt_indicator_on_state_changed);
ZMK_SUBSCRIPTION(fx_bt_indicator, zmk_endpoint_changed);
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(fx_bt_indicator, zmk_ble_active_profile_changed);
#endif

static void fx_bt_indicator_render_frame(const struct device *dev, struct rgb_fx_pixel *pixels,
                                         size_t num_pixels) {
    const struct fx_bt_indicator_config *config = dev->config;

    if (!bt_indicator_status.is_active || bt_indicator_status.profile_idx != config->profile_idx) {
        // BLE profile inactive, nothing to do
        return;
    }

    for (size_t i = 0; i < config->bg_map_size; ++i) {
        pixels[config->bg_map[i]].value = config->colors_rgb[3];
    }

    if (!config->no_blink && !indicator_on) {
        return;
    }

    size_t color_idx = 0;

    if (!bt_indicator_status.is_connected) {
        color_idx = bt_indicator_status.is_bonded ? 2 : 1;
    }

    for (size_t i = 0; i < config->pixel_map_size; ++i) {
        pixels[config->pixel_map[i]].value = config->colors_rgb[color_idx];
    }
}

static void fx_bt_indicator_start(const struct device *dev) {
    // Nothing to do.
}

static void fx_bt_indicator_stop(const struct device *dev) {
    // Nothing to do.
}

static int fx_bt_indicator_init(const struct device *dev) {
    const struct fx_bt_indicator_config *config = dev->config;

    // Color order in the config:
    //
    // 0: Main/Connected
    // 1: Pairing
    // 2: Connecting
    // 3: Backdrop

    for (size_t i = 0; i < 4; ++i) {
        zmk_hsl_to_rgb(&config->colors_hsl[i], &config->colors_rgb[i]);
    }

    return 0;
}

static const struct rgb_fx_api fx_bt_indicator_api = {
    .on_start = fx_bt_indicator_start,
    .on_stop = fx_bt_indicator_stop,
    .render_frame = fx_bt_indicator_render_frame,
};

#define FX_BT_INDICATOR_DEVICE(idx)                                                                \
                                                                                                   \
    static size_t fx_bt_indicator_##idx##_pixel_map[] = DT_INST_PROP(idx, pixels);                 \
                                                                                                   \
    static size_t fx_bt_indicator_##idx##_bg_map[] = DT_INST_PROP(idx, backdrop_pixels);           \
                                                                                                   \
    static uint32_t fx_bt_indicator_##idx##_colors_hsl[] = {DT_INST_PROP(idx, color),              \
        DT_INST_PROP_OR(idx, pairing_color, DT_INST_PROP(idx, color)),                             \
        DT_INST_PROP_OR(idx, connecting_color, DT_INST_PROP(idx, color)),                          \
        DT_INST_PROP_OR(idx, backdrop_color, (0))};                                                \
                                                                                                   \
    static struct zmk_color_rgb fx_bt_indicator_##idx##_colors_rgb[4];                             \
                                                                                                   \
    static struct fx_bt_indicator_config fx_bt_indicator_##idx##_config = {                        \
        .colors_hsl = (struct zmk_color_hsl *)fx_bt_indicator_##idx##_colors_hsl,                  \
        .colors_rgb = fx_bt_indicator_##idx##_colors_rgb,                                          \
        .pixel_map = fx_bt_indicator_##idx##_pixel_map,                                            \
        .pixel_map_size = DT_INST_PROP_LEN(idx, pixels),                                           \
        .bg_map = fx_bt_indicator_##idx##_bg_map,                                                  \
        .bg_map_size = DT_INST_PROP_LEN(idx, backdrop_pixels),                                     \
        .blending_mode = DT_INST_ENUM_IDX(idx, blending_mode),                                     \
        .no_blink = DT_INST_PROP(idx, no_blink),                                                   \
        .profile_idx = DT_INST_PROP(idx, profile_index),                                           \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(idx, &fx_bt_indicator_init, NULL, NULL, &fx_bt_indicator_##idx##_config, \
                          POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, &fx_bt_indicator_api);

DT_INST_FOREACH_STATUS_OKAY(FX_BT_INDICATOR_DEVICE);
