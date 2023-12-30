#version 460 core
in vec3 tex_coords;
uniform samplerCube cubemap;
out vec4 frag_color;
void main()
{
    frag_color = texture(cubemap, tex_coords);
}