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
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/graphics/graphics_types.hpp"

namespace electroslag {
    namespace graphics {
        // The serialized field classes use gross address-of operations; a way to treat
        // a struct as an array. GLM itself uses these kinds of things in it's
        // operator[] implementation, so if it works for them...
        class serialized_vec2
            : public serialize::serializable_object<serialized_vec2>
            , public field_structs::vec2 {
        public:
            // Implement serializable_object
            explicit serialized_vec2(serialize::archive_reader_interface* archive)
            {
                if (!archive->read_buffer("field", &x, sizeof(field_structs::vec2))) {
                    throw load_object_failure("field");
                }
            }
            virtual void save_to_archive(serialize::archive_writer_interface* archive)
            {
                serializable_object::save_to_archive(archive);
                archive->write_buffer("field", &x, sizeof(field_structs::vec2));
            }

        private:
            // Disabled operations.
            explicit serialized_vec2(serialized_vec2 const&);
            serialized_vec2& operator =(serialized_vec2 const&);
        };

        class serialized_vec3
            : public serialize::serializable_object<serialized_vec3>
            , public field_structs::vec3 {
        public:
            // Implement serializable_object
            explicit serialized_vec3(serialize::archive_reader_interface* archive)
            {
                if (!archive->read_buffer("field", &x, sizeof(field_structs::vec3))) {
                    throw load_object_failure("field");
                }
            }
            virtual void save_to_archive(serialize::archive_writer_interface* archive)
            {
                serializable_object::save_to_archive(archive);
                archive->write_buffer("field", &x, sizeof(field_structs::vec3));
            }

        private:
            // Disabled operations.
            explicit serialized_vec3(serialized_vec3 const&);
            serialized_vec3& operator =(serialized_vec3 const&);
        };

        class serialized_vec4
            : public serialize::serializable_object<serialized_vec4>
            , public field_structs::vec4 {
        public:
            // Implement serializable_object
            explicit serialized_vec4(serialize::archive_reader_interface* archive)
            {
                if (!archive->read_buffer("field", &x, sizeof(field_structs::vec4))) {
                    throw load_object_failure("field");
                }
            }
            virtual void save_to_archive(serialize::archive_writer_interface* archive)
            {
                serializable_object::save_to_archive(archive);
                archive->write_buffer("field", &x, sizeof(field_structs::vec4));
            }

        private:
            // Disabled operations.
            explicit serialized_vec4(serialized_vec4 const&);
            serialized_vec4& operator =(serialized_vec4 const&);
        };

        class serialized_uvec2
            : public serialize::serializable_object<serialized_uvec2>
            , public field_structs::uvec2 {
        public:
            // Implement serializable_object
            explicit serialized_uvec2(serialize::archive_reader_interface* archive)
            {
                if (!archive->read_buffer("field", &x, sizeof(field_structs::uvec2))) {
                    throw load_object_failure("field");
                }
            }
            virtual void save_to_archive(serialize::archive_writer_interface* archive)
            {
                serializable_object::save_to_archive(archive);
                archive->write_buffer("field", &x, sizeof(field_structs::uvec2));
            }

        private:
            // Disabled operations.
            explicit serialized_uvec2(serialized_uvec2 const&);
            serialized_uvec2& operator =(serialized_uvec2 const&);
        };

        class serialized_mat4
            : public serialize::serializable_object<serialized_mat4>
            , public field_structs::mat4 {
        public:
            // Implement serializable_object
            explicit serialized_mat4(serialize::archive_reader_interface* archive)
            {
                if (!archive->read_buffer("field", &((*this)[0][0]), sizeof(field_structs::mat4))) {
                    throw load_object_failure("field");
                }
            }
            virtual void save_to_archive(serialize::archive_writer_interface* archive)
            {
                serializable_object::save_to_archive(archive);
                archive->write_buffer("field", &((*this)[0][0]), sizeof(field_structs::mat4));
            }

        private:
            // Disabled operations.
            explicit serialized_mat4(serialized_mat4 const&);
            serialized_mat4& operator =(serialized_mat4 const&);
        };

        // The depth test parameters can be serialized as a separate object, but usually
        // in ship data, it's serialized inline.
        class serialized_depth_test_params
            : public serialize::serializable_object<serialized_depth_test_params> {
        public:
            // Implement serializable_object
            explicit serialized_depth_test_params(serialize::archive_reader_interface* archive);
            virtual void save_to_archive(serialize::archive_writer_interface* archive);

            graphics::depth_test_params get() const
            {
                return (m_depth_test_params);
            }

        private:
            graphics::depth_test_params m_depth_test_params;

            // Disabled operations.
            explicit serialized_depth_test_params(serialized_depth_test_params const&);
            serialized_depth_test_params& operator =(serialized_depth_test_params const&);
        };

        class serialized_blending_params
            : public serialize::serializable_object<serialized_blending_params> {
        public:
            // Implement serializable_object
            explicit serialized_blending_params(serialize::archive_reader_interface* archive);
            virtual void save_to_archive(serialize::archive_writer_interface* archive);

            graphics::blending_params get() const
            {
                return (m_blending_params);
            }

        private:
            graphics::blending_params m_blending_params;

            // Disabled operations.
            explicit serialized_blending_params(serialized_blending_params const&);
            serialized_blending_params& operator =(serialized_blending_params const&);
        };

        class serialized_sampler_params
            : public serialize::serializable_object<serialized_sampler_params> {
        public:
            // Implement serializable_object
            explicit serialized_sampler_params(serialize::archive_reader_interface* archive);
            virtual void save_to_archive(serialize::archive_writer_interface* archive);

            graphics::sampler_params get() const
            {
                return (m_sampler_params);
            }

        private:
            graphics::sampler_params m_sampler_params;

            // Disabled operations.
            explicit serialized_sampler_params(serialized_sampler_params const&);
            serialized_sampler_params& operator =(serialized_sampler_params const&);
        };
    }
}

