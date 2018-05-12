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
#include "electroslag/ui/ui_internal_interface.hpp"
#include "electroslag/ui/window_win32.hpp"
#include "electroslag/ui/input_win32.hpp"
#include "electroslag/ui/timer_win32.hpp"

namespace electroslag {
    namespace ui {
        // Custom windows messages.
        static unsigned int const WM_ELECTROSLAG_PAUSED_CHANGE = (WM_APP + 1);
        static unsigned int const WM_ELECTROSLAG_CAPTURE_MOUSE = (WM_APP + 2);
        static unsigned int const WM_ELECTROSLAG_RELEASE_MOUSE = (WM_APP + 3);

        class ui_win32 : public ui_internal_interface {
        public:
            ui_win32();

            // Implement ui_interface
            virtual void initialize(window_initialize_params const* window_params);
            virtual void shutdown();

            virtual bool is_initialized() const;

            virtual window_interface* get_window()
            {
                return (&m_window);
            }

            virtual timer_interface* get_timer()
            {
                return (&m_timer);
            }

            virtual input_interface* get_input()
            {
                return (&m_input);
            }

            // Implement ui_internal_interface
            virtual std::string get_program_path() const;

            // Internal methods
            window_win32* get_win32_window()
            {
                return (&m_window);
            }

            timer_win32* get_win32_timer()
            {
                return (&m_timer);
            }

            input_win32* get_win32_input()
            {
                return (&m_input);
            }

            HINSTANCE get_hinstance() const
            {
                ELECTROSLAG_CHECK(m_hinstance);
                return (m_hinstance);
            }

        private:
            static void check_operating_system();
            static void check_processor();

            void platform_initialize();

            window_win32 m_window;
            timer_win32 m_timer;
            input_win32 m_input;

            mutable threading::mutex m_initialization_mutex;
            bool m_initialized;

            HINSTANCE m_hinstance;

            // Disallowed operations:
            explicit ui_win32(ui_win32 const&);
            ui_win32& operator =(ui_win32 const&);
        };

        ui_win32* get_ui_win32();
    }
}
