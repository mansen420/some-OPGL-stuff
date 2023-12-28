#include "glad/glad.h"  //include glad.h before glfw3.h
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "shader_utils.h"
#include "object_interface.h"

//global constants
constexpr float aspect_ratio = 16.0/9.0;
constexpr int WINDOW_H = 600;
constexpr int WINDOW_W = aspect_ratio * WINDOW_H;
//statics
static object_3D::object my_object; 
static object_3D::array_drawable* cube_ptr;
static object_3D::array_drawable* plane_ptr;
static object_3D::array_drawable* screen_tex_ptr;
static GLFWwindow* myWindow;

static unsigned int program_ids[10];
static unsigned int VAO_ids[10];
static unsigned int tex_ids[10];
static unsigned int framebuffer_ids[10];

static glm::vec3 cam_pos(0, 0, 1);
static glm::vec3 cam_front(0, 0, -1);
static glm::vec3 cam_up(0, 1, 0);

static float frame_delta = 0.0;
static glm::vec2 mouse_pos(float(WINDOW_W)/2.0, float(WINDOW_H)/2.0);
float yaw = -90.0f;
float pitch;
//functions 
void frame_buffer_callback(GLFWwindow* window, int width, int height);
void process_input(GLFWwindow* window);
inline bool initialize();
inline void render();
void sendVertexData();
int main()
{
    if (!initialize())
    {
        glfwTerminate();
        return -1;
    }
    {   //link shaders using a single vertex shader id. Program switching is expensive.
        //the same shader can be attached to multiple programs, and the inverse is true.
        unsigned int vShader, fShader, screen_v, screen_f;
        bool shaders_made = 
        compileShaderFromPath(VERTEX_SHADER, vShader, "src/vShader.vert") &&
        compileShaderFromPath(FRAGMENT_SHADER, fShader, "src/fShader.frag")&&
        compileShaderFromPath(VERTEX_SHADER, screen_v, "src/screen_vshader.vert") &&
        compileShaderFromPath(FRAGMENT_SHADER, screen_f, "src/screen_fshader.frag") &&
        linkShaders(program_ids[0], vShader, fShader) &&
        linkShaders(program_ids[1], screen_v, screen_f);
        if (!shaders_made)
        {
            glfwTerminate();
            return -1;
        }
        glDeleteShader(vShader);
        glDeleteShader(fShader);
        glDeleteShader(screen_v);
        glDeleteShader(screen_f);
    }
    //****************************************
    glGenFramebuffers(1, &framebuffer_ids[0]);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_ids[0]);

    glGenTextures(1, &tex_ids[2]);
    glBindTexture(GL_TEXTURE_2D, tex_ids[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_W, WINDOW_H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_ids[2], 0);
    
    unsigned int rndr_bfr_id;
    glGenRenderbuffers(1, &rndr_bfr_id);
    glBindRenderbuffer(GL_RENDERBUFFER, rndr_bfr_id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WINDOW_W, WINDOW_H);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rndr_bfr_id);   

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "framebuffer incomplete!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //****************************************
    float screen_vertices[] = {
    // positions
                   // texCoords
    -1.0f,  1.0f,  0.0f, 1.0f, 
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f, 

    -1.0f,  1.0f,  0.0f, 1.0f, 
     1.0f,  1.0f,  1.0f, 1.0f, 
     1.0f, -1.0f,  1.0f, 0.0f
    };
    object_3D::array_drawable screen_texture(screen_vertices, sizeof(screen_vertices), false, true);
    screen_texture.pos_dimension = 2;
    screen_texture.textures.diffuse_map.id = tex_ids[2];
    screen_tex_ptr = &screen_texture;
    screen_texture.send_data();
    //****************************************
    gen_texture("marble.jpg", tex_ids[0]);
    gen_texture("metal.png", tex_ids[1]);
    if (!read_obj("backpack_model/backpack.obj", my_object))
    {
        glfwTerminate();
        return -1;
    }

    float planeVertices[] = {
        // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
         5.0f, -0.5f, -5.0f,  2.0f, 2.0f								
    };
    float cubeVertices[] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
};
   
    object_3D::array_drawable plane(planeVertices, sizeof(planeVertices), false, true);
    plane.send_data();
    plane.textures.diffuse_map.id = tex_ids[1];
    object_3D::array_drawable cube(cubeVertices, sizeof(cubeVertices), true, true);
    cube.send_data();
    cube.textures.diffuse_map.id = tex_ids[0];
    cube_ptr = &cube;
    plane_ptr = &plane;
    my_object.send_data();
    
    float previous_frame_time = 0.0;
    float fps_sum = 0.0;
    int frame_count = 0;

    while (!glfwWindowShouldClose(myWindow))
    {
        render();
        frame_delta = glfwGetTime() - previous_frame_time;
        previous_frame_time = glfwGetTime();
        process_input(myWindow);
        glfwSwapBuffers(myWindow);
        glfwPollEvents();
        //display fps 
        frame_count++;
        fps_sum += frame_delta;
        float fps_avg = fps_sum/frame_count;
        std::cout << '\r' << 1.0f/frame_delta << "FPS" << std::flush;
    }
    glfwTerminate();
    return 0;
}
inline void send_transforms()
{
    using namespace glm;
    mat4 view(1.0f);
    view = lookAt(cam_pos, cam_pos + cam_front, cam_up);
    mat4 projection = perspective(radians(45.f), float(WINDOW_W)/WINDOW_H, 0.1f, 100.f);
    
    glUniformMatrix4fv(glGetUniformLocation(program_ids[0], "view_transform"), 1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(program_ids[0], "projection_transform"), 1, GL_FALSE, value_ptr(projection));
}
static glm::vec3 light_pos(0, 0, 1.2);
inline void send_light_info()
{
    glUniform3f(glGetUniformLocation(program_ids[0], "lights[0].color"), 1.0, 1.0, 1.0);
    glUniform4f(glGetUniformLocation(program_ids[0], "lights[0].pos"), light_pos.x, light_pos.y, light_pos.z, 1);
    glUniform3f(glGetUniformLocation(program_ids[0], "eye_pos"), cam_pos.x, cam_pos.y, cam_pos.z);
}
void render()
{
    //render to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    screen_tex_ptr->draw(program_ids[1]);

    //render off-screen 
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_ids[0]);
    glClearColor(0.1f, 0.1f, 0.1f, 1.f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program_ids[0]);
    light_pos = glm::vec3(3*sin(glfwGetTime()), 0, 3*cos(glfwGetTime()));
    send_light_info();
    send_transforms();
    //draw plane
    plane_ptr->draw(program_ids[0]);
    //draw cube
    cube_ptr->model_transform = glm::mat4(1.0);
    cube_ptr->draw(program_ids[0]);
    cube_ptr->model_transform = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 1.0, 0.0));
    cube_ptr->draw(program_ids[0]);
    //draw backpack 
    my_object.model_transform = glm::translate(glm::mat4(1.0), glm::vec3(0.25, 0, 0));  
    my_object.draw(program_ids[0]);
}
void frame_buffer_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void process_input(GLFWwindow* window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    const float speed = 2*frame_delta;
    const glm::vec3 cam_right = glm::normalize(glm::cross(cam_front, cam_up)) * speed;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cam_pos += speed * cam_front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cam_pos -= speed * cam_front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cam_pos -= cam_right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cam_pos += cam_right;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cam_up += cam_right;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cam_up -= cam_right;
}
void mouse_pos_callback(GLFWwindow* window, double x_pos, double y_pos)
{
    glm::vec2 offset = glm::vec2(x_pos, y_pos) - mouse_pos;
    mouse_pos = glm::vec2(x_pos, y_pos);
    offset *= 0.1f; //sensitivity 
    yaw += offset.x;
    pitch -= offset.y;  //mouse y range is inverted 

    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;
    
    glm::vec3 cam_angle;
    cam_angle.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    cam_angle.y = sin(glm::radians(pitch));
    cam_angle.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cam_front = glm::normalize(cam_angle);
}
inline bool initialize()
{
    //glfw boilerplate
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    myWindow = glfwCreateWindow(WINDOW_W, WINDOW_H, "OPGL", NULL, NULL);
    if (myWindow == NULL)
    {
        glfwTerminate();
        std::cout << "failed to instantiate window";
        return false;
    }
    glfwMakeContextCurrent(myWindow);
    //load gl functions. Don't call gl functions before this
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "failed to initialize GLAD" << std::endl;
        return false;
    }
    glViewport(0, 0, WINDOW_W, WINDOW_H);
    glfwSetFramebufferSizeCallback(myWindow, frame_buffer_callback);
    glfwSetInputMode(myWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(myWindow, mouse_pos_callback);
    return true;
}
