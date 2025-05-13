#include <iostream>
#include <algorithm>
#include <vector>
#include <utility>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iomanip>
#include <fstream>

#include "glsl.h"
#include "GLinclude.h"
#include "matrix.h"
#include "ui.h"


#include "reader.h"
#include "volume.h"
#include <future>

int width = 1600;
int height = 1200;

float yaw = glm::pi<float>() * 0.5f;
float pitch = 0.0f;
float radius = 20.0f;
double lastX = width * 0.5;
double lastY = height * 0.5;
bool   leftPressed = false;
glm::vec3 camera_pos, camera_front;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

#define MOVE_SPEED 10.f
#define ROTATE_SPEED 2.0f

#define MODEL_LEN 256.0f
#define MODEL_HEI 256.0f
#define MODEL_WID 256.0f

GLuint shader = 0, data_tex = 0, trans_tex = 0, gradient_tex = 0;

bool mouse_captured = false;

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods){

    ImGuiIO &io = ImGui::GetIO();
    if(io.WantCaptureMouse)
        return;

    if(button == GLFW_MOUSE_BUTTON_LEFT)
        leftPressed = (action == GLFW_PRESS);
}

void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos){
    if(leftPressed){
        float xoffset = float(xpos - lastX);
        float yoffset = float(ypos - lastY);
        const float sensitivity = 0.005f;
        yaw += xoffset * sensitivity;
        pitch += yoffset * sensitivity;
        pitch = glm::clamp(pitch, -glm::half_pi<float>() + 0.1f, glm::half_pi<float>() - 0.1f);
    }
    lastX = xpos;
    lastY = ypos;
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset){
    radius = glm::clamp(radius - float(yoffset), 1.0f, 100.0f);
}


void reshape(GLFWwindow *window, int w, int h){
    width = w;
    height = h;
    glViewport(0, 0, width, height);
}

void updateTransferFunc(
    const std::vector<float> &r,
    const std::vector<float> &g,
    const std::vector<float> &b,
    const std::vector<float> &a){
    size_t N = r.size();
    if(g.size() != N || b.size() != N || a.size() != N) return;

    std::vector<GLubyte> buf(N * 4);
    for(size_t i = 0; i < N; ++i){
        buf[4 * i + 0] = static_cast<GLubyte>(glm::clamp(r[i], 0.0f, 1.0f) * 255);
        buf[4 * i + 1] = static_cast<GLubyte>(glm::clamp(g[i], 0.0f, 1.0f) * 255);
        buf[4 * i + 2] = static_cast<GLubyte>(glm::clamp(b[i], 0.0f, 1.0f) * 255);
        buf[4 * i + 3] = static_cast<GLubyte>(glm::clamp(a[i], 0.0f, 1.0f) * 255);
    }

    glBindTexture(GL_TEXTURE_1D, trans_tex);
    glTexImage1D(GL_TEXTURE_1D, 0,
        GL_RGBA,
        (GLsizei) N,
        0, GL_RGBA,
        GL_UNSIGNED_BYTE,
        buf.data());
}

std::vector<unsigned char> data;
Volume volume;

GLuint VAO1, VAO2;
GLsizei vertCount1, vertCount2;


void init_data(){
    read("Scalar/skull.raw", "Scalar/skull.inf", data);

    volume = Volume(data, MODEL_LEN, MODEL_HEI, MODEL_WID);
    volume.compute_gradient(1.0f, 255.0f);
    volume.compute_histogram2d(256, 256);
}


int main(int argc, char **argv){
    glutInit(&argc, argv);
    if(!glfwInit()){
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(width, height, "Hw1", nullptr, nullptr);
    if(!window){
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, reshape);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    lastX = width * 0.5;
    lastY = height * 0.5;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    mouse_captured = true;

    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)){
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::cout << "GLFW version: " << glfwGetVersionString() << std::endl;
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    std::cout << "GLSL version: " << glslVersion << std::endl;
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL version supported: " << version << std::endl;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 400");

    init_data();
    shader = set_shaders("shader/volume.vs", "shader/volume.fs");


    // Transfer Function Texture 
    glGenTextures(1, &trans_tex);

    glBindTexture(GL_TEXTURE_1D, trans_tex);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);


    // Volume Data Texture 
    glGenTextures(1, &data_tex);

    glBindTexture(GL_TEXTURE_3D, data_tex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED,
        MODEL_LEN, MODEL_HEI, MODEL_WID,
        0, GL_RED, GL_UNSIGNED_BYTE,
        data.data());



    int m = 255, k = 255;
    int cell_size = 1;
    float gamma = 1;
    int threadhold = 1;

    // 建立覆蓋全螢幕的四邊形 (full-screen quad)
    float quadVertices[] = {
        // 位置 (x, y)
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f,  1.0f,
        -1.0f,  1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f
    };
    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);


    glm::mat4 model = glm::mat4(1.0f);
    glm::vec3 lightPos(300.0f, 300.0f, 600.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 ambientColor(0.6f, 0.6f, 0.6f);

    glUseProgram(shader);
    glUniform3fv(glGetUniformLocation(shader, "lightPos"), 1, &lightPos[0]);
    glUniform3fv(glGetUniformLocation(shader, "lightColor"), 1, &lightColor[0]);
    glUniform3fv(glGetUniformLocation(shader, "ambientColor"), 1, &ambientColor[0]);


    std::vector<float> tf_r, tf_g, tf_b, tf_alp;
    int is_phong = 0;
    while(!glfwWindowShouldClose(window)){
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        float camX = radius * cos(pitch) * cos(yaw);
        float camY = radius * sin(pitch);
        float camZ = radius * cos(pitch) * sin(yaw);
        camera_pos = glm::vec3(camX, camY, camZ);
        camera_front = glm::normalize(-camera_pos);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f),
            (float) width / height, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(camera_pos,
            glm::vec3(0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 invVP = glm::inverse(proj * view);

        updateTransferFunc(tf_r, tf_g, tf_b, tf_alp);

        glUseProgram(shader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, data_tex);
        glUniform1i(glGetUniformLocation(shader, "volumeTex"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, trans_tex);
        glUniform1i(glGetUniformLocation(shader, "tfTex"), 1);

        glUniform3fv(glGetUniformLocation(shader, "camPos"), 1, &camera_pos[0]);
        glUniformMatrix4fv(glGetUniformLocation(shader, "invViewProj"),
            1, GL_FALSE, &invVP[0][0]);
        glUniform1f(glGetUniformLocation(shader, "stepSize"), 1.0f / 500.0f);
        glUniform1i(glGetUniformLocation(shader, "isPhong"), is_phong);


        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        input_window(camera_pos, camera_front, volume, data, m, k, threadhold, gamma, cell_size, is_phong);
        // histogram_window(volume, cell_size);
        // line_editor_winodw(volume.get_distribute().size(), tf_r, tf_g, tf_b, tf_alp);
        // for(int i = 0; i < tf_r.size(); i++)
        //     std::cout << tf_r[i] << " " << tf_g[i] << " " << tf_b[i] << " " << tf_alp[i] << std::endl;
        // std::cout << tf_r.size() << " " << tf_g.size() << " " << tf_b.size() << " " << tf_alp.size() << std::endl;
        HistogramTFEditor(volume, tf_r, tf_g, tf_b, tf_alp, cell_size);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO1);
    glDeleteVertexArrays(1, &VAO2);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
