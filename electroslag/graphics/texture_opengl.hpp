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
#include "electroslag/graphics/graphics_types.hpp"
#include "electroslag/graphics/texture_descriptor.hpp"
#include "electroslag/graphics/texture_interface.hpp"
#include "electroslag/graphics/sync_interface.hpp"

namespace electroslag {
    namespace graphics {
        class texture_opengl : public texture_interface {
        public:
            typedef reference<texture_opengl> ref;

            static ref create(
                texture_descriptor::ref& texture_desc
                )
            {
                ref new_texture(new texture_opengl());
                new_texture->schedule_async_create(texture_desc);
                return (new_texture);
            }

            static ref create_finished(
                texture_descriptor::ref& texture_desc
                )
            {
                ref new_texture(new texture_opengl());
                new_texture->schedule_async_create_finished(texture_desc);
                return (new_texture);
            }

            // Implement texture_interface
            virtual bool is_finished() const
            {
                return (m_is_finished.load(std::memory_order_acquire));
            }

            virtual field_structs::texture_handle get_handle()
            {
                ELECTROSLAG_CHECK(is_finished());
                ELECTROSLAG_CHECK(m_complete);
                return (m_handle);
            }

        private:
            class create_command : public command {
            public:
                create_command(
                    ref const& texture,
                    texture_descriptor::ref& texture_desc
                    )
                    : m_texture(texture)
                    , m_texture_desc(texture_desc)
                {}

                create_command(
                    ref const& texture,
                    texture_descriptor::ref& texture_desc,
                    sync_interface::ref const& finish_sync
                    )
                    : m_texture(texture)
                    , m_texture_desc(texture_desc)
                    , m_finish_sync(finish_sync)
                {}

                virtual void execute(context_interface* context);

            private:
                ref m_texture;
                texture_descriptor::ref m_texture_desc;
                sync_interface::ref m_finish_sync;

                // Disallowed operations:
                create_command();
                explicit create_command(create_command const&);
                create_command& operator =(create_command const&);
            };

            class destroy_command : public command {
            public:
                explicit destroy_command(opengl_object_id texture_id)
                    : m_texture_id(texture_id)
                {}

                virtual void execute(context_interface* context);

            private:
                opengl_object_id m_texture_id;

                // Disallowed operations:
                destroy_command();
                explicit destroy_command(destroy_command const&);
                destroy_command& operator =(destroy_command const&);
            };

            texture_opengl()
                : m_is_finished(false)
                , m_texture_target(0)
                , m_texture_id(0)
                , m_complete(false)
            {}
            virtual ~texture_opengl();

            void schedule_async_create(texture_descriptor::ref& texture_desc);
            void schedule_async_create_finished(texture_descriptor::ref& texture_desc);

            void opengl_create(texture_descriptor::ref& texture_desc);
            static void opengl_destroy(opengl_object_id texture_id);

            void opengl_create_map(texture_descriptor::ref& texture_desc);
            void opengl_create_3d(texture_descriptor::ref& texture_desc);
            void opengl_create_array(texture_descriptor::ref& texture_desc);
            void opengl_create_cube(texture_descriptor::ref& texture_desc);
            void opengl_create_cube_array(texture_descriptor::ref& texture_desc);

            void opengl_create_generate_levels(texture_descriptor::ref& texture_desc) const;

            void opengl_create_set_parameters(texture_descriptor::ref& texture_desc) const;

            void opengl_create_common(GLenum texture_target);

            void opengl_create_immutable_allocate(
                texture_descriptor::ref& texture_desc,
                int levels,
                int depth
                ) const;

            void opengl_create_load_level(
                GLenum map_target,
                image_descriptor::ref const& image
                ) const;

            void opengl_create_set_minification_filter(
                texture_filter filter,
                texture_filter mip_filter
                ) const;

            void opengl_create_set_magnification_filter(
                texture_filter filter
                ) const;

            void opengl_create_set_wrap_mode(
                texture_coord_wrap wrap_mode,
                GLenum pname
                ) const;

            std::atomic<bool> m_is_finished;

            // Initialized on construction and read-only after
            GLenum m_texture_target;

            // Accessed by the graphics command execution thread only
            opengl_object_id m_texture_id;

            field_structs::texture_handle m_handle;

            // Indicates if all of the map levels, array indexes, etc... are
            // populated.
            bool m_complete;

            // Disallowed operations:
            explicit texture_opengl(texture_opengl const&);
            texture_opengl& operator =(texture_opengl const&);
        };
    }
}
