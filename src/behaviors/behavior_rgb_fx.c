/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>

#include <zmk/rgb_fx_control_group.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT zmk_rgb_fx_control_group

#define DEVICE_ADDR(idx) DEVICE_DT_GET(DT_INST(idx, zmk_rgb_fx_control_group)),

/**
 * Control animation instance pointers.
 */
static const struct device *control_fx[] = {DT_INST_FOREACH_STATUS_OKAY(DEVICE_ADDR)};

/**
 * Size of control animation instance pointers array.
 */
static const uint8_t control_fx_size = sizeof(control_fx);

/**
 * Index of the current default control animation instance.
 */
static uint8_t current_zone = 0;

#define DT_DRV_COMPAT zmk_behavior_rgb_fx

static int
on_keymap_binding_convert_central_state_dependent_params(struct zmk_behavior_binding *binding,
                                                         struct zmk_behavior_binding_event event) {
    if ((binding->param1 >> 24) == 0xff) {
        binding->param1 = (current_zone << 24) | (binding->param1 & 0x00ffffff);
    }

    return 0;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    uint8_t value = binding->param1 & 0xff;
    uint8_t command = (binding->param1 >> 16) & 0xff;
    uint8_t zone = (binding->param1 >> 24) & 0xff;

    if (command == RGB_FX_CMD_NEXT_CONTROL_ZONE) {
        current_zone++;

        if (current_zone == control_fx_size) {
            current_zone = 0;
        }
        return 0;
    }

    if (command == RGB_FX_CMD_PREVIOUS_CONTROL_ZONE) {
        if (current_zone == 0) {
            current_zone = control_fx_size;
        }

        current_zone--;
        return 0;
    }

    if (control_fx_size <= zone) {
        return -ENOTSUP;
    }

    return zmk_rgb_fx_control_handle_command(control_fx[zone], command, value);
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_rgb_fx_init(const struct device *dev) { return 0; }

static const struct behavior_driver_api behavior_rgb_fx_driver_api = {
    .binding_convert_central_state_dependent_params =
        on_keymap_binding_convert_central_state_dependent_params,
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

BEHAVIOR_DT_INST_DEFINE(0, behavior_rgb_fx_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_rgb_fx_driver_api);
