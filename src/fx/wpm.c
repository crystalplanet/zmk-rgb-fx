/*
 * Copyright (c) 2024 Kuba Birecki
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_rgb_fx_wpm

#define WPM_CALC_BUFFER_LENGTH 15
#define WPM_CALC_INTERVAL 300
#define WPM_CALC_SIGMA 15
#define WPM_CALC_STROKES_PER_WORD 5

#include <stdlib.h>
#include <math.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/rgb_fx.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct fx_wpm_config {
    size_t *pixel_map;
    size_t pixel_map_size;
    struct zmk_color_hsl *colors;
    bool is_multi_color;
    uint8_t bounds_min;
    uint8_t bounds_max;
    bool is_horizontal;
    uint8_t max_wpm;
    uint8_t blending_mode;
    uint8_t edge_width;
};

/**
 * Holds the number of keystrokes between subsequent WPM re-calculations.
 * At 300ms intervals, a 15 element buffer covers the last 4.5s.
 */
static uint8_t keystrokes[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/**
 * Index of the currently active buffer cell.
 */
static uint8_t keystrokes_index = 0;

/**
 * The second-latest WPM measurement value.
 */
static uint8_t last_wpm = 0;

/**
 * The latest WPM measurement value.
 */
static uint8_t current_wpm = 0;

/**
 * The timestamp for the last WPM measurement.
 */
static uint64_t current_wpm_timestamp = 0;

static void fx_wpm_calc_value(struct k_work *work);

K_WORK_DEFINE(fx_wpm_work, fx_wpm_calc_value);

static void fx_wpm_tick_handler(struct k_timer *timer) {
    k_work_submit(&fx_wpm_work);
}

K_TIMER_DEFINE(fx_wpm_tick, fx_wpm_tick_handler, NULL);

/**
 * Calculates a real-time WPM value.
 */
static void fx_wpm_calc_value(struct k_work *work) {
    uint8_t total_keystrokes = 0;

    float averages = 0.0;
    float weights = 0.0;

    for (size_t i = 0; i < WPM_CALC_BUFFER_LENGTH; ++i) {
        float distance = keystrokes_index - i + (i > keystrokes_index ? WPM_CALC_BUFFER_LENGTH : 0);
        float weight = exp(-((distance * distance) / (2 * WPM_CALC_SIGMA)));

        weights += weight;
        averages += weight * keystrokes[i];

        total_keystrokes += keystrokes[i];
    }

    last_wpm = current_wpm;
    current_wpm =
        ((averages / weights) * ((1000 * 60) / WPM_CALC_INTERVAL)) / WPM_CALC_STROKES_PER_WORD;

    current_wpm_timestamp = k_uptime_get();

    if (current_wpm > 0) {
        zmk_rgb_fx_request_frames(1);
    }

    if (++keystrokes_index == WPM_CALC_BUFFER_LENGTH) {
        keystrokes_index = 0;
    }

    keystrokes[keystrokes_index] = 0;

    if (total_keystrokes == 0) {
        k_timer_stop(&fx_wpm_tick);
    }
}

static int fx_wpm_on_key_press(const zmk_event_t *event) {
    const struct zmk_position_state_changed *pos_event;

    if ((pos_event = as_zmk_position_state_changed(event)) == NULL) {
        // Event not supported.
        return -ENOTSUP;
    }

    if (!pos_event->state) {
        // Don't track key releases.
        return 0;
    }

    keystrokes[keystrokes_index] += 1;

    if (k_timer_remaining_get(&fx_wpm_tick) == 0) {
        k_timer_start(&fx_wpm_tick, K_MSEC(WPM_CALC_INTERVAL), K_MSEC(WPM_CALC_INTERVAL));
    }

    return 0;
}

ZMK_LISTENER(fx_wpm, fx_wpm_on_key_press);
ZMK_SUBSCRIPTION(fx_wpm, zmk_position_state_changed);

static struct zmk_color_rgb fx_wpm_get_frame_color(const struct device *dev, float step) {
    const struct fx_wpm_config *config = dev->config;

    struct zmk_color_rgb rgb;
    struct zmk_color_hsl hsl;

    if (!config->is_multi_color) {
        zmk_hsl_to_rgb(&config->colors[0], &rgb);
        return rgb;
    }

    zmk_interpolate_hsl(&config->colors[0], &config->colors[1], &hsl, step);
    zmk_hsl_to_rgb(&hsl, &rgb);

    return rgb;
}

static void fx_wpm_render_frame(const struct device *dev, struct rgb_fx_pixel *pixels,
                                size_t num_pixels) {
    const struct fx_wpm_config *config = dev->config;

    const size_t *pixel_map = config->pixel_map;

    float timestamp_delta =
        ((float)(k_uptime_get() - current_wpm_timestamp)) / ((float)WPM_CALC_INTERVAL);

    if (timestamp_delta > 1) {
        timestamp_delta = 1.0f;
    }

    const float wpm_delta = (float)last_wpm + (((float)current_wpm - (float)last_wpm) * timestamp_delta);
    const float step = wpm_delta < config->max_wpm ? wpm_delta / ((float)config->max_wpm) : 1.0f;

    const struct zmk_color_rgb color = fx_wpm_get_frame_color(dev, step);

    const int direction = config->bounds_max > config->bounds_min ? 1 : -1;
    const int gradient_edge =
        config->bounds_min + (abs(config->bounds_max - config->bounds_min) + config->edge_width) *
        step * direction;

    for (size_t i = 0; i < config->pixel_map_size; ++i) {
        const int position = config->is_horizontal ? pixels[pixel_map[i]].position_x
                                                       : pixels[pixel_map[i]].position_y;
        struct zmk_color_rgb gradient_color;

        if (direction * (position - gradient_edge) >= 0) {
            gradient_color = (struct zmk_color_rgb){0.0f, 0.0f, 0.0f};

            pixels[pixel_map[i]].value =
                zmk_apply_blending_mode(pixels[pixel_map[i]].value, gradient_color, config->blending_mode);

            continue;
        }

        if (direction * (gradient_edge - position) >= config->edge_width) {
            pixels[pixel_map[i]].value =
                zmk_apply_blending_mode(pixels[pixel_map[i]].value, color, config->blending_mode);

            continue;
        }

        float gradient_step = abs(position - gradient_edge) / (float)config->edge_width;
        gradient_color.r = color.r * gradient_step;
        gradient_color.g = color.g * gradient_step;
        gradient_color.b = color.b * gradient_step;

        pixels[pixel_map[i]].value =
            zmk_apply_blending_mode(pixels[pixel_map[i]].value, gradient_color, config->blending_mode);
    }

    if (last_wpm == 0 && current_wpm == 0) {
        return;
    }

    zmk_rgb_fx_request_frames(1);
}

static void fx_wpm_start(const struct device *dev) { zmk_rgb_fx_request_frames(1); }

static void fx_wpm_stop(const struct device *dev) {
    // Nothing to do.
}

static int fx_wpm_init(const struct device *dev) {
    // Nothing to do.
    return 0;
};

static const struct rgb_fx_api fx_wpm_api = {
    .on_start = fx_wpm_start,
    .on_stop = fx_wpm_stop,
    .render_frame = fx_wpm_render_frame,
};

#define FX_WPM_DEVICE(idx)                                                                         \
                                                                                                   \
    static size_t fx_wpm_##idx##_pixel_map[] = DT_INST_PROP(idx, pixels);                          \
                                                                                                   \
    static uint32_t fx_wpm_##idx##_colors[] = DT_INST_PROP(idx, colors);                           \
                                                                                                   \
    static struct fx_wpm_config fx_wpm_##idx##_config = {                                          \
        .pixel_map = &fx_wpm_##idx##_pixel_map[0],                                                 \
        .pixel_map_size = DT_INST_PROP_LEN(idx, pixels),                                           \
        .colors = (struct zmk_color_hsl *)fx_wpm_##idx##_colors,                                   \
        .is_multi_color = DT_INST_PROP_LEN(idx, colors) > 1,                                       \
        .bounds_min = DT_INST_PROP_BY_IDX(idx, bounds, 0),                                         \
        .bounds_max = DT_INST_PROP_BY_IDX(idx, bounds, 1),                                         \
        .is_horizontal = 0 == (DT_INST_ENUM_IDX(idx, bounds_axis)),                                \
        .max_wpm = DT_INST_PROP(idx, max_wpm),                                                     \
        .blending_mode = DT_INST_ENUM_IDX(idx, blending_mode),                                     \
        .edge_width = DT_INST_PROP(idx, edge_gradient_width),                                      \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(idx, &fx_wpm_init, NULL, NULL, &fx_wpm_##idx##_config,                   \
                          POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, &fx_wpm_api);

DT_INST_FOREACH_STATUS_OKAY(FX_WPM_DEVICE);
