/*
 * Copyright (c) 2024 Kuba Birecki
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_rgb_fx_linear_gradient

#include <math.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/rgb_fx.h>

#include <zmk/rgb_fx.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct fx_linear_gradient_config {
    struct zmk_color_hsl *colors_hsl;
    struct zmk_color_rgb *colors_rgb;
    size_t *pixel_map;
    size_t pixel_map_size;
    uint8_t blending_mode;
    uint8_t num_colors;
    uint8_t angle;
    bool use_hsl;
    uint16_t gradient_width;
    float offset_per_frame;
};

struct fx_linear_gradient_data {
    float offset;
};

static void fx_linear_gradient_render_frame(const struct device *dev, struct rgb_fx_pixel *pixels,
                                            size_t num_pixels) {
    const struct fx_linear_gradient_config *config = dev->config;
    struct fx_linear_gradient_data *data = dev->data;

    size_t *pixel_map = config->pixel_map;

    uint16_t color_width = config->gradient_width / config->num_colors;

    for (size_t i = 0; i < config->pixel_map_size; ++i) {
        // Relying on properties of 2D graph rotation to calculate the distance for each pixel along the gradient axis
        // https://en.wikipedia.org/wiki/Rotation_of_axes_in_two_dimensions
        float x = pixels[pixel_map[i]].position_x * cos(config->angle) + pixels[pixel_map[i]].position_y * sin(config->angle);
        float distance = config->gradient_width + x - data->offset;

        while (distance >= config->gradient_width) {
            distance -= config->gradient_width;
        }

        uint8_t from = distance / color_width;
        uint8_t to = (from + 1) % config->num_colors;

        float step = (float)(distance - (from * color_width)) / (float)color_width;

        struct zmk_color_hsl color_hsl;
        struct zmk_color_rgb color_rgb;

        if (config->use_hsl) {
            zmk_interpolate_hsl(&config->colors_hsl[from], &config->colors_hsl[to], &color_hsl, step);
            zmk_hsl_to_rgb(&color_hsl, &color_rgb);
        } else {
            zmk_interpolate_rgb(&config->colors_rgb[from], &config->colors_rgb[to], &color_rgb, step);
        }

        pixels[pixel_map[i]].value = zmk_apply_blending_mode(pixels[pixel_map[i]].value, color_rgb, config->blending_mode);
    }

    if (config->offset_per_frame == 0) {
        return;
    }

    data->offset += config->offset_per_frame;

    if (data->offset >= config->gradient_width) {
        data->offset -= config->gradient_width;
    }

    zmk_rgb_fx_request_frames(1);
}

static void fx_linear_gradient_start(const struct device *dev) {
    zmk_rgb_fx_request_frames(1);
}

static void fx_linear_gradient_stop(const struct device *dev) {
    // Nothing to do.
}

static int fx_linear_gradient_init(const struct device *dev) {
    const struct fx_linear_gradient_config *config = dev->config;

    for (size_t i = 0; i < config->num_colors; ++i) {
        zmk_hsl_to_rgb(&config->colors_hsl[i], &config->colors_rgb[i]);
    }

    return 0;
};

static const struct rgb_fx_api fx_linear_gradient_api = {
    .on_start = fx_linear_gradient_start,
    .on_stop = fx_linear_gradient_stop,
    .render_frame = fx_linear_gradient_render_frame,
};

#define FX_LINEAR_GRADIENT_DEVICE(idx)                                                             \
                                                                                                   \
    static size_t fx_linear_gradient_##idx##_pixel_map[] = DT_INST_PROP(idx, pixels);              \
                                                                                                   \
    static uint32_t fx_linear_gradient_##idx##_colors_hsl[] = DT_INST_PROP(idx, colors);           \
                                                                                                   \
    static struct zmk_color_rgb fx_linear_gradient_##idx##_colors_rgb[                             \
        DT_INST_PROP_LEN(idx, colors)];                                                            \
                                                                                                   \
    static struct fx_linear_gradient_config fx_linear_gradient_##idx##_config = {                  \
        .colors_hsl = (struct zmk_color_hsl *)fx_linear_gradient_##idx##_colors_hsl,               \
        .colors_rgb = fx_linear_gradient_##idx##_colors_rgb,                                       \
        .num_colors = DT_INST_PROP_LEN(idx, colors),                                               \
        .pixel_map = fx_linear_gradient_##idx##_pixel_map,                                         \
        .pixel_map_size = DT_INST_PROP_LEN(idx, pixels),                                           \
        .blending_mode = DT_INST_ENUM_IDX(idx, blending_mode),                                     \
        .use_hsl = !DT_INST_PROP(idx, use_rgb_interpolation),                                      \
        .gradient_width = DT_INST_PROP(idx, gradient_width),                                       \
        .offset_per_frame = DT_INST_PROP(idx, duration) > 0                                        \
            ? (float)DT_INST_PROP(idx, gradient_width) /                                           \
                (float)(DT_INST_PROP(idx, duration) * CONFIG_ZMK_RGB_FX_FPS)                       \
            : 0,                                                                                   \
    };                                                                                             \
                                                                                                   \
    static struct fx_linear_gradient_data fx_linear_gradient_##idx##_data = {                      \
        .offset = 0,                                                                               \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(idx, &fx_linear_gradient_init, NULL, &fx_linear_gradient_##idx##_data,   \
                          &fx_linear_gradient_##idx##_config, POST_KERNEL,                         \
                          CONFIG_APPLICATION_INIT_PRIORITY, &fx_linear_gradient_api);

DT_INST_FOREACH_STATUS_OKAY(FX_LINEAR_GRADIENT_DEVICE);
