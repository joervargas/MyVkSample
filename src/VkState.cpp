#include "VkState.h"

#include "ProfilerWrapper.h"
#include <glslang/Include/glslang_c_interface.h>

// VulkanState vkState;
VulkanInstance vk;
VulkanRenderDevice vkDev;

size_t vertexBufferSize;
size_t indexBufferSize;

// std::unique_ptr<VulkanImGui> vk_imgui;
// std::unique_ptr<VulkanModelRenderer> vk_model_renderer;
// std::unique_ptr<VulkanCubeRenderer> vk_cube_renderer;
// std::unique_ptr<VulkanCanvas> vk_canvas;
// std::unique_ptr<VulkanCanvas> vk_canvas2d;
std::unique_ptr<VulkanMultiMeshRenderer> vk_multi_mesh_renderer;
std::unique_ptr<VulkanClear> vk_clear;
std::unique_ptr<VulkanFinish> vk_finish;

FramesPerSecondCounter fpsCounter(0.2f);
// LinearGraph fpsGraph;
// LinearGraph sineGraph(4096);

vec3 cameraPos(0.0f, 0.0f, 0.0f);
vec3 cameraAngles(-45.0f, 0.0f, 0.0f);

CameraPositioner_FirstPerson positioner_firstPerson(cameraPos, vec3(0.0f, 0.0f, -1.0f) , vec3(0.0f, 1.0f, 0.0f));
CameraPositioner_MoveTo positioner_moveTo(cameraPos, cameraAngles);
Camera camera = Camera(positioner_firstPerson);

const char* cameraType = "FirstPerson";
const char* comboBoxItems[] = { "FirstPerson", "MoveTo" };
const char* currentComboBoxItem = cameraType;

Resolution detectResolution(int width, int height)
{
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const int code = glfwGetError(nullptr);

    if(code != 0)
    {
        printf("Monitor: %p; error = %x / %d\n", monitor, code, code);
        exit(255);
    }

    const GLFWvidmode* info = glfwGetVideoMode(monitor);
    const uint32_t windowWidth = width > 0 ? width : (uint32_t)(info->width * width / -100);
    const uint32_t windowHeight = height > 0 ? height : (uint32_t)(info->height * height / -100);

    return Resolution{ .width = windowWidth, .height = windowHeight };
}

GLFWwindow* initWindow(int width, int height, Resolution *resolution)
{
    glslang_initialize_process();

    // volkInitialize();

    if(!glfwInit()) { exit(EXIT_FAILURE); }
    if(!glfwVulkanSupported()) { exit(EXIT_FAILURE); }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    if(resolution)
    {
        *resolution = detectResolution(width, height);
        width = resolution->width;
        height = resolution->height;
    }

    GLFWwindow* result = glfwCreateWindow(width, height, "VulkanApp", nullptr, nullptr);
    if(!result)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    return result;
}

bool initVulkan(GLFWwindow *window, uint32_t width, uint32_t height)
{
    // EASY_FUNCTION();

    createInstance(&vk.instance);
    if(!setupDebugCallbacks(vk.instance, &vk.messenger, &vk.reportCallback)) 
        { exit(EXIT_FAILURE); }

    if(glfwCreateWindowSurface(vk.instance, window, nullptr, &vk.surface)) 
        { exit(EXIT_FAILURE); }

    VkPhysicalDeviceFeatures deviceFeatures1{};
    deviceFeatures1.geometryShader = VK_TRUE;
    deviceFeatures1.drawIndirectFirstInstance = VK_TRUE; 
    VkPhysicalDeviceShaderDrawParameterFeatures drawParamFeatures{};
    drawParamFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES;
    drawParamFeatures.shaderDrawParameters = VK_TRUE;
    VkPhysicalDeviceFeatures2 deviceFeatures =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &drawParamFeatures,
        .features = deviceFeatures1
    };
    if(!initVulkanRenderDevice(vk, vkDev, width, height, isDeviceSuitable, deviceFeatures ) )
        { exit(EXIT_FAILURE); }

    // vk_imgui = std::make_unique<VulkanImGui>(vkDev);
    // vk_model_renderer = std::make_unique<VulkanModelRenderer>(vkDev, "assets/meshes/rubber_duck/scene.gltf", "assets/meshes/rubber_duck/textures/Duck_baseColor.png", (uint32_t)sizeof(glm::mat4));
    // vk_cube_renderer = std::make_unique<VulkanCubeRenderer>(vkDev, vk_model_renderer->getDepthTexture(), "assets/piazza_bologni_1k.hdr");
    // vk_canvas = std::make_unique<VulkanCanvas>(vkDev, vk_model_renderer->getDepthTexture());
    // vk_canvas2d = std::make_unique<VulkanCanvas>(vkDev, VulkanImage{ .image = VK_NULL_HANDLE, .imageView = VK_NULL_HANDLE });
    // vk_clear = std::make_unique<VulkanClear>(vkDev, vk_model_renderer->getDepthTexture());
    // vk_finish = std::make_unique<VulkanFinish>(vkDev, vk_model_renderer->getDepthTexture());
    vk_clear = std::make_unique<VulkanClear>(vkDev, VulkanImage());
    vk_finish = std::make_unique<VulkanFinish>(vkDev, VulkanImage());

    vk_multi_mesh_renderer = std::make_unique<VulkanMultiMeshRenderer>(vkDev, "./assets/meshes/Exterior/exterior.vk_sample", "./assets/meshes/Exterior/exterior.vk_sample.drawdata", "", "./shaders/VK05.vert", "./shaders/VK05.frag");

    // {
    //    vk_canvas->plane3d(vec3(0,+1.5,0), vec3(1,0,0), vec3(0,0,1), 40, 40, 10.0f, 10.0f, vec4(1,1,1,1), vec4(1,1,1,1));

    //    for(size_t i = 0; i < vkDev.swapchainImages.size(); i++)
    //    {
    //         vk_canvas->updateBuffer(vkDev, i);
    //    }
    // }

    return true;
}

void terminateVulkan()
{
    // vk_imgui = nullptr;
    // vk_canvas2d = nullptr;
    // vk_canvas = nullptr;
    // vk_cube_renderer = nullptr;
    // vk_model_renderer = nullptr;
    vk_multi_mesh_renderer = nullptr;
    vk_finish = nullptr;
    vk_clear = nullptr;

    destroyVulkanRenderDevice(vkDev);
    destroyVulkanInstance(vk);
}

void reinitCamera()
{
    if(!strcmp(cameraType, "FirstPerson"))
    {
        camera = Camera(positioner_firstPerson);
    }
    else
    {
        if(!strcmp(cameraType, "MoveTo"))
        {
            positioner_moveTo.setDesiredPosition(cameraPos);
            positioner_moveTo.setDesiredAngles(cameraAngles.x, cameraAngles.y, cameraAngles.z);
            camera = Camera(positioner_moveTo);
        }
    }
}

// void renderGUI(GLFWwindow* window, uint32_t imageIndex)
// {
//     // EASY_FUNCTION();

//     int width, height;
//     glfwGetFramebufferSize(window, &width, &height);

//     ImGuiIO& io = ImGui::GetIO();
//     io.DisplaySize = ImVec2((float)width, (float)height);
//     ImGui::NewFrame();

//     const ImGuiWindowFlags flags =
//         ImGuiWindowFlags_NoTitleBar |
//         ImGuiWindowFlags_NoResize |
//         ImGuiWindowFlags_NoMove |
//         ImGuiWindowFlags_NoScrollbar |
//         ImGuiWindowFlags_NoSavedSettings |
//         ImGuiWindowFlags_NoInputs |
//         ImGuiWindowFlags_NoBackground;
    
//     ImGui::SetNextWindowPos(ImVec2(0, 0));
//     ImGui::Begin("Statistics", nullptr, flags);
//     ImGui::Text("FPS: %.2f", fpsCounter.getFPS());
//     ImGui::End();

//     ImGui::Begin("Camera Control", nullptr);
//     {
//         if(ImGui::BeginCombo("##combo", currentComboBoxItem))
//         {
//             for(int n = 0; n < IM_ARRAYSIZE(comboBoxItems); n++)
//             {
//                 const bool isSelected = (currentComboBoxItem == comboBoxItems[n]);

//                 if(ImGui::Selectable(comboBoxItems[n], isSelected)) 
//                     { currentComboBoxItem = comboBoxItems[n]; }

//                 if(isSelected)
//                     { ImGui::SetItemDefaultFocus(); }
//             }

//             ImGui::EndCombo();
//         }

//         if(!strcmp(cameraType, "MoveTo"))
//         {
//             if(ImGui::SliderFloat3("Position", glm::value_ptr(cameraPos), -10.0f, 10.0f))
//                 { positioner_moveTo.setDesiredPosition(cameraPos); }
//             if(ImGui::SliderFloat3("Pitch/Pan/Roll", glm::value_ptr(cameraAngles), -90.f, 90.f))
//                 { positioner_moveTo.setDesiredAngles(cameraAngles); }
//         }

//         if(currentComboBoxItem && strcmp(currentComboBoxItem, cameraType))
//         {
//             printf("Selected new camera type: %s\n", currentComboBoxItem);
//             cameraType = currentComboBoxItem;
//             reinitCamera();
//         }
//     }
//     ImGui::End();
//     ImGui::Render();

//     vk_imgui->updateBuffers(vkDev, imageIndex, ImGui::GetDrawData());
// }

void update3D(GLFWwindow *window, uint32_t imageIndex)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    const float ratio = width / (float)height;

    // const mat4 m1 = glm::rotate(
    //     glm::translate(
    //         mat4(1.0f), 
    //         vec3(0.0f, 0.5, -1.5f)) * glm::rotate(mat4(1.f), glm::pi<float>(), vec3(1, 0, 0)
    //     ), 
    //     (float)glfwGetTime(), 
    //     vec3(0.0f, 1.0f, 0.0f)
    // );
    const mat4 m1 = glm::rotate(mat4(1.f), glm::pi<float>(), vec3(1, 0, 0));

    const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.f);

    const mat4 view = camera.getViewMatrix();
    const mat4 mtx = p * view * m1;

    {
        // EASY_BLOCK("UpdateUniformBuffers");
            vk_multi_mesh_renderer->updateUniformBuffer(vkDev, imageIndex, mtx);
            // vk_model_renderer->updateUniformBuffer(vkDev, imageIndex, glm::value_ptr(mtx), sizeof(mat4));
            // vk_canvas->updateUniformBuffer(vkDev, p * view, 0.0f, imageIndex);
            // vk_canvas2d->updateUniformBuffer(vkDev, glm::ortho(0, 1, 1, 0), 0.0f, imageIndex);
            // vk_cube_renderer->updateUniformBuffer(vkDev, imageIndex, mtx);
        // EASY_END_BLOCK;
    }
}

void update2D(uint32_t imageIndex)
{
    // vk_canvas2d->clear();
    // sineGraph.renderGraph(*vk_canvas2d.get(), vec4(0.0f, 1.0f, 0.0f, 1.0));
    // fpsGraph.renderGraph(*vk_canvas2d.get());
    // vk_canvas2d->updateBuffer(vkDev, imageIndex);
}

void updateBuffers(uint32_t imageIndex)
{
    // TODO: finish this!!!
}

void composeFrame(GLFWwindow* window, uint32_t imageIndex, const std::vector<VulkanRendererBase*>& renderers)
{
    update3D(window, imageIndex);
    // // renderGUI(window, imageIndex);
    // update2D(imageIndex);

    // // EASY_BLOCK("FillCommandBuffers");

        VkCommandBuffer commandBuffer = vkDev.commandBuffers[imageIndex];

        const VkCommandBufferBeginInfo bi =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            .pInheritanceInfo = nullptr
        };

        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &bi));

        for(auto& r: renderers)
        {
            r->fillCommandBuffer(commandBuffer, imageIndex);
        }

        VK_CHECK(vkEndCommandBuffer(commandBuffer));

    // EASY_END_BLOCK;
}

bool drawFrame(GLFWwindow *window, const std::vector<VulkanRendererBase *> &renderers)
{
    // EASY_FUNCTION();

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(vkDev.device, vkDev.swapchain, 0, vkDev.semaphore, VK_NULL_HANDLE, &imageIndex);
    VK_CHECK(vkResetCommandPool(vkDev.device, vkDev.commandPool, 0));

    if(result != VK_SUCCESS) { return false; }

    composeFrame(window, imageIndex, renderers);

    const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    const VkSubmitInfo si =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vkDev.semaphore,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkDev.commandBuffers[imageIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &vkDev.renderSemaphore
    };

    {
        // EASY_BLOCK("vkQueueSubmit", profiler::colors::Magenta);
            VK_CHECK(vkQueueSubmit(vkDev.graphicsQueue, 1, &si, nullptr));
        // EASY_END_BLOCK;
    }

    const VkPresentInfoKHR pi =
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vkDev.renderSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &vkDev.swapchain,
        .pImageIndices = &imageIndex
    };

    {
        // EASY_BLOCK("vkQueuePresentKHR", profiler::colors::Magenta);
            VK_CHECK(vkQueuePresentKHR(vkDev.graphicsQueue, &pi));
        // EASY_END_BLOCK;
    }

    {
        // EASY_BLOCK("vkDeviceWaitIdle", profiler::colors::Red);
            VK_CHECK(vkDeviceWaitIdle(vkDev.device));
        // EASY_END_BLOCK;
    }

    return true;
}

bool drawFrame(VulkanRenderDevice &vkDev, const std::function<void(uint32_t)> &updateBuffersFunc, std::function<void(VkCommandBuffer, uint32_t)> &composeFrameFunc)
{
    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(vkDev.device, vkDev.swapchain, 0, vkDev.semaphore, VK_NULL_HANDLE, &imageIndex);
    VK_CHECK(vkResetCommandPool(vkDev.device, vkDev.commandPool, 0));

    if (result != VK_SUCCESS) return false;

    updateBuffersFunc(imageIndex);

    VkCommandBuffer cmdBuffer = vkDev.commandBuffers[imageIndex];

    const VkCommandBufferBeginInfo begin_info =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = nullptr
    };

    VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &begin_info));
        composeFrameFunc(cmdBuffer, imageIndex);
    VK_CHECK(vkEndCommandBuffer(cmdBuffer));

    const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    const VkSubmitInfo submit_info =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vkDev.semaphore,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkDev.commandBuffers[imageIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &vkDev.renderSemaphore
    };

    VK_CHECK(vkQueueSubmit(vkDev.graphicsQueue, 1, &submit_info, nullptr));

    const VkPresentInfoKHR present_info = 
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vkDev.renderSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &vkDev.swapchain,
        .pImageIndices = &imageIndex
    };

    VK_CHECK(vkQueuePresentKHR(vkDev.graphicsQueue, &present_info));
    VK_CHECK(vkDeviceWaitIdle(vkDev.device));

    return true;
}
