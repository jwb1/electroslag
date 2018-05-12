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
#include "electroslag/graphics/graphics_types.hpp"
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/archive_interface.hpp"

namespace electroslag {
    namespace graphics {
        class image_descriptor
            : public referenced_object
            , public serialize::serializable_object<image_descriptor> {
        public:
            typedef reference<image_descriptor> ref;

            static ref create()
            {
                return (ref(new image_descriptor()));
            }

            // Implement serializable_object
            explicit image_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            texture_color_format get_color_format() const
            {
                return (m_fields.color_format);
            }

            void set_color_format(texture_color_format color_format)
            {
                ELECTROSLAG_CHECK(color_format > texture_color_format_unknown && color_format < texture_color_format_count);
                m_fields.color_format = color_format;
            }

            int get_width() const
            {
                return (m_fields.width + 1);
            }

            int get_height() const
            {
                return (m_fields.height + 1);
            }

            int get_stride() const
            {
                return (m_stride);
            }

            void set_dimensions(int width, int height, int stride)
            {
                ELECTROSLAG_CHECK(width > 0 && width <= 65536 &&
                                  height > 0 && height <= 65536 &&
                                  stride != 0 && ((stride & 0x3) == 0));
                // One bias the width and height to allow 1 to 2^16 rather than 0 to 2^16 - 1
                m_fields.width = width - 1;
                m_fields.height = height - 1;
                m_stride = stride;
            }

            int get_mip_level() const
            {
                return (m_fields.mip_level);
            }

            void set_mip_level(int mip_level)
            {
                // Width and height are capped at 65536, which means at most levels [0, 16]
                ELECTROSLAG_CHECK(mip_level >= 0 && mip_level <= 16);
                m_fields.mip_level = mip_level;
            }

            texture_cube_face get_cube_face() const
            {
                return (m_fields.cube_face);
            }

            void set_cube_face(texture_cube_face cube_face)
            {
                ELECTROSLAG_CHECK(cube_face > texture_cube_face_unknown && cube_face < texture_cube_face_count);
                m_fields.cube_face = cube_face;
            }

            int get_slice() const
            {
                return (m_fields.slice);
            }

            void set_slice(int slice)
            {
                ELECTROSLAG_CHECK(slice >= 0 && slice < 65536);
                m_fields.slice = slice;
            }

            bool has_pixels() const
            {
                return (m_pixels.is_valid() && m_pixels->get_sizeof() > 0);
            }

            referenced_buffer_interface::ref const& get_pixels() const
            {
                return (m_pixels);
            }

            void set_pixels(referenced_buffer_interface::ref const& pixels)
            {
                m_pixels = pixels;
            }

            void clear_pixels()
            {
                m_pixels.reset();
            }

        private:
            image_descriptor()
                : m_stride(-1)
            {}

            referenced_buffer_interface::ref m_pixels;
            int m_stride;
            struct packed_fields {
                packed_fields()
                    : color_format(texture_color_format_unknown)
                    , mip_level(0)
                    , cube_face(texture_cube_face_normal)
                    , slice(0)
                    , width(0)
                    , height(0)
                {}

                texture_color_format color_format:7;
                ELECTROSLAG_ENUM_BITPACK_CHECK(texture_color_format, 7);
                unsigned int mip_level:5;
                texture_cube_face cube_face:4; // Could remove a bit here if needed.
                ELECTROSLAG_ENUM_BITPACK_CHECK(texture_cube_face, 4);
                unsigned int slice:16;

                unsigned int width:16;
                unsigned int height:16;
            } m_fields;
            ELECTROSLAG_STATIC_CHECK(sizeof(packed_fields) == sizeof(unsigned int) * 2, "Bit packing check");

            // Disallowed operations:
            explicit image_descriptor(image_descriptor const&);
            image_descriptor& operator =(image_descriptor const&);
        };
    }
}
