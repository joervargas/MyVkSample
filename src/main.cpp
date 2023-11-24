// #define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VkState.h"
#include <iostream>

// #include <volk/volk.h>
// #include <imgui/imgui.h>
#include <glslang/Include/glslang_c_interface.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
using glm::mat4;
using glm::vec3;
using glm::vec4;

GLFWwindow* window;
const uint32_t kScreenWidth = 1280;
const uint32_t kScreenHeight = 720;

struct MouseState
{
    vec2 pos = vec2(0.0f);
    bool pressedLeft = false;
} mouseState;

int main()
{
    // EASY_PROFILER_ENABLE;
    // EASY_MAIN_THREAD;

    // glslang_initialize_process();

    // volkInitialize();

    // if(!glfwInit()) { exit(EXIT_FAILURE); }
    // if(!glfwVulkanSupported()) { exit(EXIT_FAILURE); }

    // glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    // window = glfwCreateWindow(kScreenWidth, kScreenHeight, "VulkanApp", nullptr, nullptr);
    // if(!window)
    // {
    //     glfwTerminate();
    //     exit(EXIT_FAILURE);
    // }
    window = initWindow(kScreenWidth, kScreenHeight);
    
    // ImGui::CreateContext();
    // ImGuiIO& io = ImGui::GetIO();

    glfwSetCursorPosCallback(
        window,
        [](auto* window, double x, double y)
        {
            // ImGui::GetIO().MousePos = ImVec2((float)x, (float)y);
        }
    );

    glfwSetMouseButtonCallback(
        window,
        [](auto *window, int button, int action, int mods)
        {
            // auto& io = ImGui::GetIO();
            const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
            // io.MouseDown[idx] = action == GLFW_PRESS;
            if(button == GLFW_MOUSE_BUTTON_LEFT)
            {
                mouseState.pressedLeft = action == GLFW_PRESS;
            }
        }
    );

    glfwSetKeyCallback(
        window,
        [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            const bool pressed = action != GLFW_RELEASE;
            if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            if(key == GLFW_KEY_W)
            {
                positioner_firstPerson.movement.forward = pressed;
            }
            if(key == GLFW_KEY_S)
            {
                positioner_firstPerson.movement.backward = pressed;
            }
            if(key == GLFW_KEY_A)
            {
                positioner_firstPerson.movement.left = pressed;
            }
            if(key == GLFW_KEY_D)
            {
                positioner_firstPerson.movement.right = pressed;
            }
            if(key == GLFW_KEY_SPACE)
            {
                positioner_firstPerson.setUpVector(vec3(0.0f, 1.0f, 0.0f));
            }
        }
    );

    initVulkan(window, kScreenWidth, kScreenHeight);

    double timeStamp = glfwGetTime();
    float deltaSeconds = 0.0f;

    const std::vector<VulkanRendererBase*> renderers =
    {
        vk_clear.get(),
        // vk_cube_renderer.get(),
        // vk_multi_mesh_renderer.get(),
        // vk_model_renderer.get(),
        // vk_canvas.get(),
        vk_quad_renderer.get(),
        // vk_canvas2d.get(),
        // vk_imgui.get(),
        vk_finish.get()
    };

    while(!glfwWindowShouldClose(window))
    {
        // {
        //     // EASY_BLOCK("UpdateCameraPositioners");
        //         positioner_firstPerson.update(deltaSeconds, mouseState.pos, mouseState.pressedLeft);
        //         positioner_moveTo.update(deltaSeconds, mouseState.pos, mouseState.pressedLeft);
        //     // EASY_END_BLOCK;
        // }   

        const double newTimeStamp = glfwGetTime();
        deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
        timeStamp = newTimeStamp;

        const bool frameRendered = drawFrame(window, renderers);

        // if(fpsCounter.tick(deltaSeconds, frameRendered))
        // {
        //     // fpsGraph.addPoint(fpsCounter.getFPS());
        // }
        // sineGraph.addPoint((float)sin(glfwGetTime() * 10.0));
        
        // {
            // EASY_BLOCK("PollEvents");
                glfwPollEvents();
            // EASY_END_BLOCK;
        // }
    }

    // ImGui::DestroyContext();

    terminateVulkan();
    terminateWindow(window);
    // glfwTerminate();
    // glslang_finalize_process();

    // PROFILER_DUMP("profiling.prof");

    return 0;
}