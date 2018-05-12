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
#include "electroslag/logger.hpp"
#include "electroslag/graphics/shader_program_opengl.hpp"
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/graphics/context_opengl.hpp"

namespace electroslag {
    namespace graphics {
        void shader_program_opengl::schedule_async_create(
            shader_program_descriptor::ref const& shader_desc,
            shader_field_map::ref const& vertex_attrib_field_map
            )
        {
            graphics_interface* g = get_graphics();

            if (g->get_render_thread()->is_running()) {
                opengl_create(shader_desc, vertex_attrib_field_map);
            }
            else {
                g->get_render_policy()->get_system_command_queue()->enqueue_command<create_command>(
                    ref(this),
                    shader_desc,
                    vertex_attrib_field_map
                    );
            }
        }

        void shader_program_opengl::schedule_async_create_finished(
            shader_program_descriptor::ref const& shader_desc,
            shader_field_map::ref const& vertex_attrib_field_map
            )
        {
            graphics_interface* g = get_graphics();
            ELECTROSLAG_CHECK(!g->get_render_thread()->is_running());

            sync_interface::ref finish_sync = g->create_sync();

            g->get_render_policy()->get_system_command_queue()->enqueue_command<create_command>(
                ref(this),
                shader_desc,
                vertex_attrib_field_map,
                finish_sync
                );

            finish_sync->wait();
        }

        shader_program_opengl::~shader_program_opengl()
        {
            if (m_is_finished.load(std::memory_order_acquire)) {
                m_is_finished.store(false, std::memory_order_release);

                graphics_interface* g = get_graphics();

                if (g->get_render_thread()->is_running()) {
                    opengl_destroy(
                        m_vertex_part,
                        m_tessellation_control_part,
                        m_tessellation_evaluation_part,
                        m_geometry_part,
                        m_fragment_part,
                        m_compute_part,
                        m_program
                        );
                }
                else {
                    // This command can run completely asynchronously to "this" objects destruction.
                    g->get_render_policy()->get_system_command_queue()->enqueue_command<destroy_command>(
                        m_vertex_part,
                        m_tessellation_control_part,
                        m_tessellation_evaluation_part,
                        m_geometry_part,
                        m_fragment_part,
                        m_compute_part,
                        m_program
                        );
                }

                m_vertex_part = 0;
                m_tessellation_control_part = 0;
                m_tessellation_evaluation_part = 0;
                m_geometry_part = 0;
                m_fragment_part = 0;
                m_compute_part = 0;
                m_program = 0;
            }
            else {
                ELECTROSLAG_LOG_WARN("shader_program_opengl destructor before finished");
            }
        }

        // static
        void shader_program_opengl::opengl_destroy(
            opengl_object_id vertex_part,
            opengl_object_id tessellation_control_part,
            opengl_object_id tessellation_evaluation_part,
            opengl_object_id geometry_part,
            opengl_object_id fragment_part,
            opengl_object_id compute_part,
            opengl_object_id program
            )
        {
            if (program) {
                gl::DeleteProgram(program);
            }
            if (vertex_part) {
                gl::DeleteShader(vertex_part);
            }
            if (tessellation_control_part) {
                gl::DeleteShader(tessellation_control_part);
            }
            if (tessellation_evaluation_part) {
                gl::DeleteShader(tessellation_evaluation_part);
            }
            if (geometry_part) {
                gl::DeleteShader(geometry_part);
            }
            if (fragment_part) {
                gl::DeleteShader(fragment_part);
            }
            if (compute_part) {
                gl::DeleteShader(compute_part);
            }
        }

        void shader_program_opengl::opengl_create(
            shader_program_descriptor::ref const& shader_desc,
            shader_field_map::ref const& vertex_attrib_field_map
            )
        {
            ELECTROSLAG_CHECK(!is_finished());

            m_descriptor = shader_program_descriptor::clone(shader_desc);

            opengl_compile_shader_program(m_descriptor);
            opengl_set_attrib_metadata(m_descriptor, vertex_attrib_field_map);
            opengl_link_shader_program();
            opengl_gather_ubo_metadata(m_descriptor);
            opengl_finish_create(m_descriptor);
        }

        void shader_program_opengl::opengl_compile_shader_program(
            shader_program_descriptor::ref& shader_desc
            )
        {
            std::string shader_defines;
            if (shader_desc->get_shader_defines().is_valid()) {
                referenced_buffer_interface::accessor shader_define_accessor(
                    shader_desc->get_shader_defines().cast<referenced_buffer_interface>()
                    );
                shader_defines.append(
                    static_cast<char const*>(shader_define_accessor.get_pointer()),
                    shader_define_accessor.get_sizeof()
                    );
            }

            opengl_compile_shader_stage(shader_desc->get_vertex_shader(), &m_vertex_part, shader_defines);
            opengl_compile_shader_stage(shader_desc->get_tessellation_control_shader(), &m_tessellation_control_part, shader_defines);
            opengl_compile_shader_stage(shader_desc->get_tessellation_evaluation_shader(), &m_tessellation_evaluation_part, shader_defines);
            opengl_compile_shader_stage(shader_desc->get_geometry_shader(), &m_geometry_part, shader_defines);
            opengl_compile_shader_stage(shader_desc->get_fragment_shader(), &m_fragment_part, shader_defines);
            opengl_compile_shader_stage(shader_desc->get_compute_shader(), &m_compute_part, shader_defines);

            opengl_object_id program = gl::CreateProgram();
            context_opengl::check_opengl_error();

            if (m_vertex_part) {
                gl::AttachShader(program, m_vertex_part);
                context_opengl::check_opengl_error();
            }

            if (m_tessellation_control_part) {
                gl::AttachShader(program, m_tessellation_control_part);
                context_opengl::check_opengl_error();
            }

            if (m_tessellation_evaluation_part) {
                gl::AttachShader(program, m_tessellation_evaluation_part);
                context_opengl::check_opengl_error();
            }

            if (m_geometry_part) {
                gl::AttachShader(program, m_geometry_part);
                context_opengl::check_opengl_error();
            }

            if (m_fragment_part) {
                gl::AttachShader(program, m_fragment_part);
                context_opengl::check_opengl_error();
            }

            if (m_compute_part) {
                gl::AttachShader(program, m_compute_part);
                context_opengl::check_opengl_error();
            }

            ELECTROSLAG_CHECK(!m_program);
            m_program = program;
        }

        // static
        void shader_program_opengl::opengl_compile_shader_stage(
            shader_stage_descriptor::ref& stage_descriptor,
            opengl_object_id* output_part,
            std::string const& shader_defines
            )
        {
            static GLenum const shader_stage_to_gl_type[shader_stage_count] = {
                gl::VERTEX_SHADER,
                gl::TESS_CONTROL_SHADER,
                gl::TESS_EVALUATION_SHADER,
                gl::GEOMETRY_SHADER,
                gl::FRAGMENT_SHADER,
                gl::COMPUTE_SHADER
            };

            ELECTROSLAG_CHECK(output_part);
            ELECTROSLAG_CHECK(!(*output_part));

            if ((!stage_descriptor.is_valid()) || (!stage_descriptor->has_source())) {
                return;
            }

            opengl_object_id shader_part = gl::CreateShader(shader_stage_to_gl_type[stage_descriptor->get_stage_flag()]);
            context_opengl::check_opengl_error();

            *output_part = shader_part;

            {
                referenced_buffer_interface::accessor source_accessor(stage_descriptor->get_source());
                if (shader_defines.empty()) {
                    char const* shader_source = reinterpret_cast<char const*>(source_accessor.get_pointer());
                    int shader_source_length = source_accessor.get_sizeof() - 1;
                    gl::ShaderSource(shader_part, 1, &shader_source, &shader_source_length);
                }
                else {
                    // Looking to inject defines AFTER version statement, but BEFORE anything else.
                    char const* shader_source[3];
                    int shader_source_length[3];

                    shader_source[0] = reinterpret_cast<char const*>(source_accessor.get_pointer());
                    if (std::strncmp(shader_source[0], "#version", 8) != 0) {
                        throw load_object_failure("GLSL shader source does not start with #version");
                    }

                    int version_statement_length = 8;
                    while (shader_source[0][version_statement_length] != '\n') {
                        ++version_statement_length;
                    }

                    shader_source_length[0] = version_statement_length;

                    shader_source[1] = shader_defines.c_str();
                    shader_source_length[1] = static_cast<int>(shader_defines.length());

                    shader_source[2] = shader_source[0] + version_statement_length;
                    shader_source_length[2] = source_accessor.get_sizeof() - (version_statement_length + 1);

                    gl::ShaderSource(shader_part, 3, shader_source, shader_source_length);
                }
            }
            context_opengl::check_opengl_error();

            gl::CompileShader(shader_part);
            context_opengl::check_opengl_error();

            int compile_status = 0;
            gl::GetShaderiv(shader_part, gl::COMPILE_STATUS, &compile_status);
            if (!compile_status) {
                int compile_log_length = 0;
                gl::GetShaderiv(shader_part, gl::INFO_LOG_LENGTH, &compile_log_length);

                char* compile_log = static_cast<char*>(alloca(
                    compile_log_length * sizeof(char)
                    ));

                gl::GetShaderInfoLog(shader_part, compile_log_length, 0, compile_log);

                throw opengl_api_failure(compile_log, 0);
            }

#if !defined(ELECTROSLAG_BUILD_SHIP)
            if (stage_descriptor->has_name_string()) {
                std::string stage_name(stage_descriptor->get_name());
                gl::ObjectLabel(
                    gl::SHADER,
                    shader_part,
                    static_cast<GLsizei>(stage_name.length()),
                    stage_name.c_str()
                    );
                context_opengl::check_opengl_error();
            }
#endif
        }

        void shader_program_opengl::opengl_set_attrib_metadata(
            shader_program_descriptor::ref& shader_desc,
            shader_field_map::ref const& vertex_attrib_field_map
            )
        {
            // Each vertex attribute has a metadata entry including either an explicit
            // index or a primitive stream type to match.
            shader_field_map::ref& field_map(shader_desc->get_vertex_fields());
            shader_field_map::iterator sf(field_map->begin());
            while (sf != field_map->end()) {
                shader_field* field = sf->second;

                // Determine a new index to assign to this attrib in the shader.
                field_kind attrib_kind = field->get_kind();
                int attrib_location = -1;
                switch (attrib_kind) {
                case field_kind_attribute:
                    // Usually we would expect field_kind_attribute to have an index
                    // assigned in the shader. Assume the metadata index must be right.
                    attrib_location = field->get_index();
                    break;

                case field_kind_attribute_position:
                case field_kind_attribute_texcoord2:
                case field_kind_attribute_normal: {
                    // Look for a match in the primitive stream to assign the index.
                    ELECTROSLAG_CHECK(vertex_attrib_field_map.is_valid());

                    shader_field_map::const_iterator pf(vertex_attrib_field_map->begin());
                    while (pf != vertex_attrib_field_map->end()) {
                        shader_field* prim_stream_field = pf->second;

                        if (prim_stream_field->get_kind() == attrib_kind) {
                            ELECTROSLAG_CHECK(field->get_field_type() == prim_stream_field->get_field_type());
                            attrib_location = prim_stream_field->get_index();
                            break;
                        }
                        ++pf;
                    }
                    if (pf == vertex_attrib_field_map->end()) {
                        throw load_object_failure("Could not match field kind");
                    }
                    break;
                }

                default:
                    throw load_object_failure("Invalid vertex attributes in shader");
                }

                // Assign the attrib.
                gl::BindAttribLocation(m_program, attrib_location, field->get_name().c_str());
                context_opengl::check_opengl_error();
                field->set_index(attrib_location);

                ++sf;
            }
        }

        void shader_program_opengl::opengl_link_shader_program()
        {
            gl::LinkProgram(m_program);
            context_opengl::check_opengl_error();

            int link_status = 0;
            gl::GetProgramiv(m_program, gl::LINK_STATUS, &link_status);
            if (!link_status) {
                int link_log_length = 0;
                gl::GetProgramiv(m_program, gl::INFO_LOG_LENGTH, &link_log_length);

                char* link_log = static_cast<char*>(alloca(
                    link_log_length * sizeof(char)
                ));

                gl::GetProgramInfoLog(m_program, link_log_length, 0, link_log);

                throw opengl_api_failure(link_log, 0);
            }

            gl::ValidateProgram(m_program);
            context_opengl::check_opengl_error();

            int program_validated = 0;
            gl::GetProgramiv(m_program, gl::VALIDATE_STATUS, &program_validated);
            if (!program_validated) {
                throw opengl_api_failure("shader failed validation", 0);
            }

            gl::UseProgram(m_program);
            context_opengl::check_opengl_error();
        }

        void shader_program_opengl::opengl_gather_ubo_metadata(
            shader_program_descriptor::ref& shader_desc
            )
        {
            if (m_vertex_part) {
                opengl_gather_stage_ubo_metadata(shader_desc->get_vertex_shader());
            }

            if (m_tessellation_control_part) {
                opengl_gather_stage_ubo_metadata(shader_desc->get_tessellation_control_shader());
            }

            if (m_tessellation_evaluation_part) {
                opengl_gather_stage_ubo_metadata(shader_desc->get_tessellation_evaluation_shader());
            }

            if (m_geometry_part) {
                opengl_gather_stage_ubo_metadata(shader_desc->get_geometry_shader());
            }

            if (m_fragment_part) {
                opengl_gather_stage_ubo_metadata(shader_desc->get_fragment_shader());
            }

            if (m_compute_part) {
                opengl_gather_stage_ubo_metadata(shader_desc->get_compute_shader());
            }
        }

        void shader_program_opengl::opengl_gather_stage_ubo_metadata(
            shader_stage_descriptor::ref& stage_descriptor
            )
        {
            static GLenum const shader_stage_to_referenced_by[shader_stage_count] = {
                gl::REFERENCED_BY_VERTEX_SHADER,
                gl::REFERENCED_BY_TESS_CONTROL_SHADER,
                gl::REFERENCED_BY_TESS_EVALUATION_SHADER,
                gl::REFERENCED_BY_GEOMETRY_SHADER,
                gl::REFERENCED_BY_FRAGMENT_SHADER,
                gl::REFERENCED_BY_COMPUTE_SHADER
            };

            ELECTROSLAG_CHECK(stage_descriptor.is_valid());

            shader_stage stage = stage_descriptor->get_stage_flag();
            ELECTROSLAG_CHECK(stage_descriptor->get_uniform_buffer_count() <=
                get_graphics()->get_context_capability()->get_max_stage_uniform_bindings(stage));

            // Gather information on the UBOs referenced by this stage.
            shader_stage_descriptor::uniform_buffer_iterator u(stage_descriptor->begin_uniform_buffers());
            while (u != stage_descriptor->end_uniform_buffers()) {
                uniform_buffer_descriptor::ref& ubo(*u);

                // Per UBO parameters; match the UBO "block binding" to the "block index" which GL assigns.
                int gl_index = static_cast<int>(gl::GetUniformBlockIndex(m_program, ubo->get_name().c_str()));
                context_opengl::check_opengl_error();

                gl::UniformBlockBinding(m_program, gl_index, gl_index);
                context_opengl::check_opengl_error();
                ubo->set_binding(gl_index);

                // Get the UBO's size
                GLenum const ubo_props[1] = { gl::BUFFER_DATA_SIZE };
                GLint ubo_params[1] = { 0 };
                GLsizei out_length = 0;
                gl::GetProgramResourceiv(
                    m_program,
                    gl::UNIFORM_BLOCK,
                    ubo->get_binding(),
                    _countof(ubo_props),
                    ubo_props,
                    _countof(ubo_params),
                    &out_length,
                    ubo_params
                    );
                ELECTROSLAG_CHECK(out_length == _countof(ubo_params));
                context_opengl::check_opengl_error();
                ubo->set_size(ubo_params[0]);

                // Per field parameters for each ubo.
                GLenum const field_props[2] = { shader_stage_to_referenced_by[stage], gl::OFFSET };

                shader_field_map::ref& ubo_field_map(ubo->get_fields());
                shader_field_map::iterator f(ubo_field_map->begin());
                while (f != ubo_field_map->end()) {
                    shader_field* field = f->second;

                    std::string resource_name(ubo->get_name());
                    resource_name.append(".");
                    resource_name.append(field->get_name());

                    GLuint resource_index = gl::GetProgramResourceIndex(m_program, gl::UNIFORM, resource_name.c_str());
                    ELECTROSLAG_CHECK((resource_index != gl::INVALID_INDEX) && (resource_index != gl::INVALID_ENUM));
                    context_opengl::check_opengl_error();

                    GLint field_params[2] = { 0 };
                    out_length = 0;
                    gl::GetProgramResourceiv(
                        m_program,
                        gl::UNIFORM,
                        resource_index,
                        _countof(field_props),
                        field_props,
                        _countof(field_params),
                        &out_length,
                        field_params
                        );
                    ELECTROSLAG_CHECK(out_length == _countof(field_params));
                    context_opengl::check_opengl_error();

                    if (field_params[0]) {
                        field->set_active(true);
                        field->set_offset(field_params[1]);
                    }
                    else {
                        field->set_active(false);
                    }

                    ++f;
                }

                ++u;
            }
        }

        void shader_program_opengl::opengl_finish_create(
            shader_program_descriptor::ref& shader_desc
            )
        {
#if !defined(ELECTROSLAG_BUILD_SHIP)
            if (shader_desc->has_name_string()) {
                std::string shader_name(shader_desc->get_name());
                gl::ObjectLabel(
                    gl::PROGRAM,
                    m_program,
                    static_cast<GLsizei>(shader_name.length()),
                    shader_name.c_str()
                );
                context_opengl::check_opengl_error();
            }
#else
            UNREFERENCED_PARAMETER(shader_desc);
#endif
            m_is_finished.store(true, std::memory_order_release);
        }

        void shader_program_opengl::create_command::execute(
            context_interface* context
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            m_shader->opengl_create(m_shader_desc, m_vertex_attrib_field_map);

            if (m_finish_sync.is_valid()) {
                context->set_sync_point(m_finish_sync);
            }
        }

        void shader_program_opengl::destroy_command::execute(
#if !defined(ELECTROSLAG_BUILD_SHIP)
            context_interface* context
#else
            context_interface*
#endif
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            shader_program_opengl::opengl_destroy(
                m_vertex_part,
                m_tessellation_control_part,
                m_tessellation_evaluation_part,
                m_geometry_part,
                m_fragment_part,
                m_compute_part,
                m_program
                );
        }

        void shader_program_opengl::bind() const
        {
            get_graphics()->get_render_thread()->check();
            ELECTROSLAG_CHECK(is_finished());

            gl::UseProgram(m_program);
            context_opengl::check_opengl_error();
        }

#if !defined(ELECTROSLAG_BUILD_SHIP)
        // static
        void shader_program_opengl::opengl_interface_query(opengl_object_id program)
        {
            int active_uniforms = 0;
            gl::GetProgramInterfaceiv(program, gl::UNIFORM, gl::ACTIVE_RESOURCES, &active_uniforms);
            context_opengl::check_opengl_error();
            ELECTROSLAG_LOG_GFX("GL_UNIFORM interface (%d active resources)", active_uniforms);
            ELECTROSLAG_LOG_GFX(
                " NAME"
                " TYPE"
                " ARRAY_SIZE"
                " OFFSET"
                " BLOCK_INDEX"
                " ARRAY_STRIDE"
                " MATRIX_STRIDE"
                " IS_ROW_MAJOR"
                " ATOMIC_COUNTER_BUFFER_INDEX"
                " REFERENCED_BY_VERTEX_SHADER"
                " REFERENCED_BY_TESS_CONTROL_SHADER"
                " REFERENCED_BY_TESS_EVALUATION_SHADER"
                " REFERENCED_BY_GEOMETRY_SHADER"
                " REFERENCED_BY_FRAGMENT_SHADER"
                " REFERENCED_BY_COMPUTE_SHADER"
                " LOCATION"
                );

            GLenum const uniform_props[16] = {
                gl::NAME_LENGTH,
                gl::TYPE,
                gl::ARRAY_SIZE,
                gl::OFFSET,
                gl::BLOCK_INDEX,
                gl::ARRAY_STRIDE,
                gl::MATRIX_STRIDE,
                gl::IS_ROW_MAJOR,
                gl::ATOMIC_COUNTER_BUFFER_INDEX,
                gl::REFERENCED_BY_VERTEX_SHADER,
                gl::REFERENCED_BY_TESS_CONTROL_SHADER,
                gl::REFERENCED_BY_TESS_EVALUATION_SHADER,
                gl::REFERENCED_BY_GEOMETRY_SHADER,
                gl::REFERENCED_BY_FRAGMENT_SHADER,
                gl::REFERENCED_BY_COMPUTE_SHADER,
                gl::LOCATION
            };

            for (int uniform_index = 0; uniform_index < active_uniforms; ++uniform_index) {
                int uniform_params[16] = { 0 };
                gl::GetProgramResourceiv(program, gl::UNIFORM, uniform_index, _countof(uniform_props), uniform_props, _countof(uniform_params), 0, uniform_params);
                context_opengl::check_opengl_error();

                int uniform_name_length = uniform_params[0];
                char* uniform_name = static_cast<char*>(::operator new(uniform_name_length));
                gl::GetProgramResourceName(program, gl::UNIFORM, uniform_index, uniform_name_length, 0, uniform_name);
                context_opengl::check_opengl_error();

                ELECTROSLAG_LOG_GFX(
                    " %s"
                    " %d %d %d %d"
                    " %d %d %d %d"
                    " %d %d %d %d"
                    " %d %d %d",
                    uniform_name,
                    uniform_params[1], uniform_params[2], uniform_params[3], uniform_params[4],
                    uniform_params[5], uniform_params[6], uniform_params[7], uniform_params[8],
                    uniform_params[9], uniform_params[10], uniform_params[11], uniform_params[12],
                    uniform_params[13], uniform_params[14], uniform_params[15]
                    );
                delete uniform_name;
            }
        }
#endif
    }
}
