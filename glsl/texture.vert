#version 330
in vec3 v_pos;
in vec2 tex_coord;
out vec2 vs_tex_coord;

void main()
{
    gl_Position = vec4(v_pos, 1.0);
    vs_tex_coord = tex_coord;
}
