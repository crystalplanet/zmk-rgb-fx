/**
 * Copyright (c) 2024 Kuba Birecki
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_rgb_fx_sparkle

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/rgb_fx.h>

#include <zmk/rgb_fx.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct fx_sparkle_pixel {
    struct zmk_color_rgb color;
    uint16_t total_frames;
    uint16_t counter;
    float step;
};

struct fx_sparkle_config {
    size_t *pixel_map;
    size_t pixel_map_size;
    struct zmk_color_hsl *colors;
    size_t num_colors;
    uint8_t duration;
    uint8_t blending_mode;
};

struct fx_sparkle_data {
    struct fx_sparkle_pixel *pixels;
};

static void fx_sparkle_generate_pixel(const struct device *dev, uint8_t ipx, bool offset_counter) {
    const struct fx_sparkle_config *config = dev->config;
    const struct fx_sparkle_data *data = dev->data;

    if (config->num_colors > 1) {
        struct zmk_color_hsl color;

        zmk_interpolate_hsl(&config->colors[0], &config->colors[1], &color,
                            1.0f / (float)(rand() % 100));
        zmk_hsl_to_rgb(&color, &data->pixels[ipx].color);
    } else {
        zmk_hsl_to_rgb(&config->colors[0], &data->pixels[ipx].color);
    }

    data->pixels[ipx].total_frames =
        (config->duration * CONFIG_ZMK_RGB_FX_FPS) / ((rand() % 16) + 1);
    data->pixels[ipx].counter = 2 * data->pixels[ipx].total_frames;
    data->pixels[ipx].step = 1.0f / (float)data->pixels[ipx].total_frames;

    if (offset_counter) {
        data->pixels[ipx].counter = rand() % data->pixels[ipx].counter;
    }
}

static void fx_sparkle_render_frame(const struct device *dev, struct rgb_fx_pixel *pixels,
                                    size_t num_pixels) {
    const struct fx_sparkle_config *config = dev->config;
    struct fx_sparkle_data *data = dev->data;

    for (int i = 0; i < config->pixel_map_size; ++i) {
        --data->pixels[i].counter;

        float intensity = data->pixels[i].total_frames <= data->pixels[i].counter
                              ? 2.0f - (data->pixels[i].step * (float)(data->pixels[i].counter))
                              : data->pixels[i].step * (float)(data->pixels[i].counter);

        struct zmk_color_rgb color = {
            .r = intensity * data->pixels[i].color.r,
            .g = intensity * data->pixels[i].color.g,
            .b = intensity * data->pixels[i].color.b,
        };

        pixels[config->pixel_map[i]].value = zmk_apply_blending_mode(
            pixels[config->pixel_map[i]].value, color, config->blending_mode);

        if (data->pixels[i].counter == 0) {
            fx_sparkle_generate_pixel(dev, i, false);
        }
    }

    zmk_rgb_fx_request_frames(1);
}

static void fx_sparkle_start(const struct device *dev) {
    zmk_rgb_fx_request_frames(1);
}

static void fx_sparkle_stop(const struct device *dev) {
    // Nothing to do.
}

static int fx_sparkle_init(const struct device *dev) {
    const struct fx_sparkle_config *config = dev->config;

    for (uint8_t i = 0; i < config->pixel_map_size; ++i) {
        fx_sparkle_generate_pixel(dev, i, true);
    }

    return 0;
}

static const struct rgb_fx_api fx_sparkle_api = {
    .on_start = fx_sparkle_start,
    .on_stop = fx_sparkle_stop,
    .render_frame = fx_sparkle_render_frame,
};

#define FX_SPARKLE_DEVICE(idx)                                                                     \
                                                                                                   \
    static size_t fx_sparkle_##idx##_pixel_map[] = DT_INST_PROP(idx, pixels);                      \
                                                                                                   \
    static uint32_t fx_sparkle_##idx##_colors[] = DT_INST_PROP(idx, colors);                       \
                                                                                                   \
    static struct fx_sparkle_config fx_sparkle_##idx##_config = {                                  \
        .pixel_map = &fx_sparkle_##idx##_pixel_map[0],                                             \
        .pixel_map_size = DT_INST_PROP_LEN(idx, pixels),                                           \
        .colors = (struct zmk_color_hsl *)&fx_sparkle_##idx##_colors,                              \
        .num_colors = DT_INST_PROP_LEN(idx, colors),                                               \
        .duration = DT_INST_PROP(idx, duration),                                                   \
        .blending_mode = DT_INST_ENUM_IDX(idx, blending_mode),                                     \
    };                                                                                             \
                                                                                                   \
    static struct fx_sparkle_pixel                                                                 \
        fx_sparkle_##idx##_pixels[DT_INST_PROP_LEN(idx, pixels)];                                  \
                                                                                                   \
    static struct fx_sparkle_data fx_sparkle_##idx##_data = {                                      \
        .pixels = &fx_sparkle_##idx##_pixels[0],                                                   \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(idx, &fx_sparkle_init, NULL, &fx_sparkle_##idx##_data,                   \
                          &fx_sparkle_##idx##_config, POST_KERNEL,                                 \
                          CONFIG_APPLICATION_INIT_PRIORITY, &fx_sparkle_api);

DT_INST_FOREACH_STATUS_OKAY(FX_SPARKLE_DEVICE);
