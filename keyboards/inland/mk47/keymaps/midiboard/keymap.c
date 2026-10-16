/* Copyright (C) 2025 jonylee@hfd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H

#include "midi.h"
#include "qmk_midi.h"

#include "launchpad_palette.h"

enum custom_keycodes {
    LP_REC = SAFE_RANGE, // arm / stop recording
    LP_CLR,              // stop and clear the loop
    LP_PLAY,             // stop the loop, or start it again from the top
    LP_SLOW,             // loop playback speed down / up
    LP_FAST,
    LP_TRD,              // transpose loop playback down / up an octave
    LP_TRU,
    LP_STD,              // transpose loop playback down / up a semitone
    LP_STU,
    LP_UNDO,             // remove the most recent overdub
    KB_PIANO,            // toggle the piano-key lighting
    KB_LEDOFF,           // clear every colour set over MIDI
};

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    [0] = LAYOUT_planck_mit(
        MI_C4, MI_Cs4,  MI_D4, MI_Ds4,  MI_E4, MI_F4,  MI_Fs4,  MI_G4,  MI_Gs4, MI_A4, MI_As4, MI_B4,
        MI_C3, MI_Cs3,  MI_D3, MI_Ds3,  MI_E3, MI_F3,  MI_Fs3,  MI_G3,  MI_Gs3, MI_A3, MI_As3, MI_B3,
        MI_C2, MI_Cs2,  MI_D2, MI_Ds2,  MI_E2, MI_F2,  MI_Fs2,  MI_G2,  MI_Gs2, MI_A2, MI_As2, MI_B2,
        MI_C1, MI_Cs1,  MI_D1, MI_Ds1,  MI_E1,     KC_ENT,      MI_G1,  MI_Gs1, MI_A1, MI_As1, MO(2)
    ),


    [1] = LAYOUT_planck_mit(

        MI_G2, MI_Gs2, MI_A2,  MI_As2, MI_B2,  MI_C3,  MI_Cs3, MI_D3, MI_Ds3, MI_E3,  MI_F3,  MI_Fs3,
        MI_D2, MI_Ds2, MI_E2,  MI_F2,  MI_Fs2, MI_G2,  MI_Gs2, MI_A2, MI_As2, MI_B2,  MI_C3,  MI_Cs3,
        MI_A1, MI_As1, MI_B1,  MI_C2,  MI_Cs2, MI_D2,  MI_Ds2, MI_E2, MI_F2,  MI_Fs2, MI_G2,  MI_Gs2,
        MI_E1, MI_F1,  MI_Fs1, MI_G1,  MI_Gs1,     MI_A1,      MI_B1,  MI_C2,  MI_Cs2, MI_D2, MO(2)

    ),

    [2] = LAYOUT_planck_mit(


        MI_CH1, MI_CH2,   MI_CH3,   MI_CH4,   MI_CH5,   MI_CH6,   MI_CH7,   MI_CH8,   MI_CH9,   MI_CH10,   TG(1),  RGB_MOD,
        RGB_HUI, MI_CHND, MI_CHNU, KC_TRNS, KC_TRNS, KC_TRNS, KB_LEDOFF, RGB_SPD, RGB_SPI, RGB_VAD, RGB_VAI, KB_PIANO,
        LP_STD,  LP_STU,  LP_TRD,  LP_TRU,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        MI_OCTD, MI_OCTU, LP_SLOW, LP_FAST, LP_CLR,      LP_PLAY,      KC_TRNS, KC_TRNS, LP_UNDO, LP_REC,  KC_NO
)

};
// clang-format on

// Host-controlled lighting: a note-on received over MIDI lights every key that
// currently plays that note, with the velocity picking a colour from the
// Launchpad palette. Only velocity 0 clears it; note-offs are ignored. Keys
// with no host colour keep showing the normal RGB effect.
// Colours are kept per MIDI channel and the keyboard shows the set for the
// channel it is currently sending on, so each channel is its own page.
static uint8_t note_color[16][128];

// Guide lighting (piano keys, or fret markers on the bass guitar layout),
// toggled from the shift layer. Off at power-up.
static bool piano_lights;

// Brightest channel of any lit host colour is raised to at least this (of 255).
#define MIN_VISIBLE_LEVEL 80

static void note_on_received(MidiDevice *device, uint8_t status, uint8_t note, uint8_t velocity) {
    note_color[status & 0x0F][note & 0x7F] = velocity & 0x7F;
}

// Looper. LP_REC arms it; recording starts on the first note played and stops
// on the next LP_REC, which starts the loop playing. The loop length runs from
// that first note to the second LP_REC press and never changes after that.
// With a loop in memory, LP_REC instead switches overdub on and off: notes
// played while it is on are added to the loop where they fall. LP_PLAY stops
// the loop and restarts it from the top, LP_CLR discards it, and the speed and
// transpose keys change how the loop plays back without altering what was
// recorded.
#define LOOP_MAX_EVENTS 1024
#define LOOP_MAX_MS 0xFFFFF // 20-bit timestamps, about 17 minutes
#define NOT_SOUNDING 0xFF
#define LOOP_MAX_TRANSPOSE 48

typedef struct {
    uint32_t time : 20; // ms from the start of the loop
    uint32_t note : 7;
    uint32_t on : 1;
    uint32_t channel : 4;
} loop_event_t;

enum loop_state { LOOP_IDLE, LOOP_ARMED, LOOP_RECORDING, LOOP_PLAYING, LOOP_STOPPED };

// Playback speeds in 1/256ths. Each step doubles or halves, so the loop stays
// in time with what was recorded: from 1/32 of the recorded speed up to 32x.
static const uint16_t PROGMEM loop_speeds[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
#define LOOP_NORMAL_SPEED 5

static enum loop_state loop_state;
static loop_event_t    loop_events[LOOP_MAX_EVENTS];
static uint16_t        loop_event_count;
static uint16_t        loop_next_event;
static uint32_t        loop_record_start; // timer value at the first recorded note
static uint32_t        loop_length;       // ms; 0 when there is no finished loop
static uint32_t        loop_position;     // playback position in 1/256 ms
static uint32_t        loop_last_tick;
static uint8_t         loop_speed     = LOOP_NORMAL_SPEED;
static int8_t          loop_transpose = 0;
static bool            loop_overdub; // only meaningful while playing

// Which pass each event came from: 0 for the first recording, then one number
// per punch-in, so the latest overdub can be undone.
static uint8_t loop_event_layer[LOOP_MAX_EVENTS];
static uint8_t loop_layer;
static bool    loop_layer_used; // the current punch-in has added something

// What each physical tone key is holding, so a release is recorded with the
// note and channel of its press even if octave or channel changed since.
static uint8_t held_count[MIDI_TONE_COUNT];
static uint8_t held_note[MIDI_TONE_COUNT];
static uint8_t held_channel[MIDI_TONE_COUNT];

// Note each tone key added to the loop during overdub and still has open, or
// NOT_SOUNDING, so its release can be added too.
static uint8_t overdub_note[MIDI_TONE_COUNT];

// For each recorded note the loop currently has sounding: the note actually
// sent (after transpose) and its channel, so it can be released exactly.
static uint8_t loop_sounding_note[128];
static uint8_t loop_sounding_channel[128];

static void loop_note_off(uint8_t note) {
    if (loop_sounding_note[note] != NOT_SOUNDING) {
        midi_send_noteoff(&midi_device, loop_sounding_channel[note], loop_sounding_note[note], 0);
        loop_sounding_note[note] = NOT_SOUNDING;
    }
}

static void loop_silence(void) {
    for (uint8_t note = 0; note < 128; ++note) {
        loop_note_off(note);
    }
}

static void loop_add_event(uint32_t time, uint8_t note, uint8_t channel, bool on) {
    if (loop_event_count < LOOP_MAX_EVENTS) {
        loop_event_layer[loop_event_count] = 0;
        loop_events[loop_event_count++]    = (loop_event_t){.time = time, .note = note, .on = on, .channel = channel};
    }
}

// Adds an event to a playing loop at the current playback position. It goes in
// just behind the play head, so it is first heard on the next pass.
static void loop_insert_event(uint8_t note, uint8_t channel, bool on) {
    if (loop_event_count >= LOOP_MAX_EVENTS) {
        return;
    }
    uint32_t time = MIN(loop_position >> 8, loop_length);
    uint16_t following = loop_event_count - loop_next_event;
    memmove(&loop_events[loop_next_event + 1], &loop_events[loop_next_event], following * sizeof(loop_event_t));
    memmove(&loop_event_layer[loop_next_event + 1], &loop_event_layer[loop_next_event], following);
    loop_event_layer[loop_next_event] = loop_layer;
    loop_events[loop_next_event++]    = (loop_event_t){.time = time, .note = note, .on = on, .channel = channel};
    loop_event_count++;
    loop_layer_used = true;
}

static void loop_overdub_note(uint8_t tone, bool on) {
    if (on) {
        // Store the note so that it plays back at the pitch being heard now.
        int16_t note = held_note[tone] - loop_transpose;
        // Keep room in reserve so every added note can also get its release.
        if (!loop_overdub || note < 0 || note > 127 || loop_event_count >= LOOP_MAX_EVENTS - MIDI_TONE_COUNT) {
            return;
        }
        overdub_note[tone] = note;
        loop_insert_event(note, held_channel[tone], true);
    } else if (overdub_note[tone] != NOT_SOUNDING) {
        loop_insert_event(overdub_note[tone], held_channel[tone], false);
        overdub_note[tone] = NOT_SOUNDING;
    }
}

static void loop_begin_overdub(void) {
    if (loop_layer < UINT8_MAX) {
        loop_layer++;
    }
    loop_layer_used = false;
    loop_overdub    = true;
}

// Leaves overdub, releasing in the loop any added note that is still held.
static void loop_end_overdub(void) {
    if (!loop_overdub) {
        return;
    }
    for (uint8_t tone = 0; tone < MIDI_TONE_COUNT; ++tone) {
        loop_overdub_note(tone, false);
    }
    loop_overdub = false;
    // A punch-in that added nothing is not worth an undo step.
    if (!loop_layer_used && loop_layer > 0) {
        loop_layer--;
    }
}

// Removes everything added by the most recent punch-in. The first recording
// is never removed this way; that is what clear is for.
static void loop_undo(void) {
    loop_end_overdub();
    if (loop_layer == 0) {
        return;
    }
    uint16_t kept = 0, next = loop_next_event;
    for (uint16_t i = 0; i < loop_event_count; ++i) {
        if (loop_event_layer[i] == loop_layer) {
            loop_note_off(loop_events[i].note);
            if (i < loop_next_event) {
                next--;
            }
        } else {
            loop_events[kept]        = loop_events[i];
            loop_event_layer[kept++] = loop_event_layer[i];
        }
    }
    loop_event_count = kept;
    loop_next_event  = next;
    loop_layer--;
}

static void loop_play_from_top(void) {
    loop_position   = 0;
    loop_next_event = 0;
    loop_last_tick  = timer_read32();
    loop_state      = LOOP_PLAYING;
}

static void loop_finish_recording(uint32_t now) {
    loop_length = now - loop_record_start;
    if (loop_length == 0 || loop_event_count == 0) {
        loop_length = 0;
        loop_state  = LOOP_IDLE;
        return;
    }
    // Close any note still held so it does not hang across the loop point.
    for (uint8_t tone = 0; tone < MIDI_TONE_COUNT; ++tone) {
        if (held_count[tone] > 0) {
            loop_add_event(loop_length, held_note[tone], held_channel[tone], false);
        }
    }
    loop_play_from_top();
}

static void loop_record_note(uint8_t note, uint8_t channel, bool on) {
    uint32_t now = timer_read32();
    if (loop_state == LOOP_ARMED && on) {
        loop_event_count  = 0;
        loop_layer        = 0;
        loop_length       = 0;
        loop_speed        = LOOP_NORMAL_SPEED;
        loop_transpose    = 0;
        loop_record_start = now;
        loop_state        = LOOP_RECORDING;
    }
    if (loop_state != LOOP_RECORDING) {
        return;
    }
    loop_add_event(now - loop_record_start, note, channel, on);
    // Out of room: close the loop here rather than silently dropping notes.
    if (loop_event_count >= LOOP_MAX_EVENTS - MIDI_TONE_COUNT || now - loop_record_start >= LOOP_MAX_MS) {
        loop_finish_recording(now);
    }
}

static void loop_play_event(loop_event_t event) {
    loop_note_off(event.note);
    if (event.on) {
        int16_t note = event.note + loop_transpose;
        if (note >= 0 && note <= 127) {
            midi_send_noteon(&midi_device, event.channel, note, midi_config.velocity);
            loop_sounding_note[event.note]    = note;
            loop_sounding_channel[event.note] = event.channel;
        }
    }
}

static void loop_shift_transpose(int8_t semitones) {
    int8_t shifted = loop_transpose + semitones;
    if (shifted >= -LOOP_MAX_TRANSPOSE && shifted <= LOOP_MAX_TRANSPOSE) {
        loop_transpose = shifted;
    }
}

void keyboard_post_init_user(void) {
    midi_register_noteon_callback(&midi_device, note_on_received);
    // Always start in the dark Resolume mode; the RGB mode key steps on to the
    // stock effects from there and wraps back round to it.
    rgb_matrix_mode_noeeprom(RGB_MATRIX_CUSTOM_RESOLUME);
    memset(loop_sounding_note, NOT_SOUNDING, sizeof(loop_sounding_note));
    memset(overdub_note, NOT_SOUNDING, sizeof(overdub_note));
}

void housekeeping_task_user(void) {
    if (loop_state != LOOP_PLAYING) {
        return;
    }
    uint32_t now = timer_read32();
    loop_position += (now - loop_last_tick) * pgm_read_word(&loop_speeds[loop_speed]);
    loop_last_tick = now;

    uint32_t position_ms = loop_position >> 8;
    while (loop_next_event < loop_event_count && loop_events[loop_next_event].time <= position_ms) {
        loop_play_event(loop_events[loop_next_event++]);
    }
    // Every recorded note has its release somewhere in the loop, so notes
    // held across the loop point carry over instead of being cut here.
    if (position_ms >= loop_length) {
        loop_position -= loop_length << 8;
        loop_next_event = 0;
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (keycode >= MIDI_TONE_MIN && keycode <= MIDI_TONE_MAX) {
        // Mirror QMK's own bookkeeping; it sends the live note after we return.
        uint8_t tone = keycode - MIDI_TONE_MIN;
        if (record->event.pressed) {
            bool first = held_count[tone]++ == 0;
            if (first) {
                held_note[tone]    = midi_compute_note(keycode);
                held_channel[tone] = midi_config.channel;
            }
            if (loop_state == LOOP_PLAYING) {
                if (first) {
                    loop_overdub_note(tone, true);
                }
            } else {
                loop_record_note(held_note[tone], held_channel[tone], true);
            }
        } else if (held_count[tone] > 0 && --held_count[tone] == 0) {
            if (loop_state == LOOP_PLAYING) {
                loop_overdub_note(tone, false);
            } else {
                loop_record_note(held_note[tone], held_channel[tone], false);
            }
        }
        return true;
    }

    if (keycode < LP_REC || keycode > KB_LEDOFF) {
        return true;
    }
    if (!record->event.pressed) {
        return false;
    }
    switch (keycode) {
        case LP_REC:
            switch (loop_state) {
                case LOOP_RECORDING:
                    loop_finish_recording(timer_read32());
                    break;
                case LOOP_ARMED: // cancel
                    loop_state = LOOP_IDLE;
                    break;
                case LOOP_PLAYING: // punch in or out of overdub
                    if (loop_overdub) {
                        loop_end_overdub();
                    } else {
                        loop_begin_overdub();
                    }
                    break;
                case LOOP_STOPPED: // start the loop and overdub onto it
                    loop_play_from_top();
                    loop_begin_overdub();
                    break;
                default: // no loop yet: wait for the first note
                    loop_state = LOOP_ARMED;
                    break;
            }
            break;
        case LP_CLR:
            loop_end_overdub();
            loop_silence();
            loop_length = 0;
            loop_state  = LOOP_IDLE;
            break;
        case LP_UNDO:
            if (loop_state == LOOP_PLAYING || loop_state == LOOP_STOPPED) {
                loop_undo();
            }
            break;
        case LP_PLAY:
            if (loop_state == LOOP_PLAYING) {
                loop_end_overdub();
                loop_silence();
                loop_state = LOOP_STOPPED;
            } else if (loop_state == LOOP_STOPPED) {
                loop_play_from_top();
            }
            break;
        case LP_SLOW:
            if (loop_speed > 0) {
                loop_speed--;
            }
            break;
        case LP_FAST:
            if (loop_speed < ARRAY_SIZE(loop_speeds) - 1) {
                loop_speed++;
            }
            break;
        case LP_TRD:
            loop_shift_transpose(-12);
            break;
        case LP_TRU:
            loop_shift_transpose(12);
            break;
        case LP_STD:
            loop_shift_transpose(-1);
            break;
        case LP_STU:
            loop_shift_transpose(1);
            break;
        case KB_PIANO:
            piano_lights = !piano_lights;
            break;
        case KB_LEDOFF: // for colours left behind when the host goes away
            memset(note_color, 0, sizeof(note_color));
            break;
    }
    return false;
}

// Colour for a shift-layer key in the legend shown while shift is held.
// Returns false for keys with no function there.
static bool legend_color(uint16_t keycode, rgb_led_t *color) {
    switch (keycode) {
        case MI_CH1 ... MI_CH16: // the channel in use is white, the rest blue
            *color = (keycode - MI_CH1 == midi_config.channel) ? (rgb_led_t){.r = 255, .g = 255, .b = 255} : (rgb_led_t){.r = 0, .g = 40, .b = 255};
            return true;
        case MI_CHND:
        case MI_CHNU:
            *color = (rgb_led_t){.r = 0, .g = 40, .b = 255};
            return true;
        case MI_OCTD:
        case MI_OCTU:
            *color = (rgb_led_t){.r = 0, .g = 255, .b = 255};
            return true;
        case TG(1): // layout toggle
            *color = (rgb_led_t){.r = 120, .g = 255, .b = 0};
            return true;
        case KB_PIANO:
            *color = (rgb_led_t){.r = 255, .g = 255, .b = 255};
            return true;
        case KB_LEDOFF:
            *color = (rgb_led_t){.r = 255, .g = 0, .b = 40};
            return true;
        case RGB_MOD:
        case RGB_HUI:
        case RGB_VAI:
        case RGB_VAD:
        case RGB_SPI:
        case RGB_SPD:
            *color = (rgb_led_t){.r = 120, .g = 0, .b = 255};
            return true;
        case LP_REC:
            *color = (rgb_led_t){.r = 255, .g = 0, .b = 0};
            return true;
        case LP_PLAY:
            *color = (rgb_led_t){.r = 0, .g = 255, .b = 0};
            return true;
        case LP_CLR:
            *color = (rgb_led_t){.r = 255, .g = 80, .b = 0};
            return true;
        case LP_UNDO:
            *color = (rgb_led_t){.r = 255, .g = 200, .b = 120};
            return true;
        case LP_SLOW:
        case LP_FAST:
            *color = (rgb_led_t){.r = 255, .g = 255, .b = 0};
            return true;
        case LP_TRD:
        case LP_TRU:
            *color = (rgb_led_t){.r = 255, .g = 0, .b = 255};
            return true;
        case LP_STD:
        case LP_STU:
            *color = (rgb_led_t){.r = 255, .g = 60, .b = 120};
            return true;
    }
    return false;
}

bool rgb_matrix_indicators_user(void) {
    // Follow the brightness keys so host colours dim with everything else.
    uint16_t scale       = rgb_matrix_get_val();
    bool     shift_layer = IS_LAYER_ON(2);

    // Piano lighting and the shift legend both replace whatever RGB effect is
    // running.
    if (piano_lights || shift_layer) {
        rgb_matrix_set_color_all(0, 0, 0);
    }

    // Looper state on the bottom-right key: blinking red when armed, solid red
    // while recording or overdubbing, green while the loop plays, amber when
    // it is stopped.
    uint8_t state_led = g_led_config.matrix_co[MATRIX_ROWS - 1][MATRIX_COLS - 1];
    if (state_led != NO_LED) {
        if (loop_state == LOOP_RECORDING || (loop_state == LOOP_PLAYING && loop_overdub) || (loop_state == LOOP_ARMED && (timer_read32() / 250) % 2 == 0)) {
            rgb_matrix_set_color(state_led, scale, 0, 0);
        } else if (loop_state == LOOP_ARMED) {
            rgb_matrix_set_color(state_led, 0, 0, 0);
        } else if (loop_state == LOOP_PLAYING) {
            rgb_matrix_set_color(state_led, 0, scale, 0);
        } else if (loop_state == LOOP_STOPPED) {
            rgb_matrix_set_color(state_led, scale, scale / 3, 0);
        }
    }

    for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
        for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
            uint8_t led = g_led_config.matrix_co[row][col];
            if (led == NO_LED) {
                continue;
            }

            keypos_t pos = {.row = row, .col = col};

            // While shift is held, show only a colour-coded legend of what
            // each key does on that layer.
            if (shift_layer) {
                rgb_led_t color;
                if (legend_color(keymap_key_to_keycode(2, pos), &color)) {
                    rgb_matrix_set_color(led, color.r * scale / 255, color.g * scale / 255, color.b * scale / 255);
                }
                continue;
            }

            uint16_t keycode = keymap_key_to_keycode(layer_switch_get_layer(pos), pos);
            if (keycode < MIDI_TONE_MIN || keycode > MIDI_TONE_MAX) {
                continue;
            }

            uint8_t note = midi_compute_note(keycode);

            // Guide lighting. On the bass guitar layout each column is a fret
            // (the left column is the open strings), so mark it like a neck:
            // dots at frets 3, 5, 7 and 9, and the open strings in amber.
            if (piano_lights && IS_LAYER_ON(1)) {
                if (col == 0) {
                    rgb_matrix_set_color(led, scale, scale / 3, 0);
                } else if (col == 3 || col == 5 || col == 7 || col == 9) {
                    rgb_matrix_set_color(led, scale, scale, scale);
                }
            } else if (piano_lights) {
                // Chromatic layout: natural notes lit, sharps dark, every C
                // tinted so octaves are easy to find.
                uint8_t pitch_class = note % 12;
                bool    is_sharp    = (0x54A >> pitch_class) & 1; // C# D# F# G# A#
                if (pitch_class == 0) {
                    rgb_matrix_set_color(led, 0, scale / 2, scale);
                } else if (!is_sharp) {
                    rgb_matrix_set_color(led, scale, scale, scale);
                }
            }

            uint8_t color = note > 127 ? 0 : note_color[midi_config.channel][note];
            if (color == 0) {
                continue;
            }

            const uint8_t *rgb   = launchpad_palette[color];
            uint16_t       red   = pgm_read_byte(&rgb[0]);
            uint16_t       green = pgm_read_byte(&rgb[1]);
            uint16_t       blue  = pgm_read_byte(&rgb[2]);

            // The palette's dim entries are too faint to see on these LEDs, so
            // lift them to a visible floor, keeping the hue.
            uint16_t peak = MAX(red, MAX(green, blue));
            if (peak > 0 && peak < MIN_VISIBLE_LEVEL) {
                red   = red * MIN_VISIBLE_LEVEL / peak;
                green = green * MIN_VISIBLE_LEVEL / peak;
                blue  = blue * MIN_VISIBLE_LEVEL / peak;
            }
            rgb_matrix_set_color(led, red * scale / 255, green * scale / 255, blue * scale / 255);
        }
    }
    return true;
}
