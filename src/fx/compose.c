/*
 * Copyright (c) 2024 Kuba Birecki
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_rgb_fx_compose

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <drivers/rgb_fx.h>

#include <zmk/rgb_fx.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define PHANDLE_TO_DEVICE(node_id, prop, idx) DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

struct fx_compose_config {
    const struct device **fx;
    const size_t fx_size;
};

static void fx_compose_render_frame(const struct device *dev, struct rgb_fx_pixel *pixels,
                                    size_t num_pixels) {
    const struct fx_compose_config *config = dev->config;

    for (size_t i = 0; i < config->fx_size; ++i) {
        rgb_fx_render_frame(config->fx[i], pixels, num_pixels);
    }
}

static void fx_compose_start(const struct device *dev) {
    const struct fx_compose_config *config = dev->config;

    for (size_t i = 0; i < config->fx_size; ++i) {
        rgb_fx_start(config->fx[i]);
    }
}

static void fx_compose_stop(const struct device *dev) {
    const struct fx_compose_config *config = dev->config;

    for (size_t i = 0; i < config->fx_size; ++i) {
        rgb_fx_stop(config->fx[i]);
    }
}

static int fx_compose_init(const struct device *dev) { return 0; }

static const struct rgb_fx_api fx_compose_api = {
    .on_start = fx_compose_start,
    .on_stop = fx_compose_stop,
    .render_frame = fx_compose_render_frame,
};

#define FX_COMPOSE_DEVICE(idx)                                                                     \
                                                                                                   \
    static const struct device *fx_compose_##idx##_fx[] = {                                        \
        DT_INST_FOREACH_PROP_ELEM(idx, fx, PHANDLE_TO_DEVICE)};                                    \
                                                                                                   \
    static struct fx_compose_config fx_compose_##idx##_config = {                                  \
        .fx = fx_compose_##idx##_fx,                                                               \
        .fx_size = DT_INST_PROP_LEN(idx, fx),                                                      \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(idx, &fx_compose_init, NULL, NULL,                                       \
                          &fx_compose_##idx##_config, POST_KERNEL,                                 \
                          CONFIG_APPLICATION_INIT_PRIORITY, &fx_compose_api);

DT_INST_FOREACH_STATUS_OKAY(FX_COMPOSE_DEVICE);
