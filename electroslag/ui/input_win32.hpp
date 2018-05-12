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
#include "electroslag/ui/input_interface.hpp"

namespace electroslag {
    namespace ui {
        class input_win32 : public input_interface {
        public:
            input_win32()
            {}
            virtual ~input_win32()
            {}

            // Implement input_interface
            virtual void capture_mouse();
            virtual void release_mouse();

            // input_win32 methods
            bool handle_message(
                unsigned int message,
                WPARAM wparam,
                LPARAM lparam,
                LRESULT& out_result
                );

        private:
            static LPARAM const lparam_extended_key = (1 << 24);

            static key_code params_to_key_code(WPARAM wparam, LPARAM lparam);

            void on_msg_char(WPARAM wparam, LPARAM lparam);
            void on_msg_capture_mouse(WPARAM wparam, LPARAM lparam);
            void on_msg_release_mouse(WPARAM wparam, LPARAM lparam);
            void on_msg_raw_input(WPARAM wparam, LPARAM lparam);

            // Disallowed operations:
            explicit input_win32(input_win32 const&);
            input_win32& operator =(input_win32 const&);
        };
    }
}
