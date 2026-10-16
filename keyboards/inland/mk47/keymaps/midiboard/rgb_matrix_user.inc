// SPDX-License-Identifier: GPL-2.0-or-later

// "Resolume" mode: every key is dark unless the host has given it a colour
// over MIDI (those are drawn on top in keymap.c).
RGB_MATRIX_EFFECT(RESOLUME)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

static bool RESOLUME(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    for (uint8_t i = led_min; i < led_max; i++) {
        rgb_matrix_set_color(i, 0, 0, 0);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
