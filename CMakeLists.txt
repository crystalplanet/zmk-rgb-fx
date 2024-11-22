if (CONFIG_ZMK_RGB_FX)

target_include_directories(app PRIVATE include)

target_sources(app PRIVATE src/color.c)
target_sources(app PRIVATE src/rgb_fx.c)

target_sources(app PRIVATE src/behaviors/behavior_rgb_fx.c)

target_sources(app PRIVATE src/fx/compose.c)
target_sources(app PRIVATE src/fx/control_group.c)
target_sources(app PRIVATE src/fx/linear_gradient.c)
target_sources(app PRIVATE src/fx/ripple.c)
target_sources(app PRIVATE src/fx/solid.c)
target_sources(app PRIVATE src/fx/sparkle.c)
target_sources(app PRIVATE src/fx/wpm.c)

endif()
