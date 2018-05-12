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
#include "electroslag/application/loading_screen.hpp"
#include "electroslag/resource.hpp"
#include "electroslag/resource_id.hpp"
#include "electroslag/serialize/database.hpp"
#include "electroslag/ui/ui_interface.hpp"
#include "electroslag/graphics/context_interface.hpp"
#if !defined(ELECTROSLAG_BUILD_SHIP)
#include "electroslag/texture/gli_importer.hpp"
#endif

namespace electroslag {
    namespace application {
        loading_screen::loading_screen()
            : m_uniform_buffer_binding(-1)
            , m_frame_delegate(0)
            , m_visible(false)
        {}

        loading_screen::~loading_screen()
        {}

        void loading_screen::shutdown()
        {
            hide();

            if (m_frame_delegate) {
                ui::get_ui()->get_window()->frame.unbind(m_frame_delegate);
                delete m_frame_delegate;
                m_frame_delegate = 0;
            }

            m_draw_queue.reset();
            m_prim_stream.reset();
            m_shader.reset();
            m_texture.reset();
            m_uniform_buffer.reset();

            if (m_loaded_objects.is_valid()) {
                serialize::get_database()->clear_objects(m_loaded_objects);
                m_loaded_objects.reset();
            }
        }

        void loading_screen::load(
#if !defined(ELECTROSLAG_BUILD_SHIP)
            bool dump_content
#endif
            )
        {
#if !defined(ELECTROSLAG_BUILD_SHIP)
            std::string const bin_path("loading_screen.bin");
            std::string const json_path("..\\..\\..\\electroslag\\resources\\content\\loading_screen.json");
#endif

            // Load the necessary objects.
            serialize::database* d = serialize::get_database();

            resource::ref loading_screen_resource(resource::create(ID_LOADING_SCREEN));
            if (loading_screen_resource.is_valid()) {
                m_loaded_objects = d->load_objects(loading_screen_resource, std::string("."));
            }
#if !defined(ELECTROSLAG_BUILD_SHIP)
            else if (std::filesystem::exists(bin_path)) {
                m_loaded_objects = d->load_objects(bin_path);
            }
            else if (std::filesystem::exists(json_path)) {
                m_loaded_objects = d->load_objects_json(json_path);
            }

            ELECTROSLAG_CHECK(m_loaded_objects.is_valid());

            if (dump_content) {
                d->dump_objects(m_loaded_objects);
            }
#endif
        }

#if !defined(ELECTROSLAG_BUILD_SHIP)
        void loading_screen::save(std::string const& file_name)
        {
            ELECTROSLAG_CHECK(m_loaded_objects.is_valid());
            serialize::get_database()->save_objects(file_name, m_loaded_objects);
        }
#endif

        void loading_screen::show()
        {
            serialize::database* d = serialize::get_database();

            // Get references to graphics object descriptors.
            graphics::primitive_stream_descriptor::ref prim_stream_desc(
                d->find_object_ref<graphics::primitive_stream_descriptor>(
                    hash_string("loading_screen::prim_stream")
                    ));

            graphics::shader_program_descriptor::ref shader_desc(
                d->find_object_ref<graphics::shader_program_descriptor>(
                    hash_string("loading_screen::shader")
                    ));

            graphics::texture_descriptor::ref texture_desc(
                d->locate_object_ref<graphics::texture_descriptor>(
                    hash_string("loading_screen::texture")
                    ));
#if !defined(ELECTROSLAG_BUILD_SHIP)
            if (!texture_desc.is_valid()) {
                texture::gli_importer::ref texture_importer(
                    d->find_object_ref<texture::gli_importer>(
                        hash_string("loading_screen::texture::importer")
                        ));
                texture_desc = texture_importer->get_future()->get_wait();
            }
#endif

            // Create the real graphics objects.
            graphics::graphics_interface* g = graphics::get_graphics();
            g->check_initialized();

            m_draw_queue = g->create_command_queue(ELECTROSLAG_STRING_AND_HASH("cq:loading_screen"));

            m_prim_stream = g->create_primitive_stream(prim_stream_desc);
            m_shader = g->create_shader_program(shader_desc, prim_stream_desc->get_fields());
            m_texture = g->create_texture(texture_desc);

            // The size parameter for the UBO descriptor is determined by the render thread, so we need to wait
            // for it to be computed.
            graphics::get_graphics()->finish_commands();

            graphics::uniform_buffer_descriptor::ref uniform_buffer(
                m_shader->get_descriptor()->get_fragment_shader()->find_uniform_buffer(hash_string("_f_uniforms"))
            );

            graphics::buffer_descriptor::ref uniform_desc(graphics::buffer_descriptor::create());
            uniform_desc->set_buffer_memory_map(graphics::buffer_memory_map_write);
            uniform_desc->set_buffer_memory_caching(graphics::buffer_memory_caching_coherent);
            uniform_desc->set_uninitialized_data_size(uniform_buffer->get_size());
            m_uniform_buffer = g->create_buffer(uniform_desc);
            m_uniform_buffer_binding = uniform_buffer->get_binding();

            // Write the texture handle into the uniform buffer.
            // The buffer isn't mappable until after the render thread has handled the creation, so wait.
            graphics::get_graphics()->finish_commands();

            byte* ubo_memory = m_uniform_buffer->map();
            graphics::field_structs::texture_handle h(m_texture->get_handle());
            uniform_buffer->get_fields()->find(hash_string("texture"))->write_uniform(ubo_memory, &h);
            m_uniform_buffer->unmap();

            // Hook up listener
            m_frame_delegate = ui::window_interface::frame_delegate::create_from_method<loading_screen, &loading_screen::on_window_frame>(this);
            ui::get_ui()->get_window()->frame.bind(m_frame_delegate, event_bind_mode_reference_listener);

            m_visible = true;
        }

        void loading_screen::hide()
        {
            m_visible = false;
        }

        void loading_screen::on_window_frame(int /*millisec_elapsed*/)
        {
            if (m_visible) {
                m_draw_queue->enqueue_command<draw_command>(this);
                graphics::get_graphics()->flush_commands();
            }
        }

        void loading_screen::draw_command::execute(graphics::context_interface* context)
        {
            context->clear_color(0.0f, 0.0f, 0.0f, 1.0f);
            context->bind_shader_program(m_this->m_shader);
            context->bind_primitive_stream(m_this->m_prim_stream);
            context->bind_uniform_buffer(m_this->m_uniform_buffer, m_this->m_uniform_buffer_binding);
            context->draw();
            context->swap();
        }
    }
}
