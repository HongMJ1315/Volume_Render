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
#include "MarchingCubesTables.hpp"
#include <future>

int width = 800;
int height = 600;

// -------------------- Camera 全域變數 --------------------
glm::vec3 camera_pos = glm::vec3(420.0f, -100.0f, -180.0f);
glm::vec3 camera_front = glm::vec3(-0.40f, 0.60f, 0.60f);
glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;      // 初始指向 -Z（使用 -90 度）
float pitch = 0.0f;
float last_x = width / 2.0f;
float last_y = height / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
// --------------------------------------------------------

// 宣告一個 future 用來接收背景線程計算的結果（回傳兩個 surface）
#define MOVE_SPEED 10.f
#define ROTATE_SPEED 2.0f

#define MODEL_LEN 149.0f
#define MODEL_HEI 208.0f
#define MODEL_WID 110.0f



bool mouse_captured = false;

void process_input(GLFWwindow *window){
    float cameraSpeed = 50.f * deltaTime;
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera_pos += cameraSpeed * camera_front;
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera_pos -= cameraSpeed * camera_front;
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera_pos -= glm::normalize(glm::cross(camera_front, camera_up)) * cameraSpeed;
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera_pos += glm::normalize(glm::cross(camera_front, camera_up)) * cameraSpeed;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods){
    // 當按下 ESC 時，將滑鼠模式設為正常，游標將會脫離畫面
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        if(mouse_captured)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouse_captured = !mouse_captured;
        firstMouse = true;
    }
}
void mouse_callback(GLFWwindow *window, double xpos, double ypos){
    if(!mouse_captured)
        return;
    if(firstMouse){
        last_x = xpos;
        last_y = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - last_x;
    float yoffset = last_y - ypos; // 反轉 y 軸差值
    last_x = xpos;
    last_y = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camera_front = glm::normalize(front);
}

void reshape(GLFWwindow *window, int w, int h){
    width = w;
    height = h;
    glViewport(0, 0, width, height);
}

std::vector<unsigned char> data;
Volume volume;

// 用來記錄每個 surface 的 VAO 與頂點數量
GLuint VAO1, VAO2;
GLsizei vertCount1, vertCount2;


void init_data(){
    // 讀取資料並初始化兩個等值面
    read("Scalar/engine.raw", "Scalar/engine.inf", data);

    // volume: isovalue = 128, 紅色
    volume = Volume(data, MODEL_LEN, MODEL_HEI, MODEL_WID);
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


    // 設定滑鼠回呼，並隱藏游標（捕捉滑鼠）
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

    // ImGui 初始化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 400");

    // 讀取資料並生成兩個 iso_surface
    init_data();

    // 建立 shader
    int shaderProgram = set_shaders("shader/phong.vert", "shader/phong.frag");

    // 模型矩陣（如有需要可再調整位置）
    glm::mat4 model = glm::mat4(1.0f);

    // 光源 & 顏色
    glm::vec3 lightPos(300.0f, 300.0f, 600.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

    int m = 256, k = 256;
    // 渲染主迴圈
    while(!glfwWindowShouldClose(window)){
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        process_input(window);
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ImGui 畫面
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        input_window(camera_pos, camera_front, volume, m, k); // 顯示 RGB Transfer Function 編輯器
        histogram_window(volume); // 顯示 Histogram 編輯器
        line_editor_winodw(); // 顯示 RGB Transfer Function 編輯器

        glUseProgram(shaderProgram);

        // 計算 view 與 projection 矩陣
        glm::mat4 view = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float) width / height, 0.1f, 2000.0f);

        // 傳送矩陣給 shader
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
        GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
        GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
        GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera_pos));
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 清理
    glDeleteVertexArrays(1, &VAO1);
    glDeleteVertexArrays(1, &VAO2);
    // 如果你有另外存 VBO_pos, VBO_norm，要在這裡也一起刪除

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
