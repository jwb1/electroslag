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
#include "electroslag/graphics/image_descriptor.hpp"

namespace electroslag {
    namespace graphics {
        image_descriptor::image_descriptor(serialize::archive_reader_interface* ar)
        {
            if (!ar->read_buffer("packed_fields", &m_fields, sizeof(m_fields))) {
                texture_color_format color_format = texture_color_format_unknown;
                if (ar->read_enumeration("color_format", &color_format, texture_color_format_strings)) {
                    m_fields.color_format = color_format;
                }

                uint32_t width = 1;
                if (ar->read_uint32("width", &width)) {
                    if (width > (1 << 16) || width == 0) {
                        throw load_object_failure("width");
                    }
                    m_fields.width = width - 1;
                }

                uint32_t height = 1;
                if (ar->read_uint32("height", &height)) {
                    if (height > (1 << 16) || height == 0) {
                        throw load_object_failure("height");
                    }
                    m_fields.height = height - 1;
                }

                uint8_t mip_level = 0;
                if (ar->read_uint8("mip_level", &mip_level) && mip_level >= (1 << 5)) {
                    throw load_object_failure("mip_level");
                }
                m_fields.mip_level = mip_level;

                texture_cube_face cube_face = texture_cube_face_normal;
                ar->read_enumeration("cube_face", &cube_face, texture_cube_face_strings);
                m_fields.cube_face = cube_face;

                uint16_t slice = 0;
                ar->read_uint16("slice", &slice);
                m_fields.slice = slice;
            }

            if (!ar->read_int32("stride", &m_stride)) {
                throw load_object_failure("stride");
            }

            int pixels_sizeof = 0;
            if (!ar->read_int32("pixel_sizeof", &pixels_sizeof)) {
                throw load_object_failure("pixel_sizeof");
            }
            m_pixels = referenced_buffer_from_sizeof::create(pixels_sizeof);
            {
                referenced_buffer_interface::accessor pixel_accessor(m_pixels);
                if (!ar->read_buffer("pixels", pixel_accessor.get_pointer(), pixels_sizeof)) {
                    throw load_object_failure("pixels");
                }
            }
        }

        void image_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
            serializable_object::save_to_archive(ar);

            ar->write_buffer("packed_fields", &m_fields, sizeof(m_fields));
            ar->write_int32("stride", m_stride);
            {
                referenced_buffer_interface::accessor pixel_accessor(m_pixels);
                int pixels_sizeof = pixel_accessor.get_sizeof();
                ar->write_int32("pixel_sizeof", pixels_sizeof);
                ar->write_buffer("pixels", pixel_accessor.get_pointer(), pixels_sizeof);
            }
        }
    }
}
