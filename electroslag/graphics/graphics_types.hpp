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

namespace electroslag {
    namespace graphics {
        enum primitive_type {
            primitive_type_unknown = -1,
            primitive_type_triangle,
            primitive_type_triangle_strip,
            primitive_type_triangle_fan,
            primitive_type_count // Ensure this is the last enum entry
        };

        static char const* const primitive_type_strings[primitive_type_count] = {
            "primitive_type_triangle",
            "primitive_type_triangle_strip",
            "primitive_type_triangle_fan"
        };

        namespace primitive_type_util {
            inline int get_element_count(primitive_type prim_type, int prim_count)
            {
                ELECTROSLAG_CHECK(prim_count > 0);

                int element_count = -1;

                switch (prim_type) {
                case graphics::primitive_type_triangle:
                    element_count = prim_count * 3;
                    break;

                case graphics::primitive_type_triangle_strip:
                case graphics::primitive_type_triangle_fan:
                    element_count = prim_count + 2;
                    break;

                default:
                    throw std::invalid_argument("prim_type");
                }

                return (element_count);
            }

            inline GLenum get_opengl_primitive_type(primitive_type prim_type)
            {
                switch (prim_type) {
                case primitive_type_triangle:
                    return (gl::TRIANGLES);

                case primitive_type_triangle_strip:
                    return (gl::TRIANGLE_STRIP);

                case primitive_type_triangle_fan:
                    return (gl::TRIANGLE_FAN);

                default:
                    throw parameter_failure("prim_type");
                }
            }

            inline int get_index_size_from_element_count(int element_count)
            {
                ELECTROSLAG_CHECK(element_count > 0);

                int index_size = 0;
                if (element_count < 255) {
                    index_size = 1;
                }
                else if (element_count < 65536) {
                    index_size = 2;
                }
                else {
                    index_size = 4;
                }

                return (index_size);
            }

            inline GLenum get_opengl_index_size(int sizeof_index)
            {
                switch (sizeof_index) {
                case sizeof(GLubyte) :
                    return (gl::UNSIGNED_BYTE);

                case sizeof(GLushort) :
                    return (gl::UNSIGNED_SHORT);

                case sizeof(GLuint) :
                    return (gl::UNSIGNED_INT);

                default:
                    throw parameter_failure("sizeof_index");
                }
            }
        }

        enum field_kind {
            field_kind_unknown = -1,
            field_kind_attribute,              // Generic vertex attribute; primitive stream and shader indices must match
            field_kind_attribute_position,     // vec3 local space position (x, y, z)
            field_kind_attribute_texcoord2,    // vec2 texture coordinate (u, v)
            field_kind_attribute_normal,       // vec3 vector normal to the surface at the position, normalized (x, y, z)
            field_kind_attribute_normal_basis, // mat3 normal, tangent, bitangent to the surface at the position, each normalized
                                               // [[nx, ny, nz], [tx, ty, tz], [btx, bty, btz]]
            field_kind_uniform,
            field_kind_uniform_local_to_clip,  // mat4 transform local space to clip space
            field_kind_uniform_local_to_world, // mat4 transform local space to world space
            field_kind_count // Ensure this is the last enum entry
        };

        static char const* const field_kind_strings[field_kind_count] = {
            "field_kind_attribute",
            "field_kind_attribute_position",
            "field_kind_attribute_texcoord2",
            "field_kind_attribute_normal",
            "field_kind_attribute_normal_basis",
            "field_kind_uniform",
            "field_kind_uniform_local_to_clip",
            "field_kind_uniform_local_to_world"
        };

        enum field_type {
            field_type_unknown = -1,
            field_type_vec2,
            field_type_vec3,
            field_type_vec4,
            field_type_uvec2,
            field_type_quat,
            field_type_texture_handle,
            field_type_mat3,
            field_type_mat4,
            field_type_count // Ensure this is the last enum entry
        };

        static char const* const field_type_strings[field_type_count] = {
            "field_type_vec2",
            "field_type_vec3",
            "field_type_vec4",
            "field_type_uvec2",
            "field_type_quat",
            "field_type_texture_handle",
            "field_type_mat3",
            "field_type_mat4"
        };

        namespace field_type_util {
            inline int get_order(field_type type)
            {
                switch (type) {
                case field_type_texture_handle:
                    return (1);
                case field_type_vec2:
                case field_type_uvec2:
                    return (2);
                case field_type_mat3:
                case field_type_vec3:
                    return (3);
                case field_type_quat:
                case field_type_vec4:
                case field_type_mat4:
                    return (4);
                default:
                    throw std::logic_error("invalid tag enum value");
                }
            }

            inline int get_bytes(field_type type)
            {
                switch (type) {
                case field_type_vec2:
                    return (sizeof(float) * 2);
                case field_type_vec3:
                    return (sizeof(float) * 3);
                case field_type_quat:
                case field_type_vec4:
                    return (sizeof(float) * 4);
                case field_type_uvec2:
                    return (sizeof(uint32_t) * 2);
                case field_type_texture_handle:
                    return (sizeof(uint64_t));
                case field_type_mat3:
                    return (sizeof(float) * 9);
                case field_type_mat4:
                    return (sizeof(float) * 16);
                default:
                    throw std::logic_error("invalid tag enum value");
                }
            }

            inline GLenum get_opengl_type(field_type type)
            {
                switch (type) {
                case field_type_quat:
                case field_type_vec2:
                case field_type_vec3:
                case field_type_vec4:
                case field_type_mat3:
                case field_type_mat4:
                    return (gl::FLOAT);
                case field_type_uvec2:
                    return (gl::UNSIGNED_INT);
                case field_type_texture_handle:
                    return (gl::TEXTURE); // This is likely not useful for anything. But distinct.
                default:
                    throw std::logic_error("invalid tag enum value");
                }
            }

            inline bool is_vector(field_type type)
            {
                return ((type == field_type_vec2) ||
                        (type == field_type_vec3) ||
                        (type == field_type_vec4) ||
                        (type == field_type_uvec2));
            }

            inline bool is_matrix(field_type type)
            {
                return ((type == field_type_mat3) ||
                        (type == field_type_mat4));
            }
        }

        namespace field_structs {
            struct vec2 : public glm::f32vec2 {
                static constexpr field_type get_field_type() { return (field_type_vec2); }
            };

            struct vec3 : public glm::f32vec3 {
                static constexpr field_type get_field_type() { return (field_type_vec3); }
            };

            struct vec4 : public glm::f32vec4 {
                static constexpr field_type get_field_type() { return (field_type_vec4); }
            };

            struct uvec2 : public glm::u32vec2 {
                static constexpr field_type get_field_type() { return (field_type_uvec2); }
            };

            struct quat : public glm::f32quat {
                static constexpr field_type get_field_type() { return (field_type_quat); }
            };

            struct texture_handle {
                static constexpr field_type get_field_type() { return (field_type_texture_handle); }

                texture_handle()
                    : h(0)
                {}

                uint64_t h;
            };

            struct mat3 : public glm::f32mat3 {
                static constexpr field_type get_field_type() { return (field_type_mat3); }
            };

            struct mat4 : public glm::f32mat4 {
                static constexpr field_type get_field_type() { return (field_type_mat4); }
            };
        }

        enum buffer_memory_map {
            buffer_memory_map_unknown = -1,
            buffer_memory_map_static,
            buffer_memory_map_read,
            buffer_memory_map_write,
            buffer_memory_map_read_write,
            buffer_memory_map_count // Ensure this is the last enum entry
        };

        static char const* const buffer_memory_map_strings[buffer_memory_map_count] = {
            "buffer_memory_map_static",
            "buffer_memory_map_map_read",
            "buffer_memory_map_map_write",
            "buffer_memory_map_map_read_write"
        };

        enum buffer_memory_caching {
            buffer_memory_caching_unknown = -1,
            buffer_memory_caching_static,
            buffer_memory_caching_coherent,
            buffer_memory_caching_noncoherent,
            buffer_memory_caching_count // Ensure this is the last enum entry
        };

        static char const* const buffer_memory_caching_strings[buffer_memory_caching_count] = {
            "buffer_memory_caching_static",
            "buffer_memory_caching_coherent",
            "buffer_memory_caching_noncoherent"
        };

        enum shader_stage {
            shader_stage_unknown = -1,
            shader_stage_vertex,
            shader_stage_tessellation_control,
            shader_stage_tessellation_evaluation,
            shader_stage_geometry,
            shader_stage_fragment,
            shader_stage_compute,
            shader_stage_count // Ensure this is the last enum entry
        };

        static char const* const shader_stage_strings[shader_stage_count] = {
            "shader_stage_vertex",
            "shader_stage_tessellation_control",
            "shader_stage_tessellation_evaluation",
            "shader_stage_geometry",
            "shader_stage_fragment",
            "shader_stage_compute"
        };

        enum shader_stage_bits {
            shader_stage_bits_vertex = (1 << shader_stage_vertex),
            shader_stage_bits_tessellation_control = (1 << shader_stage_tessellation_control),
            shader_stage_bits_tessellation_evaluation = (1 << shader_stage_tessellation_evaluation),
            shader_stage_bits_geometry = (1 << shader_stage_geometry),
            shader_stage_bits_fragment = (1 << shader_stage_fragment),
            shader_stage_bits_compute = (1 << shader_stage_compute)
        };

        static shader_stage_bits const shader_stage_bits_values[shader_stage_count] = {
            shader_stage_bits_vertex,
            shader_stage_bits_tessellation_control,
            shader_stage_bits_tessellation_evaluation,
            shader_stage_bits_geometry,
            shader_stage_bits_fragment,
            shader_stage_bits_compute
        };

        enum texture_type_flags {
            texture_type_flags_normal = 0,
            texture_type_flags_mipmap = 1,
            texture_type_flags_3d = 2,
            texture_type_flags_array = 4,
            texture_type_flags_cube = 8,

            texture_type_flags_count = 5 // Has to be hand edited!
        };

        static char const* const texture_type_flags_strings[texture_type_flags_count] = {
            "texture_type_flags_normal",
            "texture_type_flags_mipmap",
            "texture_type_flags_3d",
            "texture_type_flags_array",
            "texture_type_flags_cube"
        };

        // Convert string array index to bit.
        static texture_type_flags const texture_type_flags_bit_values[texture_type_flags_count] = {
            texture_type_flags_normal,
            texture_type_flags_mipmap,
            texture_type_flags_3d,
            texture_type_flags_array,
            texture_type_flags_cube
        };

        enum texture_color_format {
            texture_color_format_unknown = -1,
            texture_color_format_none,
            texture_color_format_r8,
            texture_color_format_r5g6b5,
            texture_color_format_r8g8b8,
            texture_color_format_r8g8b8_srgb,
            texture_color_format_r8g8b8a8,
            texture_color_format_r8g8b8a8_srgb,
            texture_color_format_dxt1,            // Current beginning of compressed formats
            texture_color_format_dxt1_alpha,
            texture_color_format_dxt3,
            texture_color_format_dxt5,
            texture_color_format_dxt1_srgb,
            texture_color_format_dxt1_alpha_srgb,
            texture_color_format_dxt3_srgb,
            texture_color_format_dxt5_srgb,
            texture_color_format_rgtc1,
            texture_color_format_rgtc1_signed,
            texture_color_format_rgtc2,
            texture_color_format_rgtc2_signed,
            texture_color_format_bptc,
            texture_color_format_bptc_srgb,
            texture_color_format_bptc_sfloat,
            texture_color_format_bptc_ufloat,     // Current end of compressed formats
            texture_color_format_count            // Ensure this is the last enum entry
        };

        static char const* const texture_color_format_strings[texture_color_format_count] = {
            "texture_color_format_none",
            "texture_color_format_r8",
            "texture_color_format_r5g6b5",
            "texture_color_format_r8g8b8",
            "texture_color_format_r8g8b8_srgb",
            "texture_color_format_r8g8b8a8",
            "texture_color_format_r8g8b8a8_srgb",
            "texture_color_format_dxt1",
            "texture_color_format_dxt1_alpha",
            "texture_color_format_dxt3",
            "texture_color_format_dxt5",
            "texture_color_format_dxt1_srgb",
            "texture_color_format_dxt1_alpha_srgb",
            "texture_color_format_dxt3_srgb",
            "texture_color_format_dxt5_srgb",
            "texture_color_format_rgtc1",
            "texture_color_format_rgtc1_signed",
            "texture_color_format_rgtc2",
            "texture_color_format_rgtc2_signed",
            "texture_color_format_bptc",
            "texture_color_format_bptc_srgb",
            "texture_color_format_bptc_sfloat",
            "texture_color_format_bptc_ufloat"
        };

        struct pixel_r8 {
            byte red;
        };

        struct pixel_r8g8b8 {
            byte red;
            byte green;
            byte blue;
        };

        struct pixel_r8g8b8a8 {
            byte red;
            byte green;
            byte blue;
            byte alpha;
        };

        namespace texture_format_util {
            inline bool is_compressed(texture_color_format format)
            {
                if ((format >= texture_color_format_dxt1) && (format <= texture_color_format_bptc_ufloat)) {
                    return (true);
                }
                else {
                    return (false);
                }
            }
        }

        enum texture_cube_face {
            texture_cube_face_unknown = -1,
            // Faces are in the same order as GL layer order, and enum value is == to the
            // GL layer number.
            texture_cube_face_right = 0,        //  +X
            texture_cube_face_left = 1,         //  -X
            texture_cube_face_top = 2,          //  +Y
            texture_cube_face_bottom = 3,       //  -Y
            texture_cube_face_front = 4,        //  +Z
            texture_cube_face_back = 5,         //  -Z
            texture_cube_face_normal = 6,
            texture_cube_face_count // Ensure this is the last enum entry
        };

        static char const* const texture_cube_face_strings[texture_cube_face_count] = {
            "texture_cube_face_right",
            "texture_cube_face_left",
            "texture_cube_face_top",
            "texture_cube_face_bottom",
            "texture_cube_face_front",
            "texture_cube_face_back",
            "texture_cube_face_normal"
        };

        enum texture_filter {
            texture_filter_unknown = -1,
            texture_filter_default,
            texture_filter_point,
            texture_filter_linear,
            texture_filter_count // Ensure this is the last enum entry
        };

        static char const* const texture_filter_strings[texture_filter_count] = {
            "texture_filter_default",
            "texture_filter_point",
            "texture_filter_linear"
        };

        enum texture_coord_wrap {
            texture_coord_wrap_unknown = -1,
            texture_coord_wrap_default,
            texture_coord_wrap_clamp,
            texture_coord_wrap_repeat,
            texture_coord_wrap_mirror,
            texture_coord_wrap_count // Ensure this is the last enum entry
        };

        static char const* const texture_coord_wrap_strings[texture_coord_wrap_count] = {
            "texture_coord_wrap_default",
            "texture_coord_wrap_clamp",
            "texture_coord_wrap_repeat",
            "texture_coord_wrap_mirror"
        };

        struct sampler_params {
            sampler_params()
                : magnification_filter(texture_filter_default)
                , minification_filter(texture_filter_default)
                , mip_filter(texture_filter_default)
                , s_wrap_mode(texture_coord_wrap_default)
                , t_wrap_mode(texture_coord_wrap_default)
                , u_wrap_mode(texture_coord_wrap_default)
            {}

            texture_filter magnification_filter;
            texture_filter minification_filter;
            texture_filter mip_filter;

            texture_coord_wrap s_wrap_mode;
            texture_coord_wrap t_wrap_mode;
            texture_coord_wrap u_wrap_mode;
        };

        enum texture_level_generate {
            texture_level_generate_unknown = -1,
            texture_level_generate_off,
            texture_level_generate_on,
            texture_level_generate_favor_speed,
            texture_level_generate_favor_quality,
            texture_level_generate_count // Ensure this is the last enum entry
        };

        static char const* const texture_level_generate_strings[texture_level_generate_count] = {
            "texture_level_generate_off",
            "texture_level_generate_on",
            "texture_level_generate_favor_speed",
            "texture_level_generate_favor_quality"
        };

        enum frame_buffer_type {
            frame_buffer_type_unknown = -1,
            frame_buffer_type_display,
            frame_buffer_type_off_screen
        };

        enum frame_buffer_color_format {
            frame_buffer_color_format_unknown = -1,
            frame_buffer_color_format_none,
            frame_buffer_color_format_r8g8b8a8,
            frame_buffer_color_format_r8g8b8a8_srgb,

            frame_buffer_color_format_count // Ensure this is the last enum entry
        };

        static char const* const frame_buffer_color_format_strings[frame_buffer_color_format_count] = {
            "frame_buffer_color_format_none",
            "frame_buffer_color_format_r8g8b8a8",
            "frame_buffer_color_format_r8g8b8a8_srgb"
        };

        enum frame_buffer_depth_stencil_format {
            frame_buffer_depth_stencil_format_unknown = -1,
            frame_buffer_depth_stencil_format_none,
            frame_buffer_depth_stencil_format_d16,
            frame_buffer_depth_stencil_format_d24,
            frame_buffer_depth_stencil_format_d32,
            frame_buffer_depth_stencil_format_d24s8,

            frame_buffer_depth_stencil_format_count // Ensure this is the last enum entry
        };

        static char const* const frame_buffer_depth_stencil_format_strings[frame_buffer_depth_stencil_format_count] = {
            "frame_buffer_depth_stencil_format_none",
            "frame_buffer_depth_stencil_format_d16",
            "frame_buffer_depth_stencil_format_d24",
            "frame_buffer_depth_stencil_format_d32",
            "frame_buffer_depth_stencil_format_d24s8"
        };

        enum frame_buffer_msaa {
            frame_buffer_msaa_unknown = -1,
            frame_buffer_msaa_none,
            frame_buffer_msaa_2,
            frame_buffer_msaa_4,
            frame_buffer_msaa_6,
            frame_buffer_msaa_8,
            frame_buffer_msaa_16,

            frame_buffer_msaa_count // Ensure this is the last enum entry
        };

        static char const* const frame_buffer_msaa_strings[frame_buffer_msaa_count] = {
            "frame_buffer_msaa_none",
            "frame_buffer_msaa_2",
            "frame_buffer_msaa_4",
            "frame_buffer_msaa_6",
            "frame_buffer_msaa_8",
            "frame_buffer_msaa_16"
        };

        struct frame_buffer_attribs {
            frame_buffer_attribs()
                : color_format(frame_buffer_color_format_unknown)
                , msaa(frame_buffer_msaa_unknown)
                , depth_stencil_format(frame_buffer_depth_stencil_format_unknown)
                , depth_stencil_shadow_hint(false)
            {}

            frame_buffer_color_format color_format;
            frame_buffer_msaa msaa;
            frame_buffer_depth_stencil_format depth_stencil_format;
            bool depth_stencil_shadow_hint;
        };

        enum depth_test_mode {
            depth_test_mode_unknown = -1,
            depth_test_mode_never = 0,
            depth_test_mode_less = 1,
            depth_test_mode_equal = 2,
            depth_test_mode_lequal = 3,
            depth_test_mode_greater = 4,
            depth_test_mode_notequal = 5,
            depth_test_mode_gequal = 6, 
            depth_test_mode_always = 7,
            depth_test_mode_count // Ensure this is the last enum entry
        };

        static char const* const depth_test_mode_strings[depth_test_mode_count] = {
            "depth_test_mode_never",
            "depth_test_mode_less",
            "depth_test_mode_equal",
            "depth_test_mode_lequal",
            "depth_test_mode_greater",
            "depth_test_mode_notequal",
            "depth_test_mode_gequal",
            "depth_test_mode_always"
        };

        struct depth_test_params {
            depth_test_params()
                : test_mode(depth_test_mode_unknown)
                , write_enable(false)
                , test_enable(false)
                , reserved(0)
            {}

            bool operator ==(depth_test_params const& compare_with) const
            {
                return (value == compare_with.value);
            }

            bool operator !=(depth_test_params const& compare_with) const
            {
                return (value != compare_with.value);
            }

            union {
                unsigned int value;
                struct {
                    depth_test_mode test_mode:4;
                    ELECTROSLAG_ENUM_BITPACK_CHECK(depth_test_mode, 4);
                    // Can't make these "bool"s or else the struct doesn't bitpack on Visual C++
                    // https://randomascii.wordpress.com/2010/06/06/bit-field-packing-with-visual-c/
                    unsigned int write_enable:1;
                    unsigned int test_enable:1;
                    unsigned int reserved:26;
                };
            };
        };
        ELECTROSLAG_STATIC_CHECK(sizeof(depth_test_params) == sizeof(unsigned int), "Bit packing check");

        enum blending_mode {
            blending_mode_unknown = -1,
            blending_mode_add = 0,
            blending_mode_subtract = 1,
            blending_mode_reverse_subtract = 2,
            blending_mode_min = 3,
            blending_mode_max = 4,
            blending_mode_count // Ensure this is the last enum entry
        };

        static char const* const blending_mode_strings[blending_mode_count] = {
            "blending_mode_add",
            "blending_mode_subtract",
            "blending_mode_reverse_subtract",
            "blending_mode_min",
            "blending_mode_max"
        };

        enum blending_operand {
            blending_operand_unknown = -1,
            blending_operand_zero = 0,
            blending_operand_one = 1,
            blending_operand_src_color = 2,
            blending_operand_one_minus_src_color = 3,
            blending_operand_dst_color = 4,
            blending_operand_one_minus_dst_color = 5,
            blending_operand_src_alpha = 6,
            blending_operand_one_minus_src_alpha = 7,
            blending_operand_dst_alpha = 8,
            blending_operand_one_minus_dst_alpha = 9,
            blending_operand_constant_color = 10,
            blending_operand_one_minus_constant_color = 11,
            blending_operand_constant_alpha = 12,
            blending_operand_one_minus_constant_alpha = 13,
            blending_operand_src_alpha_saturate = 14,
            blending_operand_src1_color = 15,
            blending_operand_one_minus_src1_color = 16,
            blending_operand_src1_alpha = 17,
            blending_operand_one_minus_src1_alpha = 18,
            blending_operand_count // Ensure this is the last enum entry
        };

        static char const* const blending_operand_strings[blending_operand_count] = {
            "blending_operand_zero",
            "blending_operand_one",
            "blending_operand_src_color",
            "blending_operand_one_minus_src_color",
            "blending_operand_dst_color",
            "blending_operand_one_minus_dst_color",
            "blending_operand_src_alpha",
            "blending_operand_one_minus_src_alpha",
            "blending_operand_dst_alpha",
            "blending_operand_one_minus_dst_alpha",
            "blending_operand_constant_color",
            "blending_operand_one_minus_constant_color",
            "blending_operand_constant_alpha",
            "blending_operand_one_minus_constant_alpha",
            "blending_operand_src_alpha_saturate",
            "blending_operand_src1_color",
            "blending_operand_one_minus_src1_color",
            "blending_operand_src1_alpha",
            "blending_operand_one_minus_src1_alpha"
        };

        struct blending_params {
            blending_params()
                : color_mode(blending_mode_unknown)
                , color_op1(blending_operand_unknown)
                , color_op2(blending_operand_unknown)
                , alpha_mode(blending_mode_unknown)
                , alpha_op1(blending_operand_unknown)
                , alpha_op2(blending_operand_unknown)
                , enable(false)
                , reserved(0)
            {}

            bool operator ==(blending_params const& compare_with) const
            {
                return (value == compare_with.value);
            }

            bool operator !=(blending_params const& compare_with) const
            {
                return (value != compare_with.value);
            }

            union {
                unsigned int value;
                struct {
                    blending_mode color_mode:3;
                    ELECTROSLAG_ENUM_BITPACK_CHECK(blending_mode, 3);
                    blending_operand color_op1:5;
                    ELECTROSLAG_ENUM_BITPACK_CHECK(blending_operand, 5);
                    blending_operand color_op2:5;
                    ELECTROSLAG_ENUM_BITPACK_CHECK(blending_operand, 5);

                    blending_mode alpha_mode:3;
                    ELECTROSLAG_ENUM_BITPACK_CHECK(blending_mode, 3);
                    blending_operand alpha_op1:5;
                    ELECTROSLAG_ENUM_BITPACK_CHECK(blending_operand, 5);
                    blending_operand alpha_op2:5;
                    ELECTROSLAG_ENUM_BITPACK_CHECK(blending_operand, 5);

                    unsigned int enable:1;
                    unsigned int reserved:5;
                };
            };
        };
        ELECTROSLAG_STATIC_CHECK(sizeof(blending_params) == sizeof(unsigned int), "Bit packing check");

        struct graphics_initialize_params {
            graphics_initialize_params()
                : swap_interval(0)
            {}

            frame_buffer_attribs display_attribs;
            int swap_interval;
        };

        class context_interface;
        class command {
        public:
            virtual ~command()
            {}

            virtual void execute(context_interface* context) = 0;
        };

        // Internal use stuff. Maybe this needs to be in a different header file.
        typedef GLuint opengl_object_id;
    }
}

#if defined(_MSC_VER)
// Force the NVidia GPU on Optimus systems
extern "C" {
    __declspec(dllexport, selectany) DWORD NvOptimusEnablement = 0x00000001;
}
#endif
