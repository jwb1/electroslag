//  Electroslag Interactive Graphics System
//  Copyright 2015 Joshua Buckman
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
#include "electroslag/texture/gli_importer.hpp"
#include "electroslag/threading/thread_pool.hpp"
#include "electroslag/graphics/serialized_graphics_types.hpp"

namespace electroslag {
    namespace texture {
        gli_importer::gli_importer(
            std::string const& file_name,
            std::string const& object_name,
            serialize::load_record::ref& record,
            graphics::sampler_params const* sampler,
            graphics::texture_level_generate generate_mip_levels
            )
        {
            import(file_name, object_name, record, sampler, generate_mip_levels);
        }
        
        gli_importer::gli_importer(serialize::archive_reader_interface* ar)
        {
            std::string file_name;
            if (!ar->read_string("file_name", &file_name)) {
                throw load_object_failure("file_name");
            }

            std::filesystem::path dir(file_name);
            ELECTROSLAG_CHECK(dir.has_filename());

            // This method seems to have been renamed between VS 2013 and VS 2015.
#if defined(_MSC_VER) && (_MSC_VER <= 1800)
            if (!dir.is_complete()) {
#else
            if (!dir.is_absolute()) {
#endif
                std::filesystem::path combined_dir(ar->get_base_directory());
                combined_dir /= dir;
                dir = std::filesystem::canonical(combined_dir);
            }

            std::string object_name("");
            ar->read_string("object_name", &object_name);

            graphics::texture_level_generate generate_mip_levels = graphics::texture_level_generate_off;
            ar->read_enumeration("generate_mip_levels", &generate_mip_levels, graphics::texture_level_generate_strings);

            graphics::sampler_params sampler;
            unsigned long long sampler_name_hash = 0;
            if (ar->read_name_hash("sampler", &sampler_name_hash)) {
                graphics::serialized_sampler_params* serialized_params = dynamic_cast<graphics::serialized_sampler_params*>(
                    serialize::get_database()->find_object(sampler_name_hash)
                    );
                sampler = serialized_params->get();
            }

            import(dir.string(), object_name, ar->get_load_record(), &sampler, generate_mip_levels);
        }

        void gli_importer::save_to_archive(serialize::archive_writer_interface*)
        {
            // This class just imports data; there is no logical way to save an importer.
        }

        void gli_importer::finish_importing()
        {
            if (m_async_loader.is_valid()) {
                m_async_loader->wait_for_done();
            }
        }

        void gli_importer::import(
            std::string const& file_name,
            std::string const& object_name,
            serialize::load_record::ref& record,
            graphics::sampler_params const* sampler,
            graphics::texture_level_generate generate_mip_levels
            )
        {
            m_async_loader = threading::get_io_thread_pool()->enqueue_work_item<async_texture_loader>(
                ref(this),
                file_name,
                object_name,
                record,
                sampler,
                generate_mip_levels
                ).cast<import_future>();
        }

        gli_importer::import_future::value_type gli_importer::async_texture_loader::execute_for_value()
        {
            gli::texture gli_texture(gli::load(m_file_name));
            if (gli_texture.empty()) {
                throw load_object_failure("gli::load");
            }

            // Create a texture description to populate from the file.
            graphics::texture_descriptor::ref texture_desc(graphics::texture_descriptor::create());
            texture_desc->set_name(m_object_name);
            texture_desc->set_mip_level_generation_mode(m_generate_mip_levels);
            texture_desc->set_filter(
                m_sampler.magnification_filter,
                m_sampler.minification_filter,
                m_sampler.mip_filter
                );
            texture_desc->set_coord_wrap(
                m_sampler.s_wrap_mode,
                m_sampler.t_wrap_mode,
                m_sampler.u_wrap_mode
                );

            // Texture type bits.
            static int const gli_target_to_type_flags[gli::TARGET_LAST + 1] = {
                graphics::texture_type_flags_normal, // gli::TARGET_1D
                graphics::texture_type_flags_array,  // gli::TARGET_1D_ARRAY
                graphics::texture_type_flags_normal, // gli::TARGET_2D
                graphics::texture_type_flags_array,  // gli::TARGET_2D_ARRAY
                graphics::texture_type_flags_3d,     // gli::TARGET_3D
                graphics::texture_type_flags_normal, // gli::TARGET_RECT
                graphics::texture_type_flags_array,  // gli::TARGET_RECT_ARRAY
                graphics::texture_type_flags_cube,   // gli::TARGET_CUBE
                graphics::texture_type_flags_cube | graphics::texture_type_flags_array  // gli::TARGET_CUBE_ARRAY
            };

            gli::target gli_texture_target = gli_texture.target();
            int type_flags = gli_target_to_type_flags[gli_texture_target];

            if (gli_texture.levels() > 1 || m_generate_mip_levels != graphics::texture_level_generate_off) {
                type_flags |= graphics::texture_type_flags_mipmap;
            }

            texture_desc->set_type(static_cast<graphics::texture_type_flags>(type_flags));

            // Color format.
            graphics::texture_color_format color_format = graphics::texture_color_format_none;
            gli::format gli_format = gli_texture.format();
            switch (gli_format) {
            // RGBA formats
            case gli::FORMAT_L8_UNORM_PACK8:
            case gli::FORMAT_R8_UNORM_PACK8:
                color_format = graphics::texture_color_format_r8;
                break;

            case gli::FORMAT_R5G6B5_UNORM_PACK16:
                color_format = graphics::texture_color_format_r5g6b5;
                break;

            case gli::FORMAT_RGB8_UNORM_PACK8:
                color_format = graphics::texture_color_format_r8g8b8;
                break;

            case gli::FORMAT_RGB8_SRGB_PACK8:
                color_format = graphics::texture_color_format_r8g8b8_srgb;
                break;

            case gli::FORMAT_RGBA8_UNORM_PACK8:
                color_format = graphics::texture_color_format_r8g8b8a8;
                break;

            case gli::FORMAT_RGBA8_SRGB_PACK8:
                color_format = graphics::texture_color_format_r8g8b8a8_srgb;
                break;

            // BGRA formats; convert to RGBA.
            case gli::FORMAT_BGR8_UNORM_PACK8:
                gli_texture.swizzle<glm::u8vec4>(gli::swizzles(gli::SWIZZLE_BLUE, gli::SWIZZLE_GREEN, gli::SWIZZLE_RED, gli::SWIZZLE_ALPHA));
                color_format = graphics::texture_color_format_r8g8b8;
                break;

            case gli::FORMAT_BGR8_SRGB_PACK8:
                gli_texture.swizzle<glm::u8vec4>(gli::swizzles(gli::SWIZZLE_BLUE, gli::SWIZZLE_GREEN, gli::SWIZZLE_RED, gli::SWIZZLE_ALPHA));
                color_format = graphics::texture_color_format_r8g8b8_srgb;
                break;

            case gli::FORMAT_BGRA8_UNORM_PACK8:
                gli_texture.swizzle<glm::u8vec4>(gli::swizzles(gli::SWIZZLE_BLUE, gli::SWIZZLE_GREEN, gli::SWIZZLE_RED, gli::SWIZZLE_ALPHA));
                color_format = graphics::texture_color_format_r8g8b8a8;
                break;

            case gli::FORMAT_BGRA8_SRGB_PACK8:
                gli_texture.swizzle<glm::u8vec4>(gli::swizzles(gli::SWIZZLE_BLUE, gli::SWIZZLE_GREEN, gli::SWIZZLE_RED, gli::SWIZZLE_ALPHA));
                color_format = graphics::texture_color_format_r8g8b8a8_srgb;
                break;

            // Compressed formats.
            case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8:
                color_format = graphics::texture_color_format_dxt1;
                break;

            case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8:
                color_format = graphics::texture_color_format_dxt1_alpha;
                break;

            case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16:
                color_format = graphics::texture_color_format_dxt3;
                break;

            case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16:
                color_format = graphics::texture_color_format_dxt5;
                break;

            case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8:
                color_format = graphics::texture_color_format_dxt1_srgb;
                break;

            case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8:
                color_format = graphics::texture_color_format_dxt1_alpha_srgb;
                break;

            case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16:
                color_format = graphics::texture_color_format_dxt3_srgb;
                break;

            case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16:
                color_format = graphics::texture_color_format_dxt5_srgb;
                break;

            case gli::FORMAT_R_ATI1N_UNORM_BLOCK8:
                color_format = graphics::texture_color_format_rgtc1;
                break;

            case gli::FORMAT_R_ATI1N_SNORM_BLOCK8:
                color_format = graphics::texture_color_format_rgtc1_signed;
                break;

            case gli::FORMAT_RG_ATI2N_UNORM_BLOCK16:
                color_format = graphics::texture_color_format_rgtc2;
                break;

            case gli::FORMAT_RG_ATI2N_SNORM_BLOCK16:
                color_format = graphics::texture_color_format_rgtc2_signed;
                break;

            case gli::FORMAT_RGBA_BP_UNORM_BLOCK16:
                color_format = graphics::texture_color_format_bptc;
                break;

            case gli::FORMAT_RGBA_BP_SRGB_BLOCK16:
                color_format = graphics::texture_color_format_bptc_srgb;
                break;

            case gli::FORMAT_RGB_BP_SFLOAT_BLOCK16:
                color_format = graphics::texture_color_format_bptc_sfloat;
                break;

            case gli::FORMAT_RGB_BP_UFLOAT_BLOCK16:
                color_format = graphics::texture_color_format_bptc_ufloat;
                break;

            default:
                throw load_object_failure("gli::format");
            }
            texture_desc->set_color_format(color_format);

            // Texture dimensions
            gli::extent3d extent(gli_texture.extent());
            texture_desc->set_dimensions(extent.x, extent.y, extent.z);

            // Import the texture's images.
            for (int slice = 0; slice < gli_texture.layers(); ++slice) {
                for (int face_number = 0; face_number < gli_texture.faces(); ++face_number) {
                    for (int level = 0; level < gli_texture.levels(); ++level) {
                        gli::extent3d level_extent(gli_texture.extent(level));

                        graphics::image_descriptor::ref image(graphics::image_descriptor::create());

                        std::string image_name;
                        formatted_string_append(image_name, "%s::s%02.2d::f%02.2d::l%02.2d", m_object_name.c_str(), slice, face_number, level);
                        image->set_name(image_name);
                        image->set_color_format(color_format);
                        image->set_mip_level(level);
                        image->set_slice(slice);

                        graphics::texture_cube_face face = (gli::is_target_cube(gli_texture.target()) ?
                            static_cast<graphics::texture_cube_face>(face_number) :
                            graphics::texture_cube_face_normal);
                        image->set_cube_face(face);

                        int stride = static_cast<int>(level_extent.x * gli::block_size(gli_format));
                        image->set_dimensions(level_extent.x, level_extent.y, stride);

                        int image_sizeof = static_cast<int>(gli_texture.size(level));
                        referenced_buffer_interface::ref pixels = referenced_buffer_from_sizeof::create(
                            image_sizeof
                            );
                        {
                            referenced_buffer_interface::accessor pixel_accessor(pixels);
                            memcpy(pixel_accessor.get_pointer(), gli_texture.data(slice, face_number, level), image_sizeof);
                        }
                        image->set_pixels(pixels);

                        m_load_record->insert_object(image);
                        texture_desc->insert_image(image);
                    }
                }
            }

            // Save the texture in the object database.
            m_load_record->insert_object(texture_desc);

            // Resolve circular dependencies.
            m_this_importer.reset();
            m_load_record.reset();
            return (texture_desc);
        }

        gli::texture gli_importer::async_texture_loader::convert(gli::texture const& gli_texture, gli::format dest_format)
        {
            switch (gli_texture.target()) {
            case gli::TARGET_1D:
                return (gli::convert(gli::texture1d(gli_texture), dest_format));

            case gli::TARGET_1D_ARRAY:
                return (gli::convert(gli::texture1d_array(gli_texture), dest_format));

            case gli::TARGET_2D:
            case gli::TARGET_RECT:
                return (gli::convert(gli::texture2d(gli_texture), dest_format));

            case gli::TARGET_2D_ARRAY:
            case gli::TARGET_RECT_ARRAY:
                return (gli::convert(gli::texture2d_array(gli_texture), dest_format));

            case gli::TARGET_3D:
                return (gli::convert(gli::texture3d(gli_texture), dest_format));

            case gli::TARGET_CUBE:
                return (gli::convert(gli::texture_cube(gli_texture), dest_format));

            case gli::TARGET_CUBE_ARRAY:
                return (gli::convert(gli::texture_cube_array(gli_texture), dest_format));

            default:
                throw load_object_failure("gli::target");
            }
        }
    }
}
