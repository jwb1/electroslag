//  Electroslag Interactive Graphics System
//  Copyright 2018 Joshua Buckman
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#pragma once
#include "electroslag/event.hpp"

namespace electroslag {
    namespace ui {
        enum key_code {
            key_code_unknown = -1,

            key_code_backspace = 0x08,
            key_code_tab = 0x09,
            key_code_return = 0x0D,

            key_code_shift = 0x10,
            key_code_control = 0x11,
            key_code_alt = 0x12,
            key_code_pause = 0x13,
            key_code_caps_lock = 0x14,

            key_code_escape = 0x1B,
            key_code_space = 0x20,
            key_code_page_up = 0x21,
            key_code_page_down = 0x22,
            key_code_end = 0x23,
            key_code_home = 0x24,
            key_code_left_arrow = 0x25,
            key_code_up_arrow = 0x26,
            key_code_right_arrow = 0x27,
            key_code_down_arrow = 0x28,
            key_code_print_screen = 0x2C,
            key_code_insert = 0x2D,
            key_code_delete = 0x2E,

            key_code_0 = 0x30,
            key_code_1 = 0x31,
            key_code_2 = 0x32,
            key_code_3 = 0x33,
            key_code_4 = 0x34,
            key_code_5 = 0x35,
            key_code_6 = 0x36,
            key_code_7 = 0x37,
            key_code_8 = 0x38,
            key_code_9 = 0x39,

            key_code_a = 0x41,
            key_code_b = 0x42,
            key_code_c = 0x43,
            key_code_d = 0x44,
            key_code_e = 0x45,
            key_code_f = 0x46,
            key_code_g = 0x47,
            key_code_h = 0x48,
            key_code_i = 0x49,
            key_code_j = 0x4A,
            key_code_k = 0x4B,
            key_code_l = 0x4C,
            key_code_m = 0x4D,
            key_code_n = 0x4E,
            key_code_o = 0x4F,
            key_code_p = 0x50,
            key_code_q = 0x51,
            key_code_r = 0x52,
            key_code_s = 0x53,
            key_code_t = 0x54,
            key_code_u = 0x55,
            key_code_v = 0x56,
            key_code_w = 0x57,
            key_code_x = 0x58,
            key_code_y = 0x59,
            key_code_z = 0x5A,

            key_code_left_window = 0x5B,
            key_code_right_window = 0x5C,
            key_code_app_menu = 0x5D,

            key_code_numpad_0 = 0x60,
            key_code_numpad_1 = 0x61,
            key_code_numpad_2 = 0x62,
            key_code_numpad_3 = 0x63,
            key_code_numpad_4 = 0x64,
            key_code_numpad_5 = 0x65,
            key_code_numpad_6 = 0x66,
            key_code_numpad_7 = 0x67,
            key_code_numpad_8 = 0x68,
            key_code_numpad_9 = 0x69,
            key_code_numpad_multiply = 0x6A,
            key_code_numpad_add = 0x6B,
            key_code_numpad_subtract = 0x6D,
            key_code_numpad_decimal = 0x6E,
            key_code_numpad_divide = 0x6F,
            key_code_f1 = 0x70,
            key_code_f2 = 0x71,
            key_code_f3 = 0x72,
            key_code_f4 = 0x73,
            key_code_f5 = 0x74,
            key_code_f6 = 0x75,
            key_code_f7 = 0x76,
            key_code_f8 = 0x77,
            key_code_f9 = 0x78,
            key_code_f10 = 0x79,
            key_code_f11 = 0x7A,
            key_code_f12 = 0x7B,
            key_code_f13 = 0x7C,
            key_code_f14 = 0x7D,
            key_code_f15 = 0x7E,
            key_code_f16 = 0x7F,
            key_code_f17 = 0x80,
            key_code_f18 = 0x81,
            key_code_f19 = 0x82,
            key_code_f20 = 0x83,
            key_code_f21 = 0x84,
            key_code_f22 = 0x85,
            key_code_f23 = 0x86,
            key_code_f24 = 0x87,

            key_code_numpad_lock = 0x90,
            key_code_scroll_lock = 0x91,

            key_code_browser_back = 0xA6,
            key_code_browser_forward = 0xA7,
            key_code_browser_refresh = 0xA8,
            key_code_browser_stop = 0xA9,
            key_code_browser_search = 0xAA,
            key_code_browser_favorites = 0xAB,
            key_code_browser_home = 0xAC,

            key_code_volume_mute = 0xAD,
            key_code_volume_down = 0xAE,
            key_code_volume_up = 0xAF,
            key_code_media_next_track = 0xB0,
            key_code_media_prev_track = 0xB1,
            key_code_media_stop = 0xB2,
            key_code_media_play_pause = 0xB3,
            key_code_launch_mail = 0xB4,
            key_code_launch_media_select = 0xB5,
            key_code_launch_app1 = 0xB6,
            key_code_launch_app2 = 0xB7,

            key_code_oem_1 = 0xBA,   // ',:' for US
            key_code_plus = 0xBB,
            key_code_comma = 0xBC,
            key_code_minus = 0xBD,
            key_code_period = 0xBE,
            key_code_oem_2 = 0xBF,   // '/?' for US
            key_code_oem_3 = 0xC0,   // '`~' for US
            key_code_oem_4 = 0xDB,  //  '[{' for US
            key_code_oem_5 = 0xDC,  //  '\|' for US
            key_code_oem_6 = 0xDD,  //  ']}' for US
            key_code_oem_7 = 0xDE,  //  ''"' for US
            key_code_oem_8 = 0xDF,

            key_code_shift_right = 0xA1,
            key_code_control_right = 0xA3,
            key_code_alt_right = 0xA5
        };

        enum mouse_button {
            mouse_button_unknown = -1,
            mouse_button_none = 0,
            mouse_button_1 = 1,
            mouse_button_left = mouse_button_1,
            mouse_button_2 = 2,
            mouse_button_right = mouse_button_2,
            mouse_button_3 = 3,
            mouse_button_center = mouse_button_3,
            mouse_button_4 = 4,
            mouse_button_5 = 5
        };

        enum mouse_button_bits {
            mouse_button_bits_1_down = (1 << mouse_button_1),
            mouse_button_bits_left_down = mouse_button_bits_1_down,
            mouse_button_bits_2_down = (1 << mouse_button_2),
            mouse_button_bits_right_down = mouse_button_bits_2_down,
            mouse_button_bits_3_down = (1 << mouse_button_3),
            mouse_button_bits_center_down = mouse_button_bits_3_down,
            mouse_button_bits_4_down = (1 << mouse_button_4),
            mouse_button_bits_5_down = (1 << mouse_button_5),

            mouse_button_bits_up_bit = 0x80,
            mouse_button_bits_1_up = (1 << mouse_button_1) | mouse_button_bits_up_bit,
            mouse_button_bits_left_up = mouse_button_bits_1_up,
            mouse_button_bits_2_up = (1 << mouse_button_2) | mouse_button_bits_up_bit,
            mouse_button_bits_right_up = mouse_button_bits_2_up,
            mouse_button_bits_3_up = (1 << mouse_button_3) | mouse_button_bits_up_bit,
            mouse_button_bits_center_up = mouse_button_bits_3_up,
            mouse_button_bits_4_up = (1 << mouse_button_4) | mouse_button_bits_up_bit,
            mouse_button_bits_5_up = (1 << mouse_button_5) | mouse_button_bits_up_bit
        };

        class input_interface {
        public:
            // TODO: Thread safety for these events?

            // Keyboard input; raw key up and down events and character input.
            typedef event<void, key_code> key_down_event;
            typedef key_down_event::bound_delegate key_down_delegate;
            mutable key_down_event key_down;

            typedef event<void, key_code> key_up_event;
            typedef key_up_event::bound_delegate key_up_delegate;
            mutable key_up_event key_up;

            typedef event<void, char> char_in_event;
            typedef char_in_event::bound_delegate char_in_delegate;
            mutable char_in_event char_in;

            // Mouse input:
            // - When the mouse is "released" only the button up and down events are fired.
            // - When the mouse is "captured" only the mouse input event is fired.
            virtual void capture_mouse() = 0;
            virtual void release_mouse() = 0;

            // Coordinates of a button click are in window client area relative pixels.
            typedef event<void, mouse_button, int, int> mouse_button_down_event;
            typedef mouse_button_down_event::bound_delegate mouse_button_down_delegate;
            mutable mouse_button_down_event mouse_button_down;

            typedef event<void, mouse_button, int, int> mouse_button_up_event;
            typedef mouse_button_up_event::bound_delegate mouse_button_up_delegate;
            mutable mouse_button_up_event mouse_button_up;

            // Coordinates of mouse input is reported in delta of OS units (not necessarily pixels.)
            // The button bits report change in burtton state.
            typedef event<void, int, int, unsigned int> mouse_input_event;
            typedef mouse_input_event::bound_delegate mouse_input_delegate;
            mutable mouse_input_event mouse_input;
        };
    }
}
