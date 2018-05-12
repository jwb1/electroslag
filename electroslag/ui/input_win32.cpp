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

#include "electroslag/precomp.hpp"
#include "electroslag/ui/input_win32.hpp"
#include "electroslag/ui/ui_win32.hpp"

namespace electroslag {
    namespace ui {
        void input_win32::capture_mouse()
        {
            // Can't manipulate the raw input state directly as this may be
            // called as a result of a mouse button click.
            HWND hwnd = get_ui_win32()->get_win32_window()->get_hwnd();
            PostMessage(hwnd, WM_ELECTROSLAG_CAPTURE_MOUSE, 0, 0);
        }

        void input_win32::release_mouse()
        {
            HWND hwnd = get_ui_win32()->get_win32_window()->get_hwnd();
            PostMessage(hwnd, WM_ELECTROSLAG_RELEASE_MOUSE, 0, 0);
        }

        bool input_win32::handle_message(unsigned int message, WPARAM wparam, LPARAM lparam, LRESULT& out_result)
        {
            switch (message) {
            // Keyboard message handling.
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
                if (!(lparam & 0x40000000)) { // Ignore auto-repeat messages
                    key_down.signal(params_to_key_code(wparam, lparam));
                    out_result = 0;
                }
                return (true);

            case WM_SYSKEYUP:
            case WM_KEYUP:
                key_up.signal(params_to_key_code(wparam, lparam));
                out_result = 0;
                return (true);

            case WM_CHAR:
                on_msg_char(wparam, lparam);
                out_result = 0;
                return (true);

            // Mouse message handling; expecting these to be called when the mouse is not captured.
            case WM_LBUTTONDOWN:
                mouse_button_down.signal(mouse_button_left, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
                out_result = 0;
                return (true);

            case WM_RBUTTONDOWN:
                mouse_button_down.signal(mouse_button_right, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
                out_result = 0;
                return (true);

            case WM_MBUTTONDOWN:
                mouse_button_down.signal(mouse_button_center, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
                out_result = 0;
                return (true);

            case WM_XBUTTONDOWN:
                if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1) {
                    mouse_button_down.signal(mouse_button_4, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
                    out_result = TRUE;
                    return (true);
                }
                else if (GET_XBUTTON_WPARAM(wparam) == XBUTTON2) {
                    mouse_button_down.signal(mouse_button_5, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
                    out_result = TRUE;
                    return (true);
                }
                break;

            case WM_LBUTTONUP:
                mouse_button_up.signal(mouse_button_left, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
                out_result = 0;
                return (true);

            case WM_RBUTTONUP:
                mouse_button_up.signal(mouse_button_right, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
                out_result = 0;
                return (true);

            case WM_MBUTTONUP:
                mouse_button_up.signal(mouse_button_center, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
                out_result = 0;
                return (true);

            case WM_XBUTTONUP:
                if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1) {
                    mouse_button_up.signal(mouse_button_4, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
                    out_result = TRUE;
                    return (true);
                }
                else if (GET_XBUTTON_WPARAM(wparam) == XBUTTON2) {
                    mouse_button_up.signal(mouse_button_5, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
                    out_result = TRUE;
                    return (true);
                }
                break;

            case WM_ELECTROSLAG_CAPTURE_MOUSE:
                on_msg_capture_mouse(wparam, lparam);
                out_result = TRUE;
                return (true);

            case WM_ELECTROSLAG_RELEASE_MOUSE:
                on_msg_release_mouse(wparam, lparam);
                out_result = TRUE;
                return (true);

            // "Raw" input message handling; expecting these to be called when the mouse is captured.
            case WM_INPUT:
                on_msg_raw_input(wparam, lparam);
                out_result = 0;
                return (true);
            }

            return (false);
        }

        // static
        key_code input_win32::params_to_key_code(WPARAM wparam, LPARAM lparam)
        {
            if (wparam == VK_CONTROL && (lparam & lparam_extended_key)) {
                wparam = key_code_control_right;
            }
            else if (wparam == VK_MENU && (lparam & lparam_extended_key)) {
                wparam = key_code_alt_right;
            }
            else if (wparam == VK_SHIFT && GetKeyState(VK_RSHIFT)) {
                wparam = key_code_shift_right;
            }
            return (static_cast<key_code>(wparam));
        }

        void input_win32::on_msg_char(WPARAM wparam, LPARAM)
        {
            char* key_char = static_cast<char*>(alloca(MB_CUR_MAX));
            key_char[0] = '0';
            if (wctomb(key_char, static_cast<wchar_t>(wparam)) == 1) {
                char_in.signal(key_char[0]);
            }
        }

        void input_win32::on_msg_capture_mouse(WPARAM, LPARAM)
        {
            HWND hwnd = get_ui_win32()->get_win32_window()->get_hwnd();

            // Hide the cursor; we are going to capture the mouse input directly.
            ShowCursor(FALSE);

            // Set the OS level capture for this thread.
            SetCapture(hwnd);

            // Register for "raw" input from the mouse.
            RAWINPUTDEVICE rid[1];
            rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
            rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
            rid[0].dwFlags = RIDEV_NOLEGACY;
            rid[0].hwndTarget = hwnd;

            if (!RegisterRawInputDevices(rid, _countof(rid), sizeof(rid[0]))) {
                throw win32_api_failure("RegisterRawInputDevices");
            }
        }

        void input_win32::on_msg_release_mouse(WPARAM, LPARAM)
        {
            // Unregister raw input
            RAWINPUTDEVICE rid[1];
            rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
            rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
            rid[0].dwFlags = RIDEV_REMOVE;
            rid[0].hwndTarget = 0;

            if (!RegisterRawInputDevices(rid, _countof(rid), sizeof(rid[0]))) {
                throw win32_api_failure("RegisterRawInputDevices");
            }

            // Release the OS capture.
            ReleaseCapture();

            // Done with mouse capture; replace the mouse cursor in the middle of
            // the window client area.
            ui::window_dimensions const* dimensions = get_ui_win32()->get_win32_window()->get_dimensions();
            SetCursorPos(dimensions->x + (dimensions->width / 2), dimensions->y + (dimensions->height / 2));

            ShowCursor(TRUE);
        }

        void input_win32::on_msg_raw_input(WPARAM, LPARAM lparam)
        {
            // Figure out how much data there is.
            unsigned int input_data_size = 0;
            if (GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lparam),
                RID_INPUT,
                0,
                &input_data_size,
                sizeof(RAWINPUTHEADER)
                ) < 0) {
                throw win32_api_failure("GetRawInputData");
            }

            // Read the input data.
            byte* input_data = static_cast<byte*>(alloca(input_data_size));
            if (GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lparam),
                RID_INPUT,
                input_data,
                &input_data_size,
                sizeof(RAWINPUTHEADER)
                ) != input_data_size) {
                throw win32_api_failure("GetRawInputData");
            }

            // Parse the input data and signal the appropriate events.
            RAWINPUT* raw_input = reinterpret_cast<RAWINPUT*>(input_data);
            if (raw_input->header.dwType == RIM_TYPEMOUSE) {
                RAWMOUSE const* mouse = &raw_input->data.mouse;

                unsigned int mouse_button_flags = 0;
                if (mouse->usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) {
                    mouse_button_flags |= mouse_button_bits_1_down;
                }
                if (mouse->usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) {
                    mouse_button_flags |= mouse_button_bits_2_down;
                }
                if (mouse->usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) {
                    mouse_button_flags |= mouse_button_bits_3_down;
                }
                if (mouse->usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) {
                    mouse_button_flags |= mouse_button_bits_4_down;
                }
                if (mouse->usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) {
                    mouse_button_flags |= mouse_button_bits_5_down;
                }
                if (mouse->usButtonFlags & RI_MOUSE_BUTTON_1_UP) {
                    mouse_button_flags |= mouse_button_bits_1_up;
                }
                if (mouse->usButtonFlags & RI_MOUSE_BUTTON_2_UP) {
                    mouse_button_flags |= mouse_button_bits_2_up;
                }
                if (mouse->usButtonFlags & RI_MOUSE_BUTTON_3_UP) {
                    mouse_button_flags |= mouse_button_bits_3_up;
                }
                if (mouse->usButtonFlags & RI_MOUSE_BUTTON_4_UP) {
                    mouse_button_flags |= mouse_button_bits_4_up;
                }
                if (mouse->usButtonFlags & RI_MOUSE_BUTTON_5_UP) {
                    mouse_button_flags |= mouse_button_bits_5_up;
                }

                ELECTROSLAG_CHECK(mouse->usFlags == MOUSE_MOVE_RELATIVE);
                mouse_input.signal(mouse->lLastX, mouse->lLastY, mouse_button_flags);
            }
            else {
                throw std::runtime_error("Invalid raw input data");
            }
        }
    }
}
