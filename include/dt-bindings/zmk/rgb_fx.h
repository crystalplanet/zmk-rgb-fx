/*
 * Copyright (c) 2024 Kuba Birecki
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * Maps HSL color settings into a single uint32_t value
 * that can be cast to zmk_color_hsl.
 */
#ifdef CONFIG_BIG_ENDIAN
#define HSL(h, s, l) ((h << 16) + (s << 8) + l)
#else
#define HSL(h, s, l) (h + (s << 16) + (l << 24))
#endif

/**
 * Effect blending modes
 */
#define BLENDING_MODE_NORMAL 0
#define BLENDING_MODE_MULTIPLY 1
#define BLENDING_MODE_LIGHTEN 2
#define BLENDING_MODE_DARKEN 3
#define BLENDING_MODE_SCREEN 4
#define BLENDING_MODE_SUBTRACT 5

/**
 * Effect control commands
 */
#define RGBFX_CMD_TOGGLE 0
#define RGBFX_CMD_NEXT 1
#define RGBFX_CMD_PREVIOUS 2
#define RGBFX_CMD_SELECT 3
#define RGBFX_CMD_BRIGHTEN 4
#define RGBFX_CMD_DIM 5
#define RGBFX_CMD_NEXT_CONTROL_ZONE 6
#define RGBFX_CMD_PREVIOUS_CONTROL_ZONE 7

/**
 * Generic animation command macro
 */
#define RGBFX_CONTROL_CMD(command, zone, param) ((zone << 24) | (command << 16) | (param))

/**
 * Effect behavior helpers
 */
#define RGBFX_TOGGLE(zone) RGBFX_CONTROL_CMD(RGBFX_CMD_TOGGLE, zone, 0)
#define RGBFX_NEXT(zone) RGBFX_CONTROL_CMD(RGBFX_CMD_NEXT, zone, 0)
#define RGBFX_PREVIOUS(zone) RGBFX_CONTROL_CMD(RGBFX_CMD_PREVIOUS, zone, 0)
#define RGBFX_SELECT(zone, target_animation)                                                       \
    RGBFX_CONTROL_CMD(RGBFX_CMD_SELECT, zone, target_animation)
#define RGBFX_BRIGHTEN(zone) RGBFX_CONTROL_CMD(RGBFX_CMD_BRIGHTEN, zone, 0)
#define RGBFX_DIM(zone) RGBFX_CONTROL_CMD(RGBFX_CMD_DIM, zone, 0)
#define RGBFX_NEXT_CONTROL_ZONE RGBFX_CONTROL_CMD(RGBFX_CMD_NEXT_CONTROL_ZONE, 0, 0)
#define RGBFX_PREVIOUS_CONTROL_ZONE                                                                \
    RGBFX_CONTROL_CMD(RGBFX_CMD_PREVIOUS_CONTROL_ZONE, 0, 0)
