#version 460 core
layout (location = 0) in vec3 cube_pos;

uniform mat4 projection_transform, view_transform;
uniform mat4 model_transform;

out vec3 tex_coords;
void main()
{   
    tex_coords = cube_pos;
    mat4 view_transform_no_translate = mat4(mat3(view_transform));
    gl_Position = projection_transform*view_transform_no_translate*vec4(cube_pos, 1.0);
}