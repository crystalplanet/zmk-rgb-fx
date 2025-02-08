enum zmk_rgb_fx_orientation {
	FX_DIR_HORIZONTAL,
	FX_DIR_VERTICAL,
};

struct rgb_fx_gauge_config {
	size_t *pixel_map,
	size_t pixel_map_size,

	// not necessary!
	struct zmk_color_hsl *colors;	
	size_t colors_size;
	uint8_t *color_thresholds;
	size_t color_thresholds_size;
	// not necessary!

	uint8_t bounds_min;
	uint8_t bounds_max;
	uint8_t gradient_width;
	bool is_horfizontal;
};

struct rgb_fx_gauge_data {
	int counter; // NEGATIVE for going back, POSITIVE for going forward
};

void rgb_fx_render_gauge(struct *rgb_fx_pixels, struct rgb_fx_gauge_config *config,
						 struct rgb_fx_gauge_data *data, struct zmk_color_rgb color, float step,
						 uint8_t blending_mode) {
	const int direction = config->bounds_min < config->bounds_max ? 1 : -1;
	const int position = config->bounds_min + (abs(config->bounds_max - config->bounds_min) + config->gradient_width) + step * direction;

	// If data is set, might want to use that to animate step too!

	for (size_t i = 0; i < config->pixel_map_size; ++i) {
		const int px_position = config->is_horizontal ? pixels[pixel_map[i]].position_x
													  : pixels[pixel_map[i]].position_y;

        if (0 <= direction * (px_position - position)) {
        	pixels[pixel_map[i]].value =
        		zmk_apply_blending_mode(pixels[pixel_map[i]].value, {0f, 0f, 0f}, blending_mode);
        	continue;
        }

        if (config->gradient_width <= direction * (position - px_position)) {
        	pixels[pixel_map[i]].value =
        		zmk_apply_blending_mode(pixels[pixel_map[i]].value, color, blending_mode);
        	continue;
        }

        float gradient_step = abs(px_position - position) / (float)config->gradient_width;
        zmk_apply_blending_mode(
        	pixels[pixel_map[i]].value,
        	{color.r * step, color.g * step, color.b * step},
        	blending_mode
        );
	}
}
