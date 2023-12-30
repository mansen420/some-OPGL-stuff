#ifndef OBJECT_INTERFACE
#define OBJECT_INTERFACE

#define VS_TRNSFRM_MDL_NAME "model_transform"

#define TINYOBJLOADER_IMPLEMENTATION ;
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glad/glad.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <iostream>
#include <string>

bool gen_texture(const char* file_path, unsigned int &tex_id);

namespace object_3D
{
    using namespace glm;
    struct vertex
    {
        vec3 pos_coords;
        vec3 normal_coords;
        vec2 tex_coords;
    };
    using std::vector;
    using std::string;
    class drawable
    {
    protected:
        //assigns textures IDs, if any, to samplers. note that most recently assigned value will apply if you leave this function empty.
        virtual void set_samplers(const unsigned int &program_id) const = 0;
        //assigns model transform matrix to vertex shader. note that most recently assigned value will apply if you leave this function empty.
        virtual void send_model_transform(const unsigned int &program_id) const = 0;
        //Binds VAOs, if any, for the draw call.
        virtual void bind_VAO() const = 0;
        //overridable draw command. this function should handle the drawing of your object(s).
        virtual void gl_draw(const unsigned int &program_id) const = 0;
        //sends all uniforms to the shader programs 
        virtual void send_uniforms(const unsigned int &program_id) const final 
        {
            glUseProgram(program_id);
            send_model_transform(program_id);
            set_samplers(program_id);
        }
    public:
        //generates VAO(s) and/or sends buffer data.
        virtual void send_data() = 0;

        //caller must ensure that send_data() has been called before this. 
        virtual void draw(const unsigned int &program_id) const final 
        {
            send_uniforms(program_id);
            bind_VAO();
            gl_draw(program_id);
            glBindVertexArray(0);
        }
    };

    enum texture_type_option
    {
        DIFFUSE,
        SPECULAR
    };
    struct texture
    {
        unsigned int id = 0;   //id of 0 implies non-existence
        texture_type_option type;
    };
    struct material
    {
        texture diffuse_map;
        texture spec_map;
        material() {spec_map.type=SPECULAR, diffuse_map.type=DIFFUSE;}
    };
    
    class mesh : public drawable
    {
        unsigned int VAO_id;
        virtual void bind_VAO() const override { glBindVertexArray(VAO_id);}
        virtual void set_samplers(const unsigned int &program_id) const override{} //a mesh has no texture IDs
        virtual void send_model_transform(const unsigned int &program_id) const override
        {
        }
        virtual void gl_draw(const unsigned int &program_id) const override
        {
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }
    public :
        vector<unsigned int> indices;

        mesh(){}

        virtual void send_data()
        {   //glVertexAttribPointer will only have effect on the data sent by the last call to glBufferData
            //VAOs only store the last call to glVertexAttribPointer
            glGenVertexArrays(1, &VAO_id);
            glBindVertexArray(VAO_id);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, normal_coords));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, tex_coords));
            glEnableVertexAttribArray(2);         

            unsigned int EBO_id;
            glGenBuffers(1, &EBO_id);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_id);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

            glBindVertexArray(0);
        }
    };
    //holds an array of drawable meshes. Initialize with read_obj()
    class object : public drawable
    {
        virtual void bind_VAO() const override {}
        virtual void send_model_transform(const unsigned int &program_id) const override
        {
            glUniformMatrix4fv(glGetUniformLocation(program_id, VS_TRNSFRM_MDL_NAME), 1, GL_FALSE, value_ptr(model_transform));
        }
        virtual void set_samplers(const unsigned int &program_id) const override
        {
            int nr_diffuse = 0, nr_spec = 0;
            int texture_unit_limit;
            glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texture_unit_limit);
            for (size_t i = 0; i < materials.size(); i++)
            {
                if (nr_diffuse + nr_spec > texture_unit_limit)
                {
                    break;
                }
                if (materials[i].diffuse_map.id > 0)
                {
                    string type = "diffuse_maps";
                    string index = "["+std::to_string(nr_diffuse)+"]";
                    glActiveTexture(GL_TEXTURE0 + nr_diffuse + nr_spec);
                    glBindTexture(GL_TEXTURE_2D, materials[i].diffuse_map.id);
                    glUniform1i(glGetUniformLocation(program_id, (type+index).c_str()), nr_diffuse + nr_spec);
                    glUniform1i(glGetUniformLocation(program_id, "nr_valid_diffuse_maps"), ++nr_diffuse);
                }
                if (materials[i].spec_map.id > 0)
                {
                    string type = "spec_maps";
                    string index = "["+std::to_string(nr_spec)+"]";
                    glActiveTexture(GL_TEXTURE0 + nr_diffuse + nr_spec);
                    glBindTexture(GL_TEXTURE_2D, materials[i].spec_map.id);
                    glUniform1i(glGetUniformLocation(program_id, (type+index).c_str()), nr_diffuse + nr_spec);
                    glUniform1i(glGetUniformLocation(program_id, "nr_valid_diffuse_maps"), ++nr_spec);
                }
            }
        }
        void send_vertex_data() const 
        {
            unsigned int VBO_id;
            glGenBuffers(1, &VBO_id);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_id);
            glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(vertex), &vertices[0], GL_STATIC_DRAW);
        }
        virtual void gl_draw(const unsigned int &program_id) const override
        {
            for (auto mesh : meshes)
            {
                mesh.draw(program_id);
            }
        }
    public:
        object(){model_transform = mat4(1.0);}
        vector<vertex> vertices;
        vector<mesh> meshes;
        vector<material> materials;
        mat4 model_transform;

        virtual void send_data() override
        {
            send_vertex_data();
            for (size_t i = 0; i < meshes.size(); i++)
            {
                meshes[i].send_data();       
            }
        }
    };
    
    //a drawable object with a manually generated float array of vertices. assumes coordinate order of pos, normals, texture 
    class array_drawable : public drawable
    {
        const float* const vertices;
        unsigned int VAO_id;
        const int array_size;
        bool texture, normals;

        virtual void bind_VAO() const override {glBindVertexArray(VAO_id);}
        virtual void gl_draw(const unsigned int &program_id) const override
        {
            const size_t nr_floats = size_t(array_size/sizeof(float));
            const unsigned int nr_floats_per_vertex = pos_dimension + (tex_dimension*texture) + (normals_dimension*normals);
            glDrawArrays(GL_TRIANGLES, 0, nr_floats/nr_floats_per_vertex);
        }
        virtual void set_samplers(const unsigned int &program_id) const 
        {
            if (textures.diffuse_map.id > 0)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textures.diffuse_map.id);
                glUniform1i(glGetUniformLocation(program_id, "diffuse_maps[0]"), 0);
            }
            if (textures.spec_map.id > 0)
            {
                const unsigned int texture_unit = 0 + (textures.diffuse_map.id > 0);
                glActiveTexture(texture_unit);
                glBindTexture(GL_TEXTURE_2D, textures.spec_map.id);
                glUniform1i(glGetUniformLocation(program_id, "spec_maps[0]"), texture_unit);
            }
            else if (textures.diffuse_map.id > 0)
            {
                glUniform1i(glGetUniformLocation(program_id, "spec_maps[0]"), 0);   //specular map points to diffuse map as fallback 
            }
        }
        virtual void send_model_transform(const unsigned int &program_id) const
        {
            glUniformMatrix4fv(glGetUniformLocation(program_id, VS_TRNSFRM_MDL_NAME), 1, GL_FALSE, value_ptr(model_transform));
        }
        public :
        array_drawable(const float* const vertices, const int array_byte_size, bool has_normal_coords = true, 
        bool has_texture_coords = true): vertices(vertices), array_size(array_byte_size), texture(has_texture_coords), 
        normals(has_normal_coords), model_transform(mat4(1.0)) {}

        mat4 model_transform;
        material textures;
        unsigned int pos_dimension = 3;
        unsigned int normals_dimension = 3;
        unsigned int tex_dimension = 2;
        virtual void send_data() override
        {
            unsigned int VBO;
            glGenBuffers(1, &VBO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, array_size, vertices, GL_STATIC_DRAW);

            glGenVertexArrays(1, &VAO_id);
            glBindVertexArray(VAO_id);
            
            const int nr_floats_per_vertex = (pos_dimension + (tex_dimension*texture) + (normals_dimension*normals));
            glVertexAttribPointer(0, pos_dimension, GL_FLOAT, GL_FALSE, nr_floats_per_vertex * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, normals_dimension, GL_FLOAT, GL_FALSE, nr_floats_per_vertex * sizeof(float), (void*)(pos_dimension*sizeof(float)));
            glEnableVertexAttribArray(1*normals);
            glVertexAttribPointer(2, tex_dimension, GL_FLOAT, GL_FALSE, nr_floats_per_vertex * sizeof(float), (void*)((pos_dimension+normals_dimension*normals)*sizeof(float)));
            glEnableVertexAttribArray(2*texture);
        
            glBindVertexArray(0);
        }
    };
}

tinyobj::ObjReader obj_parser;
bool read_obj(std::string path, object_3D::object &obj)
{
    if (!obj_parser.ParseFromFile(path, tinyobj::ObjReaderConfig()))
    {
        std::cerr << "loading object failed :\n";
        if (!obj_parser.Error().empty())
        {
            std::cerr << obj_parser.Error() << std::endl;
        }
        return false;
    }
    if (!obj_parser.Warning().empty())
        std::cout << obj_parser.Warning() << std::endl;
    
    tinyobj::attrib_t vertex_attribs = obj_parser.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = obj_parser.GetShapes();
    std::vector<tinyobj::material_t> materials = obj_parser.GetMaterials();

    //build object vertex array
    std::vector<object_3D::vertex> &vertices = obj.vertices;
    vertices = std::vector<object_3D::vertex>(vertex_attribs.vertices.size()/3);
    for(size_t i = 0; i < shapes.size(); i++)
    {   
        std::vector<unsigned int> recorded_indices;
        const std::vector<tinyobj::index_t> &raw_indices = shapes[i].mesh.indices;
        for (size_t j = 0; j < raw_indices.size(); j++)
        {
            const unsigned int vertex_index = raw_indices[j].vertex_index;
            bool new_vertex = true;
            for (size_t k = 0; k < recorded_indices.size(); k++) //is this vertex index recorded?
            {
                if(recorded_indices[k] == vertex_index) //vertex index is recorded
                {
                    new_vertex = false;
                    break;
                }
            }
            if (new_vertex)  //construct and record the vertex
            {
                recorded_indices.push_back(vertex_index);
                object_3D::vertex temp_vert;

                temp_vert.pos_coords.x = vertex_attribs.vertices[3*vertex_index + 0];
                temp_vert.pos_coords.y = vertex_attribs.vertices[3*vertex_index + 1];
                temp_vert.pos_coords.z = vertex_attribs.vertices[3*vertex_index + 2];

                const unsigned int normal_index = raw_indices[j].normal_index;
                if(normal_index >= 0)   //-1 signifies non-available data
                {
                    temp_vert.normal_coords.x = vertex_attribs.normals[3*normal_index + 0];
                    temp_vert.normal_coords.y = vertex_attribs.normals[3*normal_index + 1];
                    temp_vert.normal_coords.z = vertex_attribs.normals[3*normal_index + 2];
                }

                const unsigned int tex_index = raw_indices[j].texcoord_index;
                if (tex_index >= 0)
                {
                    temp_vert.tex_coords.x = vertex_attribs.texcoords[2*tex_index + 0];
                    temp_vert.tex_coords.y = vertex_attribs.texcoords[2*tex_index + 1];
                }
                vertices[vertex_index] = temp_vert;
            }
        }
    }
    
    //get mesh indices
    std::vector<object_3D::mesh> &meshes = obj.meshes;
    meshes = std::vector<object_3D::mesh>(shapes.size());
    for (size_t i = 0; i < shapes.size(); i++)
    {
        const std::vector<tinyobj::index_t> &indices = shapes[i].mesh.indices;
        object_3D::mesh temp_mesh;
        temp_mesh.indices = std::vector<unsigned int>(indices.size());
        for (size_t j = 0; j < indices.size(); j++)
        {
            //note that tinyobject.h takes care of offsetting the obj indices by 1 so we don't have to do it.
            temp_mesh.indices[j] = indices[j].vertex_index;
        }
        meshes[i] = temp_mesh;
    }

    //get textures 
    std::vector<object_3D::material> &obj_materials = obj.materials;
    obj_materials = std::vector<object_3D::material>(materials.size());
    for (size_t i = 0; i < materials.size(); i++)
    {
        //within this directory, we will search for the texture names
        const std::string directory = path.substr(0, path.find_last_of("/\\")+1);

        std::string file_name = materials[i].diffuse_texname;
        gen_texture((directory+file_name).c_str(), obj_materials[i].diffuse_map.id);
        
        file_name = materials[i].specular_texname;
        gen_texture((directory+file_name).c_str(), obj_materials[i].spec_map.id);
    }
    return true;
}

//reads texture from file and assigns it to the GL_TEXTURE_2D target with tex_id.
//be warned that this functions expects images with 3 or 4 color channels,
//otherwise, undefined behaviour will occur.
bool gen_texture(const char* file_path, unsigned int &tex_id)
{
    stbi_set_flip_vertically_on_load(false);
    int img_width, img_height, img_nrChannels;
    unsigned char* data = stbi_load(file_path, &img_width, &img_height, &img_nrChannels, 0);
    if (!data)
    {
        std::cout << "reading texture file failed : " << file_path << std::endl;
        return false;
    }

    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glTexImage2D(GL_TEXTURE_2D, 0, img_nrChannels == 3 ? GL_RGB : GL_RGBA, img_width, img_height, 0, img_nrChannels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    
    std::cout << "Loaded texture : " << file_path <<std::endl;
    return true;
}
bool gen_cubemap(const std::vector<std::string> &file_paths, unsigned int cubemap_tex_id)
{
    stbi_set_flip_vertically_on_load(true);
    glGenTextures(0, &cubemap_tex_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_tex_id);
    int img_width, img_height, img_nrChannels;
    unsigned char* data;
    for (size_t i = 0; i < file_paths.size(); i++)
    {
        data = stbi_load(file_paths[i].c_str(), &img_width, &img_height, &img_nrChannels,
        0);
        if(!data)
        {
            std::cerr << "reading texture file failed : " << file_paths[i] << std::endl;
            return false;
        }
        std::cout << "Loaded texture : " << file_paths[i] <<std::endl;
        GLenum format = img_nrChannels == 3 ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, format, img_width, img_height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);
    return true;
}
#endif