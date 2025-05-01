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

int width = 800;
int height = 600;

// -------------------- Camera 全域變數 --------------------
// -----------------------------------------------------------------
// 摄像机轨道控制（Orbit）用的全局变量
// -----------------------------------------------------------------
float yaw = glm::pi<float>() * 0.5f;  // 初始绕 Y 轴 90°
float pitch = 0.0f;
float radius = 20.0f;
double lastX = width * 0.5;
double lastY = height * 0.5;
bool   leftPressed = false;
glm::vec3 camera_pos, camera_front;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
// --------------------------------------------------------

// 宣告一個 future 用來接收背景線程計算的結果（回傳兩個 surface）
#define MOVE_SPEED 10.f
#define ROTATE_SPEED 2.0f

#define MODEL_LEN 256.0f
#define MODEL_HEI 256.0f
#define MODEL_WID 256.0f

GLuint shader = 0, data_tex = 0, trans_tex = 0;

bool mouse_captured = false;

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods){

    ImGuiIO &io = ImGui::GetIO();
    if(io.WantCaptureMouse) 
        return;

    if(button == GLFW_MOUSE_BUTTON_LEFT)
        leftPressed = (action == GLFW_PRESS);
}

// 鼠标移动时更新 yaw/pitch
void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos){
    if(leftPressed){
        float xoffset = float(xpos - lastX);
        float yoffset = float(ypos - lastY);
        const float sensitivity = 0.005f;
        yaw += xoffset * sensitivity;
        pitch += yoffset * sensitivity;
        // 限制俯仰角不要翻顶
        pitch = glm::clamp(pitch, -glm::half_pi<float>() + 0.1f, glm::half_pi<float>() - 0.1f);
    }
    lastX = xpos;
    lastY = ypos;
}

// 滚轮缩放
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset){
    radius = glm::clamp(radius - float(yoffset), 1.0f, 100.0f);
}


void reshape(GLFWwindow *window, int w, int h){
    width = w;
    height = h;
    glViewport(0, 0, width, height);
}

// 2) 把 updateTransferFunc 改成動態長度版本：
void updateTransferFunc(
    const std::vector<float> &r,
    const std::vector<float> &g,
    const std::vector<float> &b,
    const std::vector<float> &a){
    // 四個 vector 長度要一致：
    size_t N = r.size();
    if(g.size() != N || b.size() != N || a.size() != N) return;

    // 組成 N×4 的 GLubyte buffer：
    std::vector<GLubyte> buf(N * 4);
    for(size_t i = 0; i < N; ++i){
        buf[4 * i + 0] = static_cast<GLubyte>(glm::clamp(r[i], 0.0f, 1.0f) * 255);
        buf[4 * i + 1] = static_cast<GLubyte>(glm::clamp(g[i], 0.0f, 1.0f) * 255);
        buf[4 * i + 2] = static_cast<GLubyte>(glm::clamp(b[i], 0.0f, 1.0f) * 255);
        buf[4 * i + 3] = static_cast<GLubyte>(glm::clamp(a[i], 0.0f, 1.0f) * 255);
        // std::cout << r[i] << " " << g[i] << " " << b[i] << " " << a[i] << std::endl;
        // std::cout << buf[4 * i + 0] << " " << buf[4 * i + 1] << " " << buf[4 * i + 2] << " " << buf[4 * i + 3] << std::endl;
    }

    // 直接用 glTexImage1D 重新上傳動態長度：
    // std::cout << "update trans tex" << std::endl;
    glBindTexture(GL_TEXTURE_1D, trans_tex);
    glTexImage1D(GL_TEXTURE_1D, 0,
        GL_RGBA,           // internal format
        (GLsizei) N,        // 動態長度
        0, GL_RGBA,
        GL_UNSIGNED_BYTE,
        buf.data());
}

std::vector<unsigned char> data;
Volume volume;

GLuint VAO1, VAO2;
GLsizei vertCount1, vertCount2;


void init_data(){
    read("Scalar/testing_engine.raw", "Scalar/testing_engine.inf", data);

    volume = Volume(data, MODEL_LEN, MODEL_HEI, MODEL_WID, 0.5, 0);
    volume.compute_gradient(1.0f, 255.0f);
    volume.compute_histogram2d(256, 256);
}


int main(int argc, char **argv){
    // 初始化
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
    // 初始把 lastX/lastY 设成窗口中心
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
    // 1) 在載入 Volume data 之後（或在 loadResources() 裡），補上 trans_tex 的初始化：
    glGenTextures(1, &trans_tex);
    glBindTexture(GL_TEXTURE_1D, trans_tex);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // 先放一個長度 1 的空貼圖，之後每 frame 以動態長度重新上傳：
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // 在 init_data() 之后、glTexImage3D 之前，添加：

    glGenTextures(1, &data_tex);
    glBindTexture(GL_TEXTURE_3D, data_tex);
    // 完善一下参数：
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // 然后才上传真正的数据
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED,
        MODEL_LEN, MODEL_HEI, MODEL_WID,
        0, GL_RED, GL_UNSIGNED_BYTE,
        data.data());



    glm::mat4 model = glm::mat4(1.0f);

    glm::vec3 lightPos(300.0f, 300.0f, 600.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

    int m = 255, k = 255;
    int cell_size = 1;
    float gamma = 0.5;
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


    std::vector<float> tf_r, tf_g, tf_b, tf_alp;
    while(!glfwWindowShouldClose(window)){
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // process_input(window);
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ImGui 
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 先算 mat
        float camX = radius * cos(pitch) * cos(yaw);
        float camY = radius * sin(pitch);
        float camZ = radius * cos(pitch) * sin(yaw);
        camera_pos = glm::vec3(camX, camY, camZ);
        camera_front = glm::normalize(-camera_pos);   // 看向原点
        glm::mat4 proj = glm::perspective(glm::radians(45.0f),
            (float) width / height, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(camera_pos,
            glm::vec3(0.0f),   // 环绕到原点
            glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 invVP = glm::inverse(proj * view);

        // 更新 TF
        updateTransferFunc(tf_r, tf_g, tf_b, tf_alp);

        // 啟用 ray-casting shader
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

        // 畫 full-screen quad
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        input_window(camera_pos, camera_front, volume, data, m, k, threadhold, gamma, cell_size);
        histogram_window(volume, cell_size);
        line_editor_winodw(volume.get_distribute().size(), tf_r, tf_g, tf_b, tf_alp);
        // for(int i = 0; i < tf_r.size(); i++)
        //     std::cout << tf_r[i] << " " << tf_g[i] << " " << tf_b[i] << " " << tf_alp[i] << std::endl;
        // std::cout << tf_r.size() << " " << tf_g.size() << " " << tf_b.size() << " " << tf_alp.size() << std::endl;


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
