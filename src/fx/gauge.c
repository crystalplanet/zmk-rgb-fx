

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

    bool is_horizontal;
    uint8_t blending_mode;

    uint8_t gradient_min_width;
    uint8_t gradient_max_width;
    bool rgb_interpolation;
};

static struct zmk_color_rgb fx_gauge_frame_color(const struct fx_gauge_config *config, ?? value) {
    size_t i = 0;
    while (config->color_threshold[i] < level && ++i < config->colors_size);

    switch (config->color_gradient) {
    case FX_GAUGE_GRADIENT_HSL:

    case FX_GAUGE_GRADIENT_RGB:

    case FX_GAUGE_GRADIENT_NONE:
    default:
        return config->colors_rgb[i - 1];
    }
}

static void fx_gauge_render_frame(const struct device *dev, struct rgb_fx_pixel *pixels,
                                  size_t num_pixels) {
    const struct fx_gauge_config *config = dev->config;

    const size_t *pixel_map = config->pixel_map;

    // Should work for both of the following:
    //
    // bounds_min_from: 0, bounds_min_to: 0 -> bounds_max_from: 0, bounds_max_to: 255
    // bounds_min_from: 84, bounds_min_to: 154 -> bounds_max_from: 0, bounds_max_to: 255

    // Returns a numeric value (int)
    const int value = zmk_rgb_fx_gauge_value(config->source);
    const float step = (float)(value - config->value_min) / (float)(config->value_max - config->value_min);

    // need to clamp step between 0 and 1

    const struct zmk_color_rgb base_color = config->colors_size > 1
                                                ? fx_gauge_frame_color(config, step)
                                                : config->colors_rgb[0];

    // for each pixel we'll need to calculate

    for (size_t i = 0; i < config->pixel_map_size; ++i) {
        int 
    }

    // good for displaying values but how do we trigger it? let's not worry about it.
}
