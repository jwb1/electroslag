#version 450 core

in vec2 vert_position;
in vec2 vert_tex_coord;
out vec2 out_tex_coord;

void main()
{
    gl_Position = vec4(vert_position, 0.0f, 1.0f);
    out_tex_coord = vert_tex_coord;
}
