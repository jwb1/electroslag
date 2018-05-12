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
        enum window_state {
            window_state_unknown = -1,
            window_state_normal,
            window_state_maximized,
            window_state_minimized,
            window_state_full_screen
        };

        struct window_dimensions {
            window_dimensions()
                : x(0)
                , y(0)
                , width(0)
                , height(0)
            {}

            window_dimensions(
                int new_x,
                int new_y,
                int new_width,
                int new_height
                )
                : x(new_x)
                , y(new_y)
                , width(new_width)
                , height(new_height)
            {}

            bool operator ==(const window_dimensions& compare_with) const
            {
                return (
                    x == compare_with.x && y == compare_with.y &&
                    width == compare_with.width && height == compare_with.height
                    );
            }

            bool operator !=(const window_dimensions& compare_with) const
            {
                return (
                    x != compare_with.x || y != compare_with.y ||
                    width != compare_with.width || height != compare_with.height
                    );
            }

            int x;
            int y;
            int width;
            int height;
        };

        struct window_initialize_params {
            window_initialize_params()
                : state(window_state_normal)
            {}

            window_state state;
            window_dimensions dimensions;
            std::string title;
        };

        class window_interface {
        public:
            virtual int run() = 0;

            virtual window_state get_state() const = 0;
            virtual std::string get_title() const = 0;
            virtual window_dimensions const* get_dimensions() const = 0;
            virtual bool get_paused() const = 0;
            virtual int get_frame_time() const = 0;

            virtual void set_state(window_state new_state) = 0;
            virtual void set_title(std::string const& new_title) = 0;
            virtual void set_dimensions(window_dimensions const* new_dimensions) = 0;
            virtual void set_paused(bool new_paused) = 0;

            // TODO: Thread safety for these events?
            typedef event<void> destroyed_event;
            typedef destroyed_event::bound_delegate destroyed_delegate;
            mutable destroyed_event destroyed;

            typedef event<void, window_state> state_changed_event;
            typedef state_changed_event::bound_delegate state_changed_delegate;
            mutable state_changed_event state_changed;

            typedef event<void, window_dimensions const*> dimensions_changed_event;
            typedef dimensions_changed_event::bound_delegate dimensions_changed_delegate;

            typedef dimensions_changed_delegate size_changed_delegate;
            mutable dimensions_changed_event size_changed;

            typedef dimensions_changed_delegate position_changed_delegate;
            mutable dimensions_changed_event position_changed;

            typedef event<void, bool> paused_changed_event;
            typedef paused_changed_event::bound_delegate paused_changed_delegate;
            mutable paused_changed_event paused_changed;

            typedef event<void, int> frame_event;
            typedef frame_event::bound_delegate frame_delegate;
            mutable frame_event frame;
        };
    }
}
