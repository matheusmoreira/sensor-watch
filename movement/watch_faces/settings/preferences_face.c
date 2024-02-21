/* SPDX-License-Identifier: MIT */

/*
 * MIT License
 *
 * Copyright © 2021-2023 Joey Castillo <joeycastillo@utexas.edu> <jose.castillo@gmail.com>
 * Copyright © 2024 Max Zettlmeißl <max@zettlmeissl.de>
 * Copyright © 2024 Matheus Afonso Martins Moreira <matheus.a.m.moreira@gmail.com> (https://www.matheusmoreira.com/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include "preferences_face.h"
#include "watch.h"

#define PREFERENCES_FACE_NUM_PREFEFENCES (7)
#define PREFERENCES_FACE_PAGE_CL 0
const char preferences_face_titles[PREFERENCES_FACE_NUM_PREFEFENCES][11] = {
    "CL        ",   // Clock: 12 or 24 hour
    "BT  Beep  ",   // Buttons: should they beep?
    "TO        ",   // Timeout: how long before we snap back to the clock face?
    "LE        ",   // Low Energy mode: how long before it engages?
    "LT        ",   // Light: duration
#ifdef WATCH_IS_BLUE_BOARD
    "LT   blu  ",   // Light: blue component (for watches with blue LED)
#else
    "LT   grn  ",   // Light: green component
#endif
    "LT   red  ",   // Light: red component
};

static void _preferences_face_next_page(uint8_t *current_page) {
    *current_page = (*current_page + 1) % PREFERENCES_FACE_NUM_PREFEFENCES;
}

static void _preferences_face_skip_CL_page_if_forced_24h(uint8_t *current_page) {
#if MOVEMENT_FORCE_24H
    if (*current_page == PREFERENCES_FACE_PAGE_CL) { _preferences_face_next_page(current_page); }
#endif
}

void preferences_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) *context_ptr = malloc(sizeof(uint8_t));
}

void preferences_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    uint8_t *current_page = context;
    *current_page = 0;
    _preferences_face_skip_CL_page_if_forced_24h(current_page);
    movement_request_tick_frequency(4); // we need to manually blink some pixels
}

bool preferences_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    uint8_t current_page = *((uint8_t *)context);
    switch (event.event_type) {
        case EVENT_TICK:
        case EVENT_ACTIVATE:
            // Do nothing; handled below.
            break;
        case EVENT_MODE_BUTTON_UP:
            watch_set_led_off();
            movement_move_to_next_face();
            return false;
        case EVENT_LIGHT_BUTTON_DOWN:
            _preferences_face_next_page(&current_page);
            _preferences_face_skip_CL_page_if_forced_24h(&current_page);
            *((uint8_t *)context) = current_page;
            break;
        case EVENT_ALARM_BUTTON_UP:
            switch (current_page) {
                case 0:
                    settings->bit.clock_mode_24h = !(settings->bit.clock_mode_24h);
                    break;
                case 1:
                    settings->bit.button_should_sound = !(settings->bit.button_should_sound);
                    break;
                case 2:
                    settings->bit.to_interval = settings->bit.to_interval + 1;
                    break;
                case 3:
                    settings->bit.le_interval = settings->bit.le_interval + 1;
                    break;
                case 4:
                    settings->bit.led_duration = settings->bit.led_duration + 1;
                    break;
                case 5:
                    settings->bit.led_green_color = settings->bit.led_green_color + 1;
                    break;
                case 6:
                    settings->bit.led_red_color = settings->bit.led_red_color + 1;
                    break;
            }
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    watch_display_string((char *)preferences_face_titles[current_page], 0);

    // blink active setting on even-numbered quarter-seconds
    if (event.subsecond % 2) {
        char buf[8];
        switch (current_page) {
            case 0:
                if (settings->bit.clock_mode_24h) watch_display_string("24h", 4);
                else watch_display_string("12h", 4);
                break;
            case 1:
                if (settings->bit.button_should_sound) watch_display_string("y", 9);
                else watch_display_string("n", 9);
                break;
            case 2:
                switch (settings->bit.to_interval) {
                    case 0:
                        watch_display_string("60 SeC", 4);
                        break;
                    case 1:
                        watch_display_string("2 n&in", 4);
                        break;
                    case 2:
                        watch_display_string("5 n&in", 4);
                        break;
                    case 3:
                        watch_display_string("30n&in", 4);
                        break;
                }
                break;
            case 3:
                switch (settings->bit.le_interval) {
                    case 0:
                        watch_display_string(" Never", 4);
                        break;
                    case 1:
                        watch_display_string("10n&in", 4);
                        break;
                    case 2:
                        watch_display_string("1 hour", 4);
                        break;
                    case 3:
                        watch_display_string("2 hour", 4);
                        break;
                    case 4:
                        watch_display_string("6 hour", 4);
                        break;
                    case 5:
                        watch_display_string("12 hr", 4);
                        break;
                    case 6:
                        watch_display_string(" 1 day", 4);
                        break;
                    case 7:
                        watch_display_string(" 7 day", 4);
                        break;
                }
                break;
            case 4:
                if (settings->bit.led_duration) {
                    sprintf(buf, " %1d SeC", settings->bit.led_duration * 2 - 1);
                    watch_display_string(buf, 4);
                } else {
                    watch_display_string("no LEd", 4);
                }
                break;
            case 5:
                sprintf(buf, "%2d", settings->bit.led_green_color);
                watch_display_string(buf, 8);
                break;
            case 6:
                sprintf(buf, "%2d", settings->bit.led_red_color);
                watch_display_string(buf, 8);
                break;
        }
    }

    // on LED color select screns, preview the color.
    if (current_page >= 5) {
        watch_set_led_color(settings->bit.led_red_color ? (0xF | settings->bit.led_red_color << 4) : 0,
                            settings->bit.led_green_color ? (0xF | settings->bit.led_green_color << 4) : 0);
        // return false so the watch stays awake (needed for the PWM driver to function).
        return false;
    }

    watch_set_led_off();
    return true;
}

void preferences_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    watch_set_led_off();
    watch_store_backup_data(settings->reg, 0);
}
