#version 450 core
#extension GL_ARB_gpu_shader_int64 : require
#define texture_handle uint64_t

in vec2 tex_coord;
out vec4 frag_color;

uniform _loading_screen_f_uniforms {
    texture_handle loading_screen_texture;
} loading_screen_f_uniforms;

void main()
{
    frag_color = texture2D(sampler2D(loading_screen_f_uniforms.loading_screen_texture), tex_coord);
}
