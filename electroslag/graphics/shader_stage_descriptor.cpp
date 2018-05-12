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
#include "electroslag/graphics/shader_stage_descriptor.hpp"

namespace electroslag {
    namespace graphics {
        shader_stage_descriptor::shader_stage_descriptor(serialize::archive_reader_interface* ar)
        {
            if (!ar->read_enumeration("stage_flag", &m_stage_flag, shader_stage_strings)) {
                throw load_object_failure("stage_flag");
            }

            bool has_source_file = true;
            ar->read_boolean("has_source_file", &has_source_file);

            // Stage source is either read from an external file or inline.
            if (has_source_file) {
                std::string source_file;
                if (!ar->read_string("source_file", &source_file)) {
                    throw load_object_failure("source_file");
                }

                std::filesystem::path source_path(source_file);
                ELECTROSLAG_CHECK(source_path.has_filename());

                // This method seems to have been renamed between VS 2013 and VS 2015.
#if defined(_MSC_VER) && (_MSC_VER <= 1800)
                if (!source_path.is_complete()) {
#else
                if (!source_path.is_absolute()) {
#endif
                    std::filesystem::path combined_dir(ar->get_base_directory());
                    combined_dir /= source_path;
                    source_path = std::filesystem::canonical(combined_dir);
                }

                file_stream shader_stream;
                shader_stream.open(source_path.string(), file_stream_access_mode_read);

                long long stream_size = shader_stream.get_size();
                ELECTROSLAG_CHECK(stream_size < INT_MAX);

                m_source = referenced_buffer_from_sizeof::create(static_cast<int>(stream_size) + 1);
                {
                    referenced_buffer_interface::accessor src(m_source);
                    shader_stream.read(src.get_pointer(), stream_size);
                    *(reinterpret_cast<char*>(src.get_pointer()) + stream_size) = '\0'; // NUL terminator.
                }
            }
            else {
                std::string stage_source;
                if (!ar->read_string("source", &stage_source)) {
                    throw load_object_failure("source");
                }

                m_source = referenced_buffer_from_sizeof::create(static_cast<int>(stage_source.length() + 1));
                {
                    referenced_buffer_interface::accessor src(m_source);
                    strcpy(static_cast<char*>(src.get_pointer()), stage_source.c_str());
                }
            }

            int ubo_count = 0; // Default
            ar->read_int32("ubo_count", &ubo_count);

            if (ubo_count) {
                m_ubo.reserve(ubo_count);

                serialize::enumeration_namer namer(ubo_count, 'u');
                while (!namer.used_all_names()) {
                    unsigned long long ubo_hash = 0;
                    if (!ar->read_name_hash(namer.get_next_name(), &ubo_hash)) {
                        throw load_object_failure("ubo");
                    }
                    m_ubo.emplace_back(serialize::get_database()->find_object_ref<uniform_buffer_descriptor>(ubo_hash));
                }
            }
        }

        void shader_stage_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
            ELECTROSLAG_CHECK(has_source());

            // Write in UBOs associated with the stage.
            uniform_buffer_iterator i(begin_uniform_buffers());
            while (i != end_uniform_buffers()) {
                (*i)->save_to_archive(ar);
                ++i;
            }

            // Write the stage object itself.
            serializable_object::save_to_archive(ar);

            ar->write_int32("stage_flag", m_stage_flag);
            ar->write_boolean("has_source_file", false);

            // Stage source code.
            {
                referenced_buffer_interface::accessor src(m_source);
                ar->write_string("source", std::string(static_cast<char*>(src.get_pointer()), src.get_sizeof()));
            }

            // Reference the UBOs
            int ubo_count = get_uniform_buffer_count();
            ar->write_uint32("ubo_count", ubo_count);

            serialize::enumeration_namer namer(ubo_count, 'u');
            i = begin_uniform_buffers();
            while (i != end_uniform_buffers()) {
                ar->write_name_hash(namer.get_next_name(), (*i)->get_hash());
                ++i;
            }
        }
    }
}
