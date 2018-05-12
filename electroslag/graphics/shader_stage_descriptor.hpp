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
#include "electroslag/referenced_buffer.hpp"
#include "electroslag/graphics/uniform_buffer_descriptor.hpp"
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/serialize/database.hpp"

namespace electroslag {
    namespace graphics {
        class shader_stage_descriptor
            : public referenced_object
            , public serialize::serializable_object<shader_stage_descriptor> {
        public:
            typedef reference<shader_stage_descriptor> ref;
        private:
            typedef std::vector<uniform_buffer_descriptor::ref> stage_ubo_vector;
        public:
            typedef stage_ubo_vector::const_iterator const_uniform_buffer_iterator;
            typedef stage_ubo_vector::iterator uniform_buffer_iterator;

            static ref create(shader_stage stage_flag)
            {
                return (ref(new shader_stage_descriptor(stage_flag)));
            }

            static ref clone(ref const& clone_ref)
            {
                return (ref(new shader_stage_descriptor(*clone_ref.get_pointer())));
            }

            // Implement serializable_object
            explicit shader_stage_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            shader_stage get_stage_flag() const
            {
                return (m_stage_flag);
            }

            // GLSL source code for the stage.
            bool has_source() const
            {
                return (m_source.is_valid() && m_source->get_sizeof() > 0);
            }

            referenced_buffer_interface::ref const& get_source() const
            {
                return (m_source);
            }

            void set_source(referenced_buffer_interface::ref const& source)
            {
                m_source = source;
            }

            void clear_source()
            {
                m_source.reset(); // Reference to source cleared on shader creation
            }

            // Metadata describing the uniform buffers that must be bound to the stage.
            void insert_uniform_buffer(uniform_buffer_descriptor::ref const& ubo)
            {
                m_ubo.emplace_back(ubo);
            }

            const_uniform_buffer_iterator begin_uniform_buffers() const
            {
                return (m_ubo.begin());
            }

            uniform_buffer_iterator begin_uniform_buffers()
            {
                return (m_ubo.begin());
            }

            const_uniform_buffer_iterator end_uniform_buffers() const
            {
                return (m_ubo.end());
            }

            uniform_buffer_iterator end_uniform_buffers()
            {
                return (m_ubo.end());
            }

            int get_uniform_buffer_count() const
            {
                return (static_cast<int>(m_ubo.size()));
            }

            uniform_buffer_descriptor::ref const& find_uniform_buffer(
                unsigned long long name_hash
                ) const
            {
                const_uniform_buffer_iterator u(begin_uniform_buffers());
                while (u != end_uniform_buffers()) {
                    if ((*u)->get_hash() == name_hash) {
                        return (*u);
                    }
                    ++u;
                }
                throw object_not_found_failure("uniform_buffer_descriptor", name_hash);
            }

            uniform_buffer_descriptor::ref& find_uniform_buffer(
                unsigned long long name_hash
                )
            {
                // Casting off the const is kind of gross, but/less gross than outright
                // duplication of the method implementation.
                return (const_cast<uniform_buffer_descriptor::ref&>(
                    const_cast<const shader_stage_descriptor*>(this)->find_uniform_buffer(name_hash)
                    ));
            }

            uniform_buffer_descriptor::ref const& find_uniform_buffer(std::string const& name) const
            {
                return (find_uniform_buffer(hash_string_runtime(name)));
            }
            uniform_buffer_descriptor::ref& find_uniform_buffer(std::string const& name)
            {
                return (find_uniform_buffer(hash_string_runtime(name)));
            }

        private:
            explicit shader_stage_descriptor(shader_stage stage_flag)
                : m_stage_flag(stage_flag)
            {}

            explicit shader_stage_descriptor(shader_stage_descriptor const& copy_object)
                : referenced_object()
                , serializable_object(copy_object)
                , m_stage_flag(copy_object.m_stage_flag)
            {
                if (copy_object.m_source.is_valid()) {
                    m_source = cloned_referenced_buffer::create(copy_object.m_source);
                }

                m_ubo.reserve(copy_object.get_uniform_buffer_count());
                const_uniform_buffer_iterator u(copy_object.begin_uniform_buffers());
                while (u != copy_object.end_uniform_buffers()) {
                    m_ubo.emplace_back(uniform_buffer_descriptor::clone(*u));
                    ++u;
                }
            }

            shader_stage m_stage_flag;
            referenced_buffer_interface::ref m_source;
            stage_ubo_vector m_ubo;

            // Disallowed operations:
            shader_stage_descriptor();
            shader_stage_descriptor& operator =(shader_stage_descriptor const&);
        };
    }
}
