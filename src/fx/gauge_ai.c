/*
 * Copyright (c) 2024 Kuba Birecki
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <math.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/rgb_fx.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct fx_gauge_config {
    size_t *pixel_map;
    size_t pixel_map_size;
    struct zmk_color_hsl *colors_hsl;
    struct zmk_color_rgb *colors_rgb;
    size_t colors_size;

    int value_min;
    int value_max;

    uint8_t bounds_min_from;
    uint8_t bounds_min_to;
    uint8_t bounds_max_from;
    uint8_t bounds_max_to;

    uint8_t blending_mode;
    uint8_t gradient_min_width;
    uint8_t gradient_max_width;
    bool rgb_interpolation;
};

static struct zmk_color_rgb fx_gauge_frame_color(const struct fx_gauge_config *config, float step) {
    size_t i = 0;
    while (i < config->colors_size - 1 && step > (float)(i + 1) / config->colors_size) {
        i++;
    }

    if (config->rgb_interpolation) {
        struct zmk_color_rgb interpolated;
        float interp_step = (step - (float)i / config->colors_size) * config->colors_size;

        zmk_interpolate_rgb(&config->colors_rgb[i], &config->colors_rgb[i + 1], &interpolated, interp_step);
        return interpolated;
    } else {
        struct zmk_color_hsl interpolated_hsl;
        struct zmk_color_rgb rgb;
        float interp_step = (step - (float)i / config->colors_size) * config->colors_size;

        zmk_interpolate_hsl(&config->colors_hsl[i], &config->colors_hsl[i + 1], &interpolated_hsl, interp_step);
        zmk_hsl_to_rgb(&interpolated_hsl, &rgb);
        return rgb;
    }
}

static void fx_gauge_render_frame(const struct device *dev, struct rgb_fx_pixel *pixels,
                                  size_t num_pixels) {
    const struct fx_gauge_config *config = dev->config;

    const size_t *pixel_map = config->pixel_map;
    const int value = zmk_rgb_fx_gauge_value(); // Fetch the value dynamically

    float step = (float)(value - config->value_min) / (float)(config->value_max - config->value_min);
    if (step < 0.0f) step = 0.0f;
    if (step > 1.0f) step = 1.0f;

    const struct zmk_color_rgb base_color = fx_gauge_frame_color(config, step);

    int bounds_min = config->bounds_min_from + step * (config->bounds_min_to - config->bounds_min_from);
    int bounds_max = config->bounds_max_from + step * (config->bounds_max_to - config->bounds_max_from);
    int direction = bounds_max > bounds_min ? 1 : -1;

    for (size_t i = 0; i < config->pixel_map_size; ++i) {
        const int position = pixels[pixel_map[i]].position_x; // Assuming x-axis; adapt for y if needed
        struct zmk_color_rgb pixel_color;

        if (direction * (position - bounds_min) < 0 || direction * (position - bounds_max) > 0) {
            pixel_color = (struct zmk_color_rgb){0.0f, 0.0f, 0.0f}; // Outside bounds, no color
        } else {
            float gradient_step = (float)abs(position - bounds_min) / (float)config->gradient_min_width;
            if (gradient_step > 1.0f) gradient_step = 1.0f;

            pixel_color.r = base_color.r * gradient_step;
            pixel_color.g = base_color.g * gradient_step;
            pixel_color.b = base_color.b * gradient_step;
        }

        pixels[pixel_map[i]].value =
            zmk_apply_blending_mode(pixels[pixel_map[i]].value, pixel_color, config->blending_mode);
    }

    zmk_rgb_fx_request_frames(1);
}

static void fx_gauge_start(const struct device *dev) { zmk_rgb_fx_request_frames(1); }

static void fx_gauge_stop(const struct device *dev) {
    // Stop animation logic, if necessary
}

static int fx_gauge_init(const struct device *dev) {
    // Initialization logic, if necessary
    return 0;
}

static const struct rgb_fx_api fx_gauge_api = {
    .on_start = fx_gauge_start,
    .on_stop = fx_gauge_stop,
    .render_frame = fx_gauge_render_frame,
};

#define FX_GAUGE_DEVICE(idx)                                                                       \
                                                                                                   \
    static size_t fx_gauge_##idx##_pixel_map[] = DT_INST_PROP(idx, pixels);                        \
                                                                                                   \
    static uint32_t fx_gauge_##idx##_colors_hsl[] = DT_INST_PROP(idx, colors_hsl);                 \
                                                                                                   \
    static uint32_t fx_gauge_##idx##_colors_rgb[] = DT_INST_PROP(idx, colors_rgb);                 \
                                                                                                   \
    static struct fx_gauge_config fx_gauge_##idx##_config = {                                      \
        .pixel_map = fx_gauge_##idx##_pixel_map,                                               \
        .pixel_map_size = DT_INST_PROP_LEN(idx, pixels),                                       \
        .colors_hsl = (struct zmk_color_hsl *)fx_gauge_##idx##_colors_hsl,                    \
        .colors_rgb = (struct zmk_color_rgb *)fx_gauge_##idx##_colors_rgb,                    \
        .colors_size = DT_INST_PROP_LEN(idx, colors_hsl),                                      \
        .value_min = DT_INST_PROP(idx, value_min),                                            \
        .value_max = DT_INST_PROP(idx, value_max),                                            \
        .bounds_min_from = DT_INST_PROP_BY_IDX(idx, bounds_min, 0),                           \
        .bounds_min_to = DT_INST_PROP_BY_IDX(idx, bounds_min, 1),                             \
        .bounds_max_from = DT_INST_PROP_BY_IDX(idx, bounds_max, 0),                           \
        .bounds_max_to = DT_INST_PROP_BY_IDX(idx, bounds_max, 1),                             \
        .blending_mode = DT_INST_ENUM_IDX(idx, blending_mode),                                \
        .gradient_min_width = DT_INST_PROP(idx, gradient_min_width),                          \
        .gradient_max_width = DT_INST_PROP(idx, gradient_max_width),                          \
        .rgb_interpolation = DT_INST_PROP(idx, rgb_interpolation),                            \
    };                                                                                        \
    DEVICE_DT_INST_DEFINE(idx, &fx_gauge_init, NULL, NULL, &fx_gauge_##idx##_config,               \
                          POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, &fx_gauge_api);

DT_INST_FOREACH_STATUS_OKAY(FX_GAUGE_DEVICE);
