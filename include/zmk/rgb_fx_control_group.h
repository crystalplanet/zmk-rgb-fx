/*
 * Copyright (c) 2024 Kuba Birecki
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>
#include <zephyr/kernel.h>

/**
 * Animation control commands
 */
#define RGB_FX_CMD_TOGGLE 0
#define RGB_FX_CMD_NEXT 1
#define RGB_FX_CMD_PREVIOUS 2
#define RGB_FX_CMD_SELECT 3
#define RGB_FX_CMD_BRIGHTEN 4
#define RGB_FX_CMD_DIM 5
#define RGB_FX_CMD_NEXT_CONTROL_ZONE 6
#define RGB_FX_CMD_PREVIOUS_CONTROL_ZONE 7

int zmk_rgb_fx_control_handle_command(const struct device *dev, uint8_t command, uint8_t param);
