#version 440 core
in vec2 texture_coordinates;
in vec2 pos_coords;
const float offset = 1.0/300.0;
uniform sampler2D[1] diffuse_maps;
uniform sampler2D[1] spec_maps;
out vec4 color_output;
void main()
{   
    vec2 offsets[9] =
    {
        vec2(-offset, offset),
        vec2(0, offset),
        vec2(offset, offset),
        vec2(-offset, 0),
        vec2(0, 0),
        vec2(offset, 0),
        vec2(-offset, -offset),
        vec2(0, -offset),
        vec2(offset, -offset),
    };
    float sharp_kernel[9] =
    {
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1
    };
    float skernel[9] =
    {
         -1,  2,  -1,
          2, -1,   2,
         -1,  2,  -1
    };
    vec3 col = vec3(0.0);
    for (int i = 0; i<9; i++)
    {
        col += (skernel[i]/3.0)*vec3(texture(diffuse_maps[0], texture_coordinates + offsets[i]));
    }

    color_output = 2*texture(diffuse_maps[0], texture_coordinates); //why does it get darker if I scale down the quad?
}