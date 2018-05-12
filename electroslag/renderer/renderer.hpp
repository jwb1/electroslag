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
#include "electroslag/ui/ui_interface.hpp"
#include "electroslag/threading/mutex.hpp"
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/graphics/context_interface.hpp"
#include "electroslag/renderer/renderer_interface.hpp"
#include "electroslag/renderer/field_source_interface.hpp"
#include "electroslag/renderer/pass_interface.hpp"
#include "electroslag/renderer/uniform_buffer_manager.hpp"
#include "electroslag/renderer/primitive_stream_manager.hpp"
#include "electroslag/renderer/texture_manager.hpp"
#include "electroslag/renderer/shader_program_manager.hpp"
#include "electroslag/renderer/pipeline_manager.hpp"

namespace electroslag {
    namespace renderer {
        class renderer 
            : public renderer_interface
            , public field_source_interface {
        private:
            typedef std::vector<pass_interface::ref> pass_vector;
        public:
            typedef pass_vector::const_iterator const_pass_iterator;
            typedef pass_vector::iterator pass_iterator;

            renderer();
            virtual ~renderer()
            {}

            // Implement field_source_interface
            virtual bool locate_field_source(graphics::shader_field const* field, void const** field_source) const;

            // Implement renderer_interface methods.
            virtual void initialize();
            virtual void shutdown();

            virtual bool is_initialized() const;

            virtual scene::ref create_instances(instance_descriptor::ref const& desc);

            typedef event<void, scene::ref const&> scene_created_event;
            typedef scene_created_event::bound_delegate scene_created_delegate;
            mutable scene_created_event scene_created;

            // renderer internal methods.

            // The list of passes is built at initialization time and is constant afterwards,
            // so no mutex required for iteration.
            const_pass_iterator begin_passes() const
            {
                return (m_passes.begin());
            }

            pass_iterator begin_passes()
            {
                return (m_passes.begin());
            }

            const_pass_iterator end_passes() const
            {
                return (m_passes.end());
            }

            pass_iterator end_passes()
            {
                return (m_passes.end());
            }

            int get_pass_count() const
            {
                return (static_cast<int>(m_passes.size()));
            }

            int get_geometry_pass_count() const
            {
                return (m_geometry_pass_count);
            }

            // Each manager object needs to do it's own thread synchronization.
            uniform_buffer_manager* get_ubo_manager()
            {
                return (&m_ubo_manager);
            }

            primitive_stream_manager* get_primitive_stream_manager()
            {
                return (&m_primitive_stream_manager);
            }

            texture_manager* get_texture_manager()
            {
                return (&m_texture_manager);
            }

            shader_program_manager* get_shader_program_manager()
            {
                return (&m_shader_program_manager);
            }

            pipeline_manager* get_pipeline_manager()
            {
                return (&m_pipeline_manager);
            }

            // Called by mesh rendering work item.
            void finish_rendering_mesh(frame_details* this_frame_details);

        private:
            class finish_drawing_command : public graphics::command {
            public:
                explicit finish_drawing_command(frame_details* this_frame_details)
                    : m_this_frame_details(this_frame_details)
                {}

                virtual void execute(graphics::context_interface* context);

            private:
                frame_details* m_this_frame_details;
            };

            void on_window_destroyed();
            void on_window_size_changed(ui::window_dimensions const* dimensions);
            void on_window_paused_changed(bool paused);
            void on_window_frame(int millisec_elapsed);

            mutable threading::mutex m_mutex;

            ui::window_interface::destroyed_delegate* m_destroyed_delegate;
            ui::window_interface::size_changed_delegate* m_size_changed_delegate;
            ui::window_interface::paused_changed_delegate* m_paused_changed_delegate;
            ui::window_interface::frame_delegate* m_frame_delegate;

            graphics::command_queue_interface::ref m_pre_frame_queue;
            graphics::command_queue_interface::ref m_post_frame_queue;

            typedef std::vector<scene::ref> scene_vector;
            scene_vector m_scenes;

            pass_vector m_passes;
            int m_geometry_pass_count;

            uniform_buffer_manager m_ubo_manager;
            primitive_stream_manager m_primitive_stream_manager;
            texture_manager m_texture_manager;
            shader_program_manager m_shader_program_manager;
            pipeline_manager m_pipeline_manager;

            static constexpr int const per_frame_count = graphics::context_interface::display_buffer_count + 1;
            frame_details m_per_frame_details[per_frame_count];

            int m_current_frame_index;

            bool m_initialized;

            // Disallowed operations:
            explicit renderer(renderer const&);
            renderer& operator =(renderer const&);
        };

        renderer* get_renderer_internal();
    }
}
