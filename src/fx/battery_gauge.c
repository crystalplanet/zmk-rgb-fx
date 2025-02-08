/*
 * Copyright (c) 2024 Kuba Birecki
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_rgb_fx_battery_gauge

#include <stdlib.h>
#include <math.h>

#include <zmk/battery.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct fx_battery_gauge_config {

};

// charging gpio

static void fx_battery_gauge_toggle(const struct device *dev) {
    struct fx_battery_gauge_data *data = dev->data;

    // If active, let's turn it off, if not active, let's turn on

    data->is_active = !data->is_active;
    data->battery_level = zmk_battery_state_of_charge();

    zmk_rgb_fx_request_frames(1);
}

static struct zmk_color_rgb fx_battery_gauge_base_color(const struct device *dev, uint8_t level) {
    const struct fx_battery_gauge_config *config = dev->config;

    size_t i = 0;
    while (config->color_threshold[i] < level && ++i < config->colors_size);

    return config->colors_rgb[i - 1];
}

static void fx_battery_gauge_render_frame(const struct device *dev, struct rgb_fx_pixel *pixels,
                                          size_t num_pixels) {
    const struct fx_battery_gauge_config *config = dev->config;
    struct fx_battery_gauge_data *data = dev->data;

    if (!data->is_active && data->counter == 0) {
        return;
    }

    const struct zmk_color_rgb base_color = fx_battery_gauge_base_color(dev, data->battery_level);
    const float level = (float)data->battery_level / 100.0f;

    for (size_t i = 0; i < config->pixel_map_size; ++i) {
        const int position = config->is_horizontal ? pixels[pixel_map[i]].position_x
                                                   : pixels[pixel_map[i]].position_y;

        pixels[pixel_map[i]].value = zmk_apply_blending_mode(pixels[pixel_map[i]].value, color, config->blending_mode);
    }

    zmk_rgb_fx_request_frames(1);
}

// transition => HOW LONG IT TAKES FOR THE INDICATOR TO ANIMATE ITSELF
// FADE => FADE THE BACKGROUND IN & OUT
