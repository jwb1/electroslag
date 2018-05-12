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
#include "electroslag/ui/timer_win32.hpp"

namespace electroslag {
    namespace ui {
        void timer_win32::initialize(window_win32* window)
        {
            ELECTROSLAG_CHECK(!m_high_precision);

            window->state_changed.bind(
                window_interface::state_changed_delegate::create_from_method<timer_win32, &timer_win32::on_window_state_changed>(this),
                event_bind_mode_own_listener
                );
        }

        void timer_win32::shutdown()
        {
            if (m_high_precision) {
                // Probably don't care if this one fails.
                timeEndPeriod(1);
                m_high_precision = false;
            }
        }

        unsigned int timer_win32::read_milliseconds() const
        {
            ELECTROSLAG_CHECK(m_first_reading > 0);
            return (timeGetTime() - m_first_reading);
        }

        void timer_win32::on_window_state_changed(window_state new_state)
        {
            if (new_state != window_state_minimized && !m_high_precision) {
                ELECTROSLAG_CHECK(
                    new_state == window_state_full_screen ||
                    new_state == window_state_maximized ||
                    new_state == window_state_normal
                    );
                if (timeBeginPeriod(1) == TIMERR_NOERROR) {
                    m_high_precision = true;
                }
                else {
                    throw win32_api_failure("timeBeginPeriod");
                }
            }
            else if (new_state == window_state_minimized && m_high_precision) {
                if (timeEndPeriod(1) == TIMERR_NOERROR) {
                    m_high_precision = false;
                }
                else {
                    throw win32_api_failure("timeEndPeriod");
                }
            }
        }
    }
}
