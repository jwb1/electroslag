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
#include "electroslag/serialize/load_record.hpp"
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/ui/window_interface.hpp"

namespace electroslag {
    namespace application {
        class loading_screen {
        public:
            loading_screen();
            ~loading_screen();

            void shutdown();

#if !defined(ELECTROSLAG_BUILD_SHIP)
            void load(bool dump_content);
            void save(std::string const& file_name);
#else
            void load();
#endif

            void show();
            void hide();

        private:
            class draw_command : public graphics::command {
            public:
                explicit draw_command(loading_screen* this_loading_screen)
                    : m_this(this_loading_screen)
                {}

                virtual void execute(graphics::context_interface* context);

            private:
                loading_screen* m_this;

                // Disallowed operations:
                explicit draw_command(draw_command const&);
                draw_command& operator =(draw_command const&);
            };

            void on_window_frame(int millisec_elapsed);

            // Load record.
            serialize::load_record::ref m_loaded_objects;

            // Graphics objects.
            graphics::command_queue_interface::ref m_draw_queue;
            graphics::primitive_stream_interface::ref m_prim_stream;
            graphics::shader_program_interface::ref m_shader;
            graphics::texture_interface::ref m_texture;
            graphics::buffer_interface::ref m_uniform_buffer;

            int m_uniform_buffer_binding;

            // Showing or not?
            ui::window_interface::frame_delegate* m_frame_delegate;
            std::atomic<bool> m_visible;
        };
    }
}
