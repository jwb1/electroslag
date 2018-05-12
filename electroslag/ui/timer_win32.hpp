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
#include "electroslag/ui/timer_interface.hpp"
#include "electroslag/ui/window_win32.hpp"

namespace electroslag {
    namespace ui {
        class timer_win32 : public timer_interface {
        public:
            timer_win32()
                : m_first_reading(timeGetTime())
                , m_high_precision(false)
            {}
            virtual ~timer_win32()
            {
                shutdown();
            }

            void initialize(window_win32* window);
            void shutdown();

            // Implement timer_interface
            virtual unsigned int read_milliseconds() const;

        private:
            void on_window_state_changed(window_state new_state);

            unsigned int m_first_reading;
            bool m_high_precision;

            // Disallowed operations:
            explicit timer_win32(timer_win32 const&);
            timer_win32& operator =(timer_win32 const&);
        };
    }
}
