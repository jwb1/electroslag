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
#include "electroslag/threading/mutex.hpp"
#include "electroslag/ui/window_interface.hpp"

namespace electroslag {
    namespace ui {
        class window_win32 : public window_interface {
        public:
            window_win32();

            void initialize(window_initialize_params const* params);
            void shutdown();

            // Implement window_interface
            virtual int run();

            virtual window_state get_state() const;
            virtual std::string get_title() const;
            virtual window_dimensions const* get_dimensions() const;
            virtual bool get_paused() const;
            virtual int get_frame_time() const;

            virtual void set_state(window_state new_state);
            virtual void set_title(std::string const& new_title);
            virtual void set_dimensions(window_dimensions const* new_dimensions);
            virtual void set_paused(bool new_paused);

            // Windows specific
            HWND get_hwnd() const;
            bool get_internal_paused() const;
            bool get_overall_paused() const;

        private:
            union paused_change_lparam {
                paused_change_lparam()
                    : lparam(0) // Sets all flags to false.
                {}

                paused_change_lparam(LPARAM new_lparam)
                    : lparam(new_lparam)
                {}

                LPARAM lparam;
                struct pause_change_flags {
                    bool paused_changed:1;
                    bool new_paused:1;
                    bool internal_paused_changed:1;
                    bool new_internal_paused:1;
                    bool new_overall_paused:1;
                } flags;
                ELECTROSLAG_STATIC_CHECK(sizeof(pause_change_flags) == 1, "Bit packing check");
            };

            static char const* const window_class_name;
            static window_dimensions const default_dimensions;

            static char const* const default_title;

            // Currently hardcoded to 30fps.
            static int const total_frame_time = 33;
            // If there is less than this many milliseconds in a frame, just proceed to the next.
            static int const pad_frame_time = 3;

            // The internal pause flag may be read externally, but not changed.
            void set_internal_paused(bool new_internal_paused);

            void set_state_enter_full_screen();
            void set_state_exit_full_screen(window_state new_state);
            void signal_full_screen_change(window_state new_state);

            int run_paused(bool* out_run_loop, bool* out_paused);
            int run_not_paused(bool* out_run_loop, bool* out_paused);

            int state_to_cmd_show(window_state state);

            RECT dimensions_to_window_rect(window_dimensions const* dimensions);

            static LRESULT CALLBACK window_proc(
                HWND hwnd,
                unsigned int message,
                WPARAM wparam,
                LPARAM lparam
                );

            LRESULT handle_message(
                unsigned int message,
                WPARAM wparam,
                LPARAM lparam
                );

            LRESULT on_msg_destroy(WPARAM wparam, LPARAM lparam);
            LRESULT on_msg_size(WPARAM wparam, LPARAM lparam);
            LRESULT on_msg_move(WPARAM wparam, LPARAM lparam);
            LRESULT on_msg_settext(WPARAM wparam, LPARAM lparam);
            LRESULT on_msg_activate(WPARAM wparam, LPARAM lparam);
            LRESULT on_msg_enter_size_move(WPARAM wparam, LPARAM lparam);
            LRESULT on_msg_exit_size_move(WPARAM wparam, LPARAM lparam);
            LRESULT on_msg_paint(WPARAM wparam, LPARAM lparam);
            LRESULT on_msg_erase_background(WPARAM wparam, LPARAM lparam);

            void handle_state_change(window_state new_state);
            void handle_resize(int new_width, int new_height);
            void handle_reposition(int new_x, int new_y);
            void handle_paused(bool new_paused, bool new_internal_paused);
            void handle_frame(int milliseconds_elapsed);

            mutable threading::mutex m_mutex;

            window_state m_state;

            window_dimensions m_dimensions;
            WINDOWPLACEMENT m_window_placement;

            window_dimensions m_full_screen_change_dimensions;

            std::string m_title;

            HWND m_hwnd;
            std::exception_ptr m_exception;

            struct window_flags {
                window_flags()
                    : paused(false)
                    , internal_paused(true) // Will be un-paused in initialize
                    , entering_full_screen(false)
                    , exiting_full_screen(false)
                    , restore_full_screen(false)
                    , registered_class(false)
                {}

                bool paused:1;
                bool internal_paused:1;
                bool entering_full_screen:1;
                bool exiting_full_screen:1;
                bool restore_full_screen:1;
                bool registered_class:1;
            } m_flags;
            ELECTROSLAG_STATIC_CHECK(sizeof(window_flags) == 1, "Bit packing check");

            // Disallowed operations:
            explicit window_win32(window_win32 const&);
            window_win32& operator =(window_win32 const&);
        };
    }
}
