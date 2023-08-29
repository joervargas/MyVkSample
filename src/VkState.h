#pragma once

#include "VkUtils.h"
#include <VkShader.h>
#include "Camera.h"
#include "UtilsFPS.h"

#include "VulkanClear.h"
#include "VulkanFinish.h"
#include "VulkanImGui.h"
#include "VulkanCanvas.h"
#include "VulkanModelRenderer.h"
#include "VulkanCubeRenderer.h"

#include "LinearGraph.h"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <memory>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::mat4;
using glm::vec3;
using glm::vec4;

struct UniformBuffer
{
    mat4 mvp;
};

extern std::unique_ptr<VulkanImGui> vk_imgui;
extern std::unique_ptr<VulkanModelRenderer> vk_model_renderer;
extern std::unique_ptr<VulkanCubeRenderer> vk_cube_renderer;
extern std::unique_ptr<VulkanCanvas> vk_canvas;
extern std::unique_ptr<VulkanCanvas> vk_canvas2d;
extern std::unique_ptr<VulkanClear> vk_clear;
extern std::unique_ptr<VulkanFinish> vk_finish;

static constexpr VkClearColorValue clearColorValue = { 1.0f, 1.0f, 1.0f, 1.0f };

extern FramesPerSecondCounter fpsCounter;
extern LinearGraph fpsGraph;
extern LinearGraph sineGraph;

extern vec3 cameraPos;
extern vec3 cameraAngles;

extern CameraPositioner_FirstPerson positioner_firstPerson;
extern CameraPositioner_MoveTo positioner_moveTo;
extern Camera camera;

extern const char* cameraType;
extern const char* comboBoxItems[];
extern const char* currentComboBoxItem;

bool initVulkan(GLFWwindow* window, uint32_t width, uint32_t height);

void terminateVulkan();

void reinitCamera();

void renderGUI(GLFWwindow* window, uint32_t imageIndex);

void update3D(GLFWwindow* window, uint32_t imageIndex);

void update2D(uint32_t imageIndex);

void composeFrame(GLFWwindow* window, uint32_t imageIndex, const std::vector<VulkanRendererBase*>& renderers);

bool drawFrame(GLFWwindow* window, const std::vector<VulkanRendererBase*>& renderers);