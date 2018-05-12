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
#include "electroslag/graphics/texture_opengl.hpp"
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/graphics/context_opengl.hpp"


namespace electroslag {
    namespace graphics {
        void texture_opengl::schedule_async_create(
            texture_descriptor::ref& texture_desc
            )
        {
            graphics_interface* g = get_graphics();

            if (g->get_render_thread()->is_running()) {
                opengl_create(texture_desc);
            }
            else {
                g->get_render_policy()->get_system_command_queue()->enqueue_command<create_command>(
                    ref(this),
                    texture_desc
                    );
            }
        }

        void texture_opengl::schedule_async_create_finished(
            texture_descriptor::ref& texture_desc
            )
        {
            graphics_interface* g = get_graphics();
            ELECTROSLAG_CHECK(!g->get_render_thread()->is_running());

            sync_interface::ref finish_sync = g->create_sync();

            g->get_render_policy()->get_system_command_queue()->enqueue_command<create_command>(
                ref(this),
                texture_desc,
                finish_sync
                );

            finish_sync->wait();
        }

        texture_opengl::~texture_opengl()
        {
            if (m_is_finished.load(std::memory_order_acquire)) {
                m_is_finished.store(false, std::memory_order_release);

                graphics_interface* g = get_graphics();

                if (g->get_render_thread()->is_running()) {
                    opengl_destroy(m_texture_id);
                }
                else {
                    // This command can run completely asynchronously to "this" objects destruction.
                    g->get_render_policy()->get_system_command_queue()->enqueue_command<destroy_command>(
                        m_texture_id
                        );
                }

                m_texture_id = 0;
            }
            else {
                ELECTROSLAG_LOG_WARN("texture_opengl destructor before finished");
            }
        }

        // static
        void texture_opengl::opengl_destroy(
            opengl_object_id texture_id
            )
        {
            if (texture_id) {
                gl::DeleteTextures(1, &texture_id);
            }
        }

        void texture_opengl::opengl_create(texture_descriptor::ref& texture_desc)
        {
            ELECTROSLAG_CHECK(!is_finished());
            ELECTROSLAG_CHECK(texture_descriptor::is_texture_complete(texture_desc.get_pointer()));

            switch (texture_desc->get_type()) {
            case texture_type_flags_normal:
            case texture_type_flags_mipmap:
                opengl_create_map(texture_desc);
                break;

            case texture_type_flags_3d:
                opengl_create_3d(texture_desc);
                break;

            case texture_type_flags_array:
                opengl_create_array(texture_desc);
                break;

            case texture_type_flags_cube:
                opengl_create_cube(texture_desc);
                break;

            // TODO: support the creation of the rest of the texture types
            //case texture_type_flags_cube | texture_type_flags_array:
            //    opengl_create_cube_array(texture_desc);

            default:
                throw parameter_failure("OpenGL does not support the given texture flag feature combination");
            }

            opengl_create_generate_levels(texture_desc);
            opengl_create_set_parameters(texture_desc);

            m_handle.h = gl::GetTextureHandleARB(m_texture_id);
            gl::MakeTextureHandleResidentARB(m_handle.h);

            m_complete = true;

#if !defined(ELECTROSLAG_BUILD_SHIP)
            if (texture_desc->has_name_string()) {
                std::string texture_name(texture_desc->get_name());
                gl::ObjectLabel(
                    gl::TEXTURE,
                    m_texture_id,
                    static_cast<GLsizei>(texture_name.length()),
                    texture_name.c_str()
                    );
                context_opengl::check_opengl_error();
            }
#endif

            m_is_finished.store(true, std::memory_order_release);
        }

        void texture_opengl::opengl_create_map(
            texture_descriptor::ref& texture_desc
            )
        {
            opengl_create_common(gl::TEXTURE_2D);

            int levels = texture_desc->compute_mip_levels();
            opengl_create_immutable_allocate(texture_desc, levels, 0);

            texture_descriptor::const_image_iterator i(texture_desc->begin_images());
            ELECTROSLAG_CHECK(i != texture_desc->end_images());

            opengl_create_load_level(gl::TEXTURE_2D, *i);
            ++i;
            --levels;

            if (i != texture_desc->end_images()) {
                ELECTROSLAG_CHECK(texture_desc->get_type() & texture_type_flags_mipmap);
                if (texture_desc->get_mip_level_generation_mode() == texture_level_generate_off) {
                    do {
                        opengl_create_load_level(gl::TEXTURE_2D, *i);
                        ++i;
                        --levels;
                    } while (i != texture_desc->end_images());
                    ELECTROSLAG_CHECK(levels == 0);
                }
            }
        }

        void texture_opengl::opengl_create_3d(
            texture_descriptor::ref& texture_desc
            )
        {
            opengl_create_common(gl::TEXTURE_3D);

            int levels = texture_desc->compute_mip_levels();
            opengl_create_immutable_allocate(texture_desc, levels, 0);
        }

        void texture_opengl::opengl_create_array(
            texture_descriptor::ref& texture_desc
            )
        {
            opengl_create_common(gl::TEXTURE_2D_ARRAY);

            int levels = texture_desc->compute_mip_levels();
            opengl_create_immutable_allocate(texture_desc, levels, 0);
        }

        void texture_opengl::opengl_create_cube(
            texture_descriptor::ref& texture_desc
            )
        {
            opengl_create_common(gl::TEXTURE_CUBE_MAP);

            int levels_per_face = texture_desc->compute_mip_levels();
            opengl_create_immutable_allocate(texture_desc, levels_per_face, 0);

            // Same order as the texture_cube_face
            static GLenum const cube_targets[] = {
                0,                               // texture_cube_face_normal
                gl::TEXTURE_CUBE_MAP_POSITIVE_Z, // texture_cube_face_front
                gl::TEXTURE_CUBE_MAP_NEGATIVE_Z, // texture_cube_face_back
                gl::TEXTURE_CUBE_MAP_POSITIVE_X, // texture_cube_face_left
                gl::TEXTURE_CUBE_MAP_NEGATIVE_X, // texture_cube_face_right
                gl::TEXTURE_CUBE_MAP_POSITIVE_Y, // texture_cube_face_top
                gl::TEXTURE_CUBE_MAP_NEGATIVE_Y  // texture_cube_face_bottom
            };
            ELECTROSLAG_STATIC_CHECK(
                _countof(cube_targets) == texture_cube_face_count,
                "cube_targets mismatch"
                );

            texture_descriptor::const_image_iterator i(texture_desc->begin_images());
            ELECTROSLAG_CHECK(i != texture_desc->end_images());

            for (unsigned int face = 1; face < texture_cube_face_count; ++face) {
                int levels = levels_per_face;
                GLenum target = cube_targets[(*i)->get_cube_face()];

                opengl_create_load_level(target, *i);
                ++i;
                --levels;

                if (i != texture_desc->end_images()) {
                    ELECTROSLAG_CHECK(texture_desc->get_type() & texture_type_flags_mipmap);
                    if (texture_desc->get_mip_level_generation_mode() == texture_level_generate_off) {
                        do {
                            ELECTROSLAG_CHECK(target == cube_targets[(*i)->get_cube_face()]);
                            opengl_create_load_level(target, *i);
                            ++i;
                            --levels;
                        } while (i != texture_desc->end_images());
                    }
                }
                ELECTROSLAG_CHECK(levels == 0);
            }
        }

        /*
        void texture_opengl::initialize_command::init_cube_array(
        slago_texture_header const* tex_header,
        byte const* //buffer
        )
        {
        if (!GLEW_ARB_texture_cube_map_array) {
        BOOST_THROW_EXCEPTION(runtime_failure("cube map arrays are not supported"));
        }

        common_opengl_init(GL_TEXTURE_CUBE_MAP_ARRAY_ARB);

        int levels = 0;
        if (tex_header->type.flags.mipmap) {
        levels = compute_mip_levels(tex_header->width, tex_header->height);
        }
        else {
        levels = 1;
        }

        bool immutable_storage = immutable_allocate(tex_header, levels, tex_header->array_count * 6);

        m_texture->m_complete = false;
        }
        */

        void texture_opengl::opengl_create_generate_levels(
            texture_descriptor::ref& texture_desc
            ) const
        {
            if (texture_desc->get_type() & texture_type_flags_mipmap) {
                texture_level_generate generate_mode = texture_desc->get_mip_level_generation_mode();
                if (generate_mode != texture_level_generate_off) {
                    // TODO: Mipmap generation quality flags?
                    gl::GenerateMipmap(m_texture_target);
                    context_opengl::check_opengl_error();
                }
            }
        }

        void texture_opengl::opengl_create_set_parameters(
            texture_descriptor::ref& texture_desc
            ) const
        {
            texture_filter mipmap_filter = texture_filter_default;
            if (texture_desc->get_type() & texture_type_flags_mipmap) {
                mipmap_filter = texture_desc->get_mip_filter();
            }

            opengl_create_set_magnification_filter(
                texture_desc->get_magnification_filter()
                );

            opengl_create_set_minification_filter(
                texture_desc->get_minification_filter(),
                mipmap_filter
                );

            opengl_create_set_wrap_mode(
                texture_desc->get_s_wrap_mode(),
                gl::TEXTURE_WRAP_S
                );

            opengl_create_set_wrap_mode(
                texture_desc->get_t_wrap_mode(),
                gl::TEXTURE_WRAP_T
                );

            if (texture_desc->get_type() & texture_type_flags_3d) {
                opengl_create_set_wrap_mode(
                    texture_desc->get_u_wrap_mode(),
                    gl::TEXTURE_WRAP_R
                    );
            }
        }

        void texture_opengl::opengl_create_common(
            GLenum texture_target
            )
        {
            m_texture_target = texture_target;

            opengl_object_id texture_id = 0;
            gl::GenTextures(1, &texture_id);
            context_opengl::check_opengl_error();

            ELECTROSLAG_CHECK(!m_texture_id);
            m_texture_id = texture_id;

            gl::ActiveTexture(gl::TEXTURE0);
            context_opengl::check_opengl_error();

            gl::BindTexture(texture_target, texture_id);
            context_opengl::check_opengl_error();
        }

        void texture_opengl::opengl_create_immutable_allocate(
            texture_descriptor::ref& texture_desc,
            int levels,
            int depth
            ) const
        {
            // For immutable storage, we need a different format specification than glTexImage2D
            static GLenum const sized_formats[texture_color_format_count] = {
                0,                 // texture_color_format_none
                gl::R8,            // texture_color_format_r8
                gl::RGB565,        // texture_color_format_r5g6b5
                gl::RGB8,          // texture_color_format_r8g8b8
                gl::SRGB8,         // texture_color_format_r8g8b8_srgb
                gl::RGBA8,         // texture_color_format_r8g8b8a8
                gl::SRGB8_ALPHA8,  // texture_color_format_r8g8b8a8_srgb

                gl::COMPRESSED_RGB_S3TC_DXT1_EXT,  // texture_color_format_dxt1
                gl::COMPRESSED_RGBA_S3TC_DXT1_EXT, // texture_color_format_dxt1_alpha
                gl::COMPRESSED_RGBA_S3TC_DXT3_EXT, // texture_color_format_dxt3
                gl::COMPRESSED_RGBA_S3TC_DXT5_EXT, // texture_color_format_dxt5

                gl::COMPRESSED_SRGB_S3TC_DXT1_EXT,       // texture_color_format_dxt1_srgb
                gl::COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, // texture_color_format_dxt1_alpha_srgb
                gl::COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, // texture_color_format_dxt3_srgb
                gl::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, // texture_color_format_dxt5_srgb

                gl::COMPRESSED_RED_RGTC1,        // texture_color_format_rgtc1
                gl::COMPRESSED_SIGNED_RED_RGTC1, // texture_color_format_rgtc1_signed
                gl::COMPRESSED_RG_RGTC2,         // texture_color_format_rgtc2
                gl::COMPRESSED_SIGNED_RG_RGTC2,  // texture_color_format_rgtc2_signed

                gl::COMPRESSED_RGBA_BPTC_UNORM,         // texture_color_format_bptc
                gl::COMPRESSED_SRGB_ALPHA_BPTC_UNORM,   // texture_color_format_bptc_srgb
                gl::COMPRESSED_RGB_BPTC_SIGNED_FLOAT,   // texture_color_format_bptc_sfloat
                gl::COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT  // texture_color_format_bptc_ufloat
            };

            if (!depth) {
                gl::TexStorage2D(
                    m_texture_target,
                    levels,
                    sized_formats[texture_desc->get_color_format()],
                    texture_desc->get_width(),
                    texture_desc->get_height()
                    );
                context_opengl::check_opengl_error();
            }
            else {
                gl::TexStorage3D(
                    m_texture_target,
                    levels,
                    sized_formats[texture_desc->get_color_format()],
                    texture_desc->get_width(),
                    texture_desc->get_height(),
                    depth
                    );
                context_opengl::check_opengl_error();
            }
        }

        void texture_opengl::opengl_create_load_level(
            GLenum map_target,
            image_descriptor::ref const& image
            ) const
        {
            GLenum format = 0;
            GLenum type = 0;
            bool compressed = false;

            switch (image->get_color_format()) {
            case texture_color_format_r8:
                format = gl::RED;
                type = gl::UNSIGNED_BYTE;
                break;

            case texture_color_format_r5g6b5:
                format = gl::RGB;
                type = gl::UNSIGNED_SHORT_5_6_5;
                break;

            case texture_color_format_r8g8b8:
            case texture_color_format_r8g8b8_srgb:
                format = gl::RGB;
                type = gl::UNSIGNED_BYTE;
                break;

            case texture_color_format_r8g8b8a8:
            case texture_color_format_r8g8b8a8_srgb:
                format = gl::RGBA;
                type = gl::UNSIGNED_BYTE;
                break;

            case texture_color_format_dxt1:
                type = gl::COMPRESSED_RGB_S3TC_DXT1_EXT;
                compressed = true;
                break;

            case texture_color_format_dxt1_alpha:
                type = gl::COMPRESSED_RGBA_S3TC_DXT1_EXT;
                compressed = true;
                break;

            case texture_color_format_dxt3:
                type = gl::COMPRESSED_RGBA_S3TC_DXT3_EXT;
                compressed = true;
                break;

            case texture_color_format_dxt5:
                type = gl::COMPRESSED_RGBA_S3TC_DXT5_EXT;
                compressed = true;
                break;

            case texture_color_format_dxt1_srgb:
                type = gl::COMPRESSED_SRGB_S3TC_DXT1_EXT;
                compressed = true;
                break;

            case texture_color_format_dxt1_alpha_srgb:
                type = gl::COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
                compressed = true;
                break;

            case texture_color_format_dxt3_srgb:
                type = gl::COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
                compressed = true;
                break;

            case texture_color_format_dxt5_srgb:
                type = gl::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
                compressed = true;
                break;

            case texture_color_format_rgtc1:
                type = gl::COMPRESSED_RED_RGTC1;
                compressed = true;
                break;

            case texture_color_format_rgtc1_signed:
                type = gl::COMPRESSED_SIGNED_RED_RGTC1;
                compressed = true;
                break;

            case texture_color_format_rgtc2:
                type = gl::COMPRESSED_RG_RGTC2;
                compressed = true;
                break;

            case texture_color_format_rgtc2_signed:
                type = gl::COMPRESSED_SIGNED_RG_RGTC2;
                compressed = true;
                break;

            case texture_color_format_bptc:
                type = gl::COMPRESSED_RGBA_BPTC_UNORM;
                compressed = true;
                break;

            case texture_color_format_bptc_srgb:
                type = gl::COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
                compressed = true;
                break;

            case texture_color_format_bptc_sfloat:
                type = gl::COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
                compressed = true;
                break;

            case texture_color_format_bptc_ufloat:
                type = gl::COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
                compressed = true;
                break;

            default:
                throw std::logic_error("unknown texture color format");
            }

            {
                referenced_buffer_interface::accessor pixel_accessor(image->get_pixels());
                if (compressed) {
                    gl::CompressedTexSubImage2D(
                        map_target,
                        image->get_mip_level(),
                        0,
                        0,
                        image->get_width(),
                        image->get_height(),
                        type,
                        pixel_accessor.get_sizeof(),
                        pixel_accessor.get_pointer()
                        );
                }
                else {
                    gl::TexSubImage2D(
                        map_target,
                        image->get_mip_level(),
                        0,
                        0,
                        image->get_width(),
                        image->get_height(),
                        format,
                        type,
                        pixel_accessor.get_pointer()
                        );
                }
                context_opengl::check_opengl_error();
            }
        }

        void texture_opengl::opengl_create_set_magnification_filter(
            texture_filter filter
            ) const
        {
            GLint gl_filter = 0;
            switch (filter) {
            case texture_filter_point:
                gl_filter = gl::NEAREST;
                break;

            case texture_filter_default:
            case texture_filter_linear:
                gl_filter = gl::LINEAR;
                break;

            default:
                throw std::logic_error("unknown texture filter");
            }

            gl::TexParameteri(m_texture_target, gl::TEXTURE_MAG_FILTER, gl_filter);
            context_opengl::check_opengl_error();
        }

        void texture_opengl::opengl_create_set_minification_filter(
            texture_filter filter,
            texture_filter mipmap_filter
            ) const
        {
            GLint gl_filter = 0;
            switch (filter) {
            case texture_filter_point:

                switch (mipmap_filter) {
                case texture_filter_default:
                    gl_filter = gl::NEAREST;
                    break;

                case texture_filter_point:
                    gl_filter = gl::NEAREST_MIPMAP_NEAREST;
                    break;

                case texture_filter_linear:
                    gl_filter = gl::NEAREST_MIPMAP_LINEAR;
                    break;

                default:
                    throw std::logic_error("unknown texture mip filter");
                }
                break;

            case texture_filter_default:
            case texture_filter_linear:

                switch (mipmap_filter) {
                case texture_filter_default:
                    gl_filter = gl::LINEAR;
                    break;

                case texture_filter_point:
                    gl_filter = gl::LINEAR_MIPMAP_NEAREST;
                    break;

                case texture_filter_linear:
                    gl_filter = gl::LINEAR_MIPMAP_LINEAR;
                    break;

                default:
                    throw std::logic_error("unknown texture mip filter");
                }
                break;

            default:
                throw std::logic_error("unknown texture filter");
            }

            gl::TexParameteri(m_texture_target, gl::TEXTURE_MIN_FILTER, gl_filter);
            context_opengl::check_opengl_error();
        }

        void texture_opengl::opengl_create_set_wrap_mode(
            texture_coord_wrap wrap_mode,
            GLenum pname
            ) const
        {
            GLint gl_wrap_mode = 0;
            switch (wrap_mode) {
            case texture_coord_wrap_default:
            case texture_coord_wrap_clamp:
                gl_wrap_mode = gl::CLAMP_TO_EDGE;
                break;

            case texture_coord_wrap_repeat:
                gl_wrap_mode = gl::REPEAT;
                break;

            case texture_coord_wrap_mirror:
                gl_wrap_mode = gl::MIRRORED_REPEAT;
                break;

            default:
                throw std::logic_error("unknown texture coordinate wrap mode");
            }

            gl::TexParameteri(m_texture_target, pname, gl_wrap_mode);
            context_opengl::check_opengl_error();
        }

        void texture_opengl::create_command::execute(
            context_interface* context
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            m_texture->opengl_create(m_texture_desc);

            if (m_finish_sync.is_valid()) {
                context->set_sync_point(m_finish_sync);
            }
        }

        void texture_opengl::destroy_command::execute(
#if !defined(ELECTROSLAG_BUILD_SHIP)
            context_interface* context
#else
            context_interface*
#endif
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            texture_opengl::opengl_destroy(m_texture_id);
        }
    }
}
