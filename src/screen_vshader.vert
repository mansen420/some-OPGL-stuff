#version 440 core
layout (location = 0) in vec2 screen_coords;
layout (location = 2) in vec2 tex_coords;

out vec2 texture_coordinates;
out vec2 pos_coords;

uniform mat4 model_transform;
void main()
{
    pos_coords = screen_coords;
    texture_coordinates = tex_coords;
    gl_Position = model_transform*vec4(screen_coords, 0 , 1.0);
}