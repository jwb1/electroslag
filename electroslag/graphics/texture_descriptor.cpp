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
#include "electroslag/graphics/texture_descriptor.hpp"
#include "electroslag/graphics/serialized_graphics_types.hpp"

namespace electroslag {
    namespace graphics {

        texture_descriptor::texture_descriptor(serialize::archive_reader_interface* ar)
        {
            if (!ar->read_buffer("packed_fields", &m_fields, sizeof(m_fields))) {
                // Per value read; allows for some values to be left out and use sensible results.
                texture_type_flags type_flags = texture_type_flags_normal;
                ar->read_enumeration_flags("type_flags", &type_flags, texture_type_flags_strings, texture_type_flags_bit_values);
                m_fields.type_flags = type_flags;

                texture_level_generate generate_mip_levels = texture_level_generate_off;
                ar->read_enumeration("generate_mip_levels", &generate_mip_levels, texture_level_generate_strings);
                m_fields.generate_mip_levels = generate_mip_levels;

                texture_color_format color_format = texture_color_format_unknown;
                ar->read_enumeration("color_format", &color_format, texture_color_format_strings);
                m_fields.color_format = color_format;

                uint32_t extent = 0;
                ar->read_uint32("extent", &extent);
                m_fields.extent = extent;

                uint32_t width = 0;
                ar->read_uint32("width", &width);
                m_fields.width = width;

                uint32_t height = 0;
                ar->read_uint32("height", &height);
                m_fields.height = height;

                unsigned long long sampler_name_hash = 0;
                if (ar->read_name_hash("sampler", &sampler_name_hash)) {
                    serialized_sampler_params* serialized_params = dynamic_cast<serialized_sampler_params*>(
                        serialize::get_database()->find_object(sampler_name_hash)
                        );
                    m_fields.magnification_filter = serialized_params->get().magnification_filter;
                    m_fields.minification_filter = serialized_params->get().minification_filter;
                    m_fields.mip_filter = serialized_params->get().mip_filter;
                    m_fields.s_wrap_mode = serialized_params->get().s_wrap_mode;
                    m_fields.t_wrap_mode = serialized_params->get().t_wrap_mode;
                    m_fields.u_wrap_mode = serialized_params->get().u_wrap_mode;
                }
            }

            int image_count = 0;
            if (!ar->read_int32("image_count", &image_count) || image_count <= 0) {
                throw load_object_failure("image_count");
            }
            m_images.reserve(image_count);

            serialize::enumeration_namer namer(image_count, 'i');
            while (!namer.used_all_names()) {
                unsigned long long image_name_hash = 0;
                if (!ar->read_name_hash(namer.get_next_name(), &image_name_hash)) {
                    throw load_object_failure("image");
                }
                m_images.emplace_back(
                    serialize::get_database()->find_object_ref<image_descriptor>(image_name_hash)
                    );
            }
        }

        void texture_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
            ELECTROSLAG_CHECK(is_texture_complete(this));

            // Write in all of the images first.
            image_iterator i(m_images.begin());
            while (i != m_images.end()) {
                (*i)->save_to_archive(ar);
                ++i;
            }

            // Next write the texture itself.
            serializable_object::save_to_archive(ar);

            ar->write_buffer("packed_fields", &m_fields, sizeof(m_fields));

            int image_count = static_cast<int>(m_images.size());
            ar->write_int32("image_count", image_count);

            serialize::enumeration_namer namer(image_count, 'i');
            i = m_images.begin();
            while (i != m_images.end()) {
                ar->write_name_hash(namer.get_next_name(), (*i)->get_hash());
                ++i;
            }
        }

        void texture_descriptor::insert_image(image_descriptor::ref& image)
        {
            // The use of the reverse iterator is a guess that the most likely insertion
            // point is more to the end. If you insert in the "right" order, this method
            // will not iterate at all.
            reverse_image_iterator i(rbegin_images());
            while (i != rend_images()) {
                bool do_insert_here = false;

                int image_slice = image->get_slice();
                int current_slice = (*i)->get_slice();
                if (current_slice < image_slice) {
                    do_insert_here = true;
                }
                else if (current_slice == image_slice) {
                    texture_cube_face image_face = image->get_cube_face();
                    texture_cube_face current_face = (*i)->get_cube_face();
                    if (current_face < image_face) {
                        do_insert_here = true;
                    }
                    else if (current_face == image_face) {
                        int image_level = image->get_mip_level();
                        int current_level = (*i)->get_mip_level();
                        if (current_level < image_level) {
                            do_insert_here = true;
                        }
                    }
                }

                if (do_insert_here) {
                    m_images.insert(i.base(), image);
                    return;
                }

                ++i;
            }

            if (m_images.empty()) {
                m_images.emplace_back(image);
            }
            else {
                throw std::runtime_error("Could not locate insertion point");
            }
        }

        // static
        bool texture_descriptor::is_texture_complete(texture_descriptor const* t)
        {
            const_image_iterator i(t->begin_images());

            int total_slices = 1;
            if ((t->get_type() & texture_type_flags_3d) ||
                (t->get_type() & texture_type_flags_array)) {
                total_slices = t->get_extent();
            }

            int mip_levels = 1;
            bool expect_all_mip_levels = false;
            if (t->get_type() & texture_type_flags_mipmap) {
                mip_levels = t->compute_mip_levels();
                expect_all_mip_levels = (t->get_mip_level_generation_mode() == texture_level_generate_off);
            }

            for (int expected_slice = 0; expected_slice < total_slices; ++expected_slice) {

                if (t->get_type() & texture_type_flags_cube) {
                    for (texture_cube_face expected_face = texture_cube_face_front;
                         expected_face < texture_cube_face_count;
                         expected_face = static_cast<texture_cube_face>(static_cast<int>(expected_face) + 1)) {

                        if (expect_all_mip_levels) {
                            for (int expected_level = 0; expected_level < mip_levels; ++expected_level) {
                                if (i == t->end_images()) return (false);
                                if (!is_expected_image(t, *i, expected_slice, expected_face, expected_level)) return (false);
                                ++i;
                            }
                        }
                        else {
                            if (i == t->end_images()) return (false);
                            if (!is_expected_image(t, *i, expected_slice, expected_face, 0)) return (false);
                            ++i;
                        }
                    }
                }
                else {
                    if (expect_all_mip_levels) {
                        for (int expected_level = 0; expected_level < mip_levels; ++expected_level) {
                            if (i == t->end_images()) return (false);
                            if (!is_expected_image(t, *i, expected_slice, texture_cube_face_normal, expected_level)) return (false);
                            ++i;
                        }
                    }
                    else {
                        if (i == t->end_images()) return (false);
                        if (!is_expected_image(t, *i, expected_slice, texture_cube_face_normal, 0)) return (false);
                        ++i;
                    }
                }
            }

            if (i == t->end_images()) return (true);
            else return (false);
        }

        // static
        bool texture_descriptor::is_expected_image(
            texture_descriptor const* t,
            image_descriptor::ref const& image,
            int expected_slice,
            texture_cube_face expected_face,
            int expected_level
            )
        {
            int expected_width = t->get_width() / (1 << expected_level);
            if (expected_width < 1) expected_width = 1;
            int expected_height = t->get_height() / (1 << expected_level);
            if (expected_height < 1) expected_height = 1;

            if (image->get_slice() == expected_slice &&
                image->get_cube_face() == expected_face &&
                image->get_mip_level() == expected_level &&
                image->get_color_format() == t->get_color_format() &&
                image->get_height() == expected_height &&
                image->get_width() == expected_width
                ) {

                return (true);
            }
            else {
                return (false);
            }
        }
    }
}
