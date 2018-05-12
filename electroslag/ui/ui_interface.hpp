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
#include "electroslag/ui/window_interface.hpp"
#include "electroslag/ui/timer_interface.hpp"
#include "electroslag/ui/input_interface.hpp"

namespace electroslag {
    namespace ui {
        class ui_interface {
        public:
            virtual void initialize(window_initialize_params const* window_params) = 0;
            virtual void shutdown() = 0;

            virtual bool is_initialized() const = 0;

            void check_not_initialized() const
            {
                ELECTROSLAG_CHECK(!is_initialized());
            }

            void check_initialized() const
            {
                ELECTROSLAG_CHECK(is_initialized());
            }

            virtual window_interface* get_window() = 0;
            virtual timer_interface* get_timer() = 0;
            virtual input_interface* get_input() = 0;
        };

        ui_interface* get_ui();
    }
}
