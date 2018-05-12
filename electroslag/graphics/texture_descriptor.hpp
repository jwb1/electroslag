//  Electros1lag Interactive Graphics System
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
#include "electroslag/graphics/image_descriptor.hpp"
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/serialize/database.hpp"

namespace electroslag {
    namespace graphics {
        class texture_descriptor
            : public referenced_object
            , public serialize::serializable_object<texture_descriptor> {
        public:
            typedef reference<texture_descriptor> ref;
        private:
            typedef std::vector<image_descriptor::ref> image_vector;
        public:
            typedef image_vector::const_iterator const_image_iterator;
            typedef image_vector::iterator image_iterator;
            typedef image_vector::const_reverse_iterator const_reverse_image_iterator;
            typedef image_vector::reverse_iterator reverse_image_iterator;

            static ref create()
            {
                return (ref(new texture_descriptor()));
            }

            // Implement serializable_object
            explicit texture_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            texture_type_flags get_type() const
            {
                return (m_fields.type_flags);
            }

            void set_type(texture_type_flags type_flags)
            {
                check_valid_texture_type_flags(type_flags);
                m_fields.type_flags = type_flags;
            }

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
                return (m_fields.width);
            }

            int get_height() const
            {
                return (m_fields.height);
            }

            int get_extent() const
            {
                return (m_fields.extent);
            }

            void set_dimensions(int width, int height = 1, int extent = 1)
            {
                ELECTROSLAG_CHECK(width > 0 && width <= 65536 &&
                                  height > 0 && height <= 65536 &&
                                  extent > 0 && extent <= 65536);
                m_fields.width = width;
                m_fields.height = height;
                m_fields.extent = extent;
            }

            texture_level_generate get_mip_level_generation_mode() const
            {
                return (m_fields.generate_mip_levels);
            }

            void set_mip_level_generation_mode(texture_level_generate generate_mip_levels)
            {
                ELECTROSLAG_CHECK(generate_mip_levels > texture_level_generate_unknown &&
                                  generate_mip_levels < texture_level_generate_count);
                m_fields.generate_mip_levels = generate_mip_levels;
            }

            texture_filter get_magnification_filter() const
            {
                return (m_fields.magnification_filter);
            }

            texture_filter get_minification_filter() const
            {
                return (m_fields.minification_filter);
            }

            texture_filter get_mip_filter() const
            {
                return (m_fields.mip_filter);
            }

            void set_filter(
                texture_filter magnification_filter,
                texture_filter minification_filter,
                texture_filter mip_filter = texture_filter_default
                )
            {
                ELECTROSLAG_CHECK(magnification_filter > texture_filter_unknown && magnification_filter < texture_filter_count);
                ELECTROSLAG_CHECK(minification_filter > texture_filter_unknown && minification_filter < texture_filter_count);
                ELECTROSLAG_CHECK(mip_filter > texture_filter_unknown && mip_filter < texture_filter_count);

                m_fields.magnification_filter = magnification_filter;
                m_fields.minification_filter = minification_filter;
                m_fields.mip_filter = mip_filter;
            }

            texture_coord_wrap get_s_wrap_mode() const
            {
                return (m_fields.s_wrap_mode);
            }

            texture_coord_wrap get_t_wrap_mode() const
            {
                return (m_fields.t_wrap_mode);
            }

            texture_coord_wrap get_u_wrap_mode() const
            {
                return (m_fields.u_wrap_mode);
            }

            void set_coord_wrap(
                texture_coord_wrap s_wrap_mode,
                texture_coord_wrap t_wrap_mode,
                texture_coord_wrap u_wrap_mode = texture_coord_wrap_default
                )
            {
                ELECTROSLAG_CHECK(s_wrap_mode > texture_coord_wrap_unknown && s_wrap_mode < texture_coord_wrap_count);
                ELECTROSLAG_CHECK(t_wrap_mode > texture_coord_wrap_unknown && t_wrap_mode < texture_coord_wrap_count);
                ELECTROSLAG_CHECK(u_wrap_mode > texture_coord_wrap_unknown && u_wrap_mode < texture_coord_wrap_count);

                m_fields.s_wrap_mode = s_wrap_mode;
                m_fields.t_wrap_mode = t_wrap_mode;
                m_fields.u_wrap_mode = u_wrap_mode;
            }

            // Images
            void insert_image(image_descriptor::ref& image);

            const_image_iterator begin_images() const
            {
                return (m_images.begin());
            }

            image_iterator begin_images()
            {
                return (m_images.begin());
            }

            const_image_iterator end_images() const
            {
                return (m_images.end());
            }

            image_iterator end_images()
            {
                return (m_images.end());
            }

            const_reverse_image_iterator rbegin_images() const
            {
                return (m_images.rbegin());
            }

            reverse_image_iterator rbegin_images()
            {
                return (m_images.rbegin());
            }

            const_reverse_image_iterator rend_images() const
            {
                return (m_images.rend());
            }

            reverse_image_iterator rend_images()
            {
                return (m_images.rend());
            }

            int compute_mip_levels() const
            {
                int levels = 1;
                if (m_fields.type_flags & texture_type_flags_mipmap) {
                    int level0_width = m_fields.width;
                    int level0_height = m_fields.height;
                    while (level0_width > 1 || level0_height > 1) {
                        level0_width /= 2;
                        level0_height /= 2;
                        ++levels;
                    }
                }
                return (levels);
            }

            static bool is_texture_complete(texture_descriptor const* t);

        private:
            texture_descriptor()
            {}

            static void check_valid_texture_type_flags(
#if !defined(ELECTROSLAG_BUILD_SHIP)
                texture_type_flags type_flags
#else
                texture_type_flags
#endif
                )
            {
                // Only certain combinations of the type flags are currently valid.
                ELECTROSLAG_CHECK(
                    ((type_flags == texture_type_flags_normal) ||
                     (type_flags == texture_type_flags_mipmap) ||
                     (type_flags == texture_type_flags_3d) ||
                     (type_flags == texture_type_flags_array) ||
                     (type_flags == texture_type_flags_cube) ||
                     (type_flags == (texture_type_flags_3d | texture_type_flags_mipmap)) ||
                     (type_flags == (texture_type_flags_array | texture_type_flags_mipmap)) ||
                     (type_flags == (texture_type_flags_cube | texture_type_flags_mipmap)) ||
                     (type_flags == (texture_type_flags_cube | texture_type_flags_array)) ||
                     (type_flags == (texture_type_flags_cube | texture_type_flags_array | texture_type_flags_mipmap)))
                    );
            }

            static bool is_expected_image(
                texture_descriptor const* t,
                image_descriptor::ref const& image,
                int expected_slice,
                texture_cube_face expected_face,
                int expected_level
                );

            image_vector m_images;
            struct packed_fields {
                packed_fields()
                    : type_flags(texture_type_flags_normal)
                    , generate_mip_levels(texture_level_generate_off)
                    , color_format(texture_color_format_unknown)
                    , extent(0)
                    , width(0)
                    , height(0)
                    , magnification_filter(texture_filter_default)
                    , minification_filter(texture_filter_default)
                    , mip_filter(texture_filter_default)
                    , s_wrap_mode(texture_coord_wrap_default)
                    , t_wrap_mode(texture_coord_wrap_default)
                    , u_wrap_mode(texture_coord_wrap_default)
                    , reserved(0)
                {}

                texture_type_flags type_flags:4;
                texture_level_generate generate_mip_levels:4;
                ELECTROSLAG_ENUM_BITPACK_CHECK(texture_level_generate, 4);
                texture_color_format color_format:8; // This is serialized in 7 bits in the image_descriptor.
                ELECTROSLAG_ENUM_BITPACK_CHECK(texture_color_format, 8);
                unsigned int extent:16; // Z-depth for 3D textures, array count for array textures

                unsigned int width:16;
                unsigned int height:16;

                texture_filter magnification_filter:4;
                texture_filter minification_filter:4;
                texture_filter mip_filter:4;
                ELECTROSLAG_ENUM_BITPACK_CHECK(texture_filter, 4);

                texture_coord_wrap s_wrap_mode:4;
                texture_coord_wrap t_wrap_mode:4;
                texture_coord_wrap u_wrap_mode:4;
                ELECTROSLAG_ENUM_BITPACK_CHECK(texture_coord_wrap, 4);
                
                unsigned int reserved:8;
            } m_fields;
            ELECTROSLAG_STATIC_CHECK(sizeof(packed_fields) == sizeof(unsigned int) * 3, "Bit packing check");

            // Disallowed operations:
            explicit texture_descriptor(texture_descriptor const&);
            texture_descriptor& operator =(texture_descriptor const&);
        };
    }
}
