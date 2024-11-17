/*
 * Copyright (c) 2024 Kuba Birecki
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>
#include <zephyr/device.h>

#include <zmk/rgb_fx.h>

/**
 * @file
 * #brief Public API for controlling RGB effects.
 *
 * This library abstracts the implementation details
 * for various types of 2D effects.
 */

struct rgb_fx_pixel {
    const uint8_t position_x;
    const uint8_t position_y;

    struct zmk_color_rgb value;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef rgb_fx_start
 * @brief Callback API for starting an animation.
 *
 * @see rgb_fx_start() for argument descriptions.
 */
typedef void (*rgb_fx_api_start)(const struct device *dev);

/**
 * @typedef animation_stop
 * @brief Callback API for stopping an animation.
 *
 * @see animation_stop() for argument descriptions.
 */
typedef void (*rgb_fx_api_stop)(const struct device *dev);

/**
 * @typedef animation_render_frame
 * @brief Callback API for generating the next animation frame.
 *
 * @see animation_render_frame() for argument descriptions.
 */
typedef void (*rgb_fx_api_render_frame)(const struct device *dev, struct rgb_fx_pixel *pixels,
                                        size_t num_pixels);

struct rgb_fx_api {
    rgb_fx_api_start on_start;
    rgb_fx_api_stop on_stop;
    rgb_fx_api_render_frame render_frame;
};

static inline void rgb_fx_start(const struct device *dev) {
    const struct rgb_fx_api *api = (const struct rgb_fx_api *)dev->api;

    return api->on_start(dev);
}

static inline void rgb_fx_stop(const struct device *dev) {
    const struct rgb_fx_api *api = (const struct rgb_fx_api *)dev->api;

    return api->on_stop(dev);
}

static inline void rgb_fx_render_frame(const struct device *dev, struct rgb_fx_pixel *pixels,
                                       size_t num_pixels) {
    const struct rgb_fx_api *api = (const struct rgb_fx_api *)dev->api;

    return api->render_frame(dev, pixels, num_pixels);
}

#ifdef __cplusplus
}
#endif
