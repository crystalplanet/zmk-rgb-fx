/**
 * Copyright (c) 2024 Kuba Birecki
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_rgb_fx_static

#include <stdlib.h>
#include <math.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/rgb_fx.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct fx_static_config {
	size_t *pixel_map;
	size_t pixel_map_size;
	struct zmk_color_hsl *colors_hsl;
	struct zmk_color_rgb *colors_rgb;
    uint8_t blending_mode;
};

static void fx_static_render_frame(const struct device *dev, struct rgb_fx_pixel *pixels,
								   size_t num_pixels) {
	const struct fx_static_config *config = dev->config;

	for (size_t i = 0; i < config->pixel_map_size; ++i) {
        pixels[config->pixel_map[i]].value =
            zmk_apply_blending_mode(pixels[config->pixel_map[i]].value, config->colors_rgb[i],
            					    config->blending_mode);
	}
}

static void fx_static_start(const struct device *dev) {
	zmk_rgb_fx_request_frames(1);
}

static void fx_static_stop(const struct device *dev) {
	// Nothing to do.
}

static int fx_static_init(const struct device *dev) {
	const struct fx_static_config *config = dev->config;

    for (size_t i = 0; i < config->pixel_map_size; ++i) {
        zmk_hsl_to_rgb(&config->colors_hsl[i], &config->colors_rgb[i]);
    }

	return 0;
}

static const struct rgb_fx_api fx_static_api = {
	.on_start = fx_static_start,
	.on_stop = fx_static_stop,
	.render_frame = fx_static_render_frame,
};

#define FX_STATIC_DEVICE(idx)                                                                      \
                                                                                                   \
	static size_t fx_static_##idx##_pixel_map[] = DT_INST_PROP(idx, pixels);                       \
                                                                                                   \
	static uint32_t fx_static_##idx##_colors_hsl[] = DT_INST_PROP(idx, colors);                    \
                                                                                                   \
	static struct zmk_color_rgb fx_static_##idx##_colors_rgb[DT_INST_PROP_LEN(idx, colors)];       \
                                                                                                   \
	static struct fx_static_config fx_static_##idx##_config = {                                    \
		.pixel_map = fx_static_##idx##_pixel_map,                                                  \
		.pixel_map_size = DT_INST_PROP_LEN(idx, pixels),                                           \
		.colors_hsl = (struct zmk_color_hsl *)fx_static_##idx##_colors_hsl,                        \
		.colors_rgb = fx_static_##idx##_colors_rgb,                                                \
	    .blending_mode = DT_INST_ENUM_IDX(idx, blending_mode),                                     \
	};                                                                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, &fx_static_init, NULL, NULL, &fx_static_##idx##_config, POST_KERNEL,\
		                  CONFIG_APPLICATION_INIT_PRIORITY, &fx_static_api);

DT_INST_FOREACH_STATUS_OKAY(FX_STATIC_DEVICE);
