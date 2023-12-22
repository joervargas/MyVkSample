#include "VkUtils.h"
#include "VkShader.h"
#include "Bitmap.h"
#include "UtilsCubemap.h"
#include "vk_exts/vk_exts.h"

// #include <volk/volk.h>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <string.h>
#include <array>

#include <stb/stb_image.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

void VK_ASSERT(bool check, const char *fileName, int lineNumber)
{
    if(!check)
    {
        printf("\nVK_Check() failed at %s:%i\n\n", fileName, lineNumber); fflush(stdout);
        assert(false);
        exit(EXIT_FAILURE);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
        VkDebugUtilsMessageTypeFlagsEXT Type,
        const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
        void* UserData
    )
{
    printf("Validation layer: %s\n", CallbackData->pMessage);
    return false;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objectType,
        uint64_t object,
        size_t location,
        int32_t messageCode,
        const char* pLayerPrefix,
        const char* pMessage,
        void* UserData
    )
{
    if(flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT){ return VK_FALSE; }

    printf("Debug Callback (%s): %s\n", pLayerPrefix, pMessage);
    return VK_FALSE;
}


bool setupDebugCallbacks(
        VkInstance instance, 
        VkDebugUtilsMessengerEXT* messenger, 
        VkDebugReportCallbackEXT* reportCallback
    )
{
    {
        const VkDebugUtilsMessengerCreateInfoEXT ci = 
        {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = &VulkanDebugCallback,
            .pUserData = nullptr
        };
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        VK_CHECK(func(instance, &ci, nullptr, messenger));
    }
    {
        const VkDebugReportCallbackCreateInfoEXT ci =
        {
            .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                VK_DEBUG_REPORT_ERROR_BIT_EXT |
                VK_DEBUG_REPORT_DEBUG_BIT_EXT,
            .pfnCallback = &VulkanDebugReportCallback,
            .pUserData = nullptr
        };
        auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
        VK_CHECK(func(instance, &ci, nullptr, reportCallback));
    }
    return true;
}

void destroyDebugCallbacks(VkInstance &instance, VkDebugUtilsMessengerEXT &messenger, VkDebugReportCallbackEXT &reportCallback)
{
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if(func != nullptr)
        {
            func(instance, messenger, nullptr);
        }
        else
        {
            printf("vkDestroyDebugUtilsMessengerEXT funtion unable to load");
            exit(EXIT_FAILURE);
        }
    }
    {
        auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
        if(func != nullptr)
        {
            func(instance, reportCallback, nullptr);
        }
        else
        {
            printf("vkDestroyDebugReportCallbackEXT funtion unable to load");
            exit(EXIT_FAILURE);
        }
    }
}

bool setVkObjectName(VkInstance& instance, VulkanRenderDevice& vkDev, void* object, VkObjectType objType, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT nameInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = objType,
        .objectHandle = (uint64_t)object,
        .pObjectName = name
    };
    auto func = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
    return (func(vkDev.device, &nameInfo) == VK_SUCCESS);
}

void createInstance(VkInstance *instance)
{
    const std::vector<const char *> ValidationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char *> Instance_Exts =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        #if defined(WIN32)
            "VK_KHR_win32_surface",
        #endif
        #if defined(__APPLE__)
            "VK_MVK_macos_surface",
        #endif
        #if defined(__linux__)
            "VK_KHR_xcb_surface",
        #endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
    };

    const VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Vulkan",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_3
    };

    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(Instance_Exts.size()),
        .ppEnabledExtensionNames = Instance_Exts.data()
    };

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, instance));

    vk_ext::load_vk_instance_functions(*instance, Instance_Exts);
    // volkLoadInstance(*instance);
}

VkResult createDevice(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 deviceFeatures, uint32_t graphicsFamily, VkDevice *device)
{
    const float queuePriority = 1.0f;

    const std::vector<const char *> device_exts =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME,
        VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
        // VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME
    };

    const VkDeviceQueueCreateInfo queueCI = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = graphicsFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    const VkDeviceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &deviceFeatures,
        // .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCI,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(device_exts.size()),
        .ppEnabledExtensionNames = device_exts.data(),
        // .pEnabledFeatures = &deviceFeatures.features
    };
    return vkCreateDevice(physicalDevice, &ci, nullptr, device);
}

VkResult createDeviceWithCompute(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 deviceFeatures, uint32_t graphicsFamily, uint32_t computeFamily, VkDevice *device)
{
    const std::vector<const char*> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    if(graphicsFamily == computeFamily)
    {
        return createDevice(physicalDevice, deviceFeatures, graphicsFamily, device);
    }

    const float queuePriorities[2] = { 0.f, 0.f };
    // graphics queue
    const VkDeviceQueueCreateInfo q_ci_grfx =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = graphicsFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriorities[0]
    };
    // compute queue
    const VkDeviceQueueCreateInfo q_ci_cmp =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = computeFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriorities[1]
    };

    const VkDeviceQueueCreateInfo q_ci[] = { q_ci_grfx, q_ci_cmp };
    const VkDeviceCreateInfo ci =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &deviceFeatures,
        .flags = 0,
        .queueCreateInfoCount = 2,
        .pQueueCreateInfos = q_ci,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = uint32_t(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        // .pEnabledFeatures = &deviceFeatures.features
    };

    return vkCreateDevice(physicalDevice, &ci, nullptr, device);
}

bool isDeviceSuitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceShaderDrawParameterFeatures shaderDrawParamFeatures{};
    shaderDrawParamFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES;

    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &shaderDrawParamFeatures;
    vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

    const bool isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    const bool isIntegratedGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    const bool isGPU = isDiscreteGPU || isIntegratedGPU;

    return isGPU && deviceFeatures.features.geometryShader && shaderDrawParamFeatures.shaderDrawParameters;
}

VkResult findSuitablePhysicalDevice(VkInstance instance, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDevice *physicalDevice)
{
    uint32_t deviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, & deviceCount, nullptr));

    if(!deviceCount) return VK_ERROR_INITIALIZATION_FAILED;
    std::vector<VkPhysicalDevice> devices(deviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

    for(const auto& device: devices)
    {
        if(selector(device))
        {
            *physicalDevice = device;
            return VK_SUCCESS;
        }
    }
    return VK_ERROR_INITIALIZATION_FAILED;
}

uint32_t findQueueFamilies(VkPhysicalDevice device, VkQueueFlags desiredFlags)
{
    uint32_t familyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);

    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

    for(uint32_t i = 0; i != families.size(); i++)
    {
        if(families[i].queueCount && families[i].queueFlags & desiredFlags)
        {
            return i;
        }
    }
    return 0;
}

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if(formatCount)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if(presentModeCount)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for(const auto mode : availablePresentModes)
    {
        if(mode == VK_PRESENT_MODE_MAILBOX_KHR) { return mode; }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t chooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities)
{
    const uint32_t imageCount = capabilities.minImageCount + 1;

    const bool imageCountExceeded = capabilities.maxImageCount && imageCount > capabilities.maxImageCount;

    return imageCountExceeded ? capabilities.maxImageCount : imageCount;
}

VkResult createSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t graphicsFamily, uint32_t width, uint32_t height, VkSwapchainKHR *swapchain, bool supportScreenshots)
{
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice, surface);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);

    const VkSwapchainCreateInfoKHR ci =
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .flags = 0,
        .surface = surface,
        .minImageCount = chooseSwapImageCount(swapchainSupport.capabilities),
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = { .width = width, .height = height },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &graphicsFamily,
        .preTransform = swapchainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    return vkCreateSwapchainKHR(device, &ci, nullptr, swapchain);
}

size_t createSwapchainImages(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage> &swapchainImages, std::vector<VkImageView> &swapchainImageViews)
{
    uint32_t imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));

    swapchainImages.resize(imageCount);
    swapchainImageViews.resize(imageCount);

    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));

    for(unsigned i = 0; i < imageCount; i++)
    {
        if(!createImageView(device, swapchainImages[i], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &swapchainImageViews[i]))
        { exit(EXIT_FAILURE); }
    }

    return static_cast<size_t>(imageCount);
}

VkResult createSemaphore(VkDevice device, VkSemaphore *outSemaphore)
{
    const VkSemaphoreCreateInfo ci = 
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    return vkCreateSemaphore(device, &ci, nullptr, outSemaphore);
}

bool initVulkanRenderDevice(VulkanInstance &vk, VulkanRenderDevice &vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures2 deviceFeatures)
{
    vkDev.framebufferWidth = width;
    vkDev.framebufferHeight = height;

    VK_CHECK(findSuitablePhysicalDevice(vk.instance, selector, &vkDev.physicalDevice));

    vkDev.graphicsFamily = findQueueFamilies(vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT);

    // vkGetPhysicalDeviceFeatures2(vkDev.physicalDevice, &deviceFeatures);
    VK_CHECK(createDevice(vkDev.physicalDevice, deviceFeatures, vkDev.graphicsFamily, &vkDev.device));

    vkGetDeviceQueue(vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue);
    if(vkDev.graphicsQueue == nullptr) { exit(EXIT_FAILURE); }

    VkBool32 presentSupported = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported);
    if(!presentSupported) { exit(EXIT_FAILURE); }

    VK_CHECK(createSwapchain(
        vkDev.device, 
        vkDev.physicalDevice, 
        vk.surface, 
        vkDev.graphicsFamily, 
        width, height, 
        &vkDev.swapchain)
    );
    const size_t imageCount = createSwapchainImages(vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews);
    vkDev.commandBuffers.resize(imageCount);

    VK_CHECK(createSemaphore(vkDev.device, &vkDev.semaphore));
    VK_CHECK(createSemaphore(vkDev.device, &vkDev.renderSemaphore));

    const VkCommandPoolCreateInfo cpi =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = 0,
        .queueFamilyIndex = vkDev.graphicsFamily
    };
    VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi, nullptr, &vkDev.commandPool));

    const VkCommandBufferAllocateInfo ai =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = vkDev.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (uint32_t)(vkDev.swapchainImages.size())
    };
    VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai, vkDev.commandBuffers.data()));

    return true;
}

bool initVulkanRenderDeviceWithCompute(VulkanInstance &vk, VulkanRenderDevice &vkDev, uint32_t width, uint32_t height, VkPhysicalDeviceFeatures2 deviceFeatures)
{
    vkDev.framebufferWidth = width;
    vkDev.framebufferHeight = height;

    VK_CHECK(findSuitablePhysicalDevice(vk.instance, &isDeviceSuitable, &vkDev.physicalDevice));

    vkDev.graphicsFamily = findQueueFamilies(vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
    vkDev.computeFamily = findQueueFamilies(vkDev.physicalDevice, VK_QUEUE_COMPUTE_BIT);

    VK_CHECK(createDeviceWithCompute(vkDev.physicalDevice, deviceFeatures, vkDev.graphicsFamily, vkDev.computeFamily, &vkDev.device));
    
    vkDev.deviceQueueIndices.push_back(vkDev.graphicsFamily);
    if(vkDev.graphicsFamily != vkDev.computeFamily)
    {
        vkDev.deviceQueueIndices.push_back(vkDev.computeFamily);
    }

    vkGetDeviceQueue(vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue);
    if(!vkDev.graphicsQueue) exit(EXIT_FAILURE);

    vkGetDeviceQueue(vkDev.device, vkDev.computeFamily, 0, &vkDev.computeQueue);
    if(!vkDev.computeQueue) exit(EXIT_FAILURE);

    VkBool32 presentSupported = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported);
    if(!presentSupported) exit(EXIT_FAILURE);

    VK_CHECK(createSwapchain(vkDev.device, vkDev.physicalDevice, vk.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain));
    const size_t imageCount = createSwapchainImages(vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews);

    vkDev.commandBuffers.resize(imageCount);

    VK_CHECK(createSemaphore(vkDev.device, &vkDev.semaphore));
    VK_CHECK(createSemaphore(vkDev.device, &vkDev.renderSemaphore));

    const VkCommandPoolCreateInfo cmd_pool_ci =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = 0,
        .queueFamilyIndex = vkDev.graphicsFamily
    };
    VK_CHECK(vkCreateCommandPool(vkDev.device, &cmd_pool_ci, nullptr, &vkDev.commandPool));
    
    const VkCommandBufferAllocateInfo alloc_info =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = vkDev.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(vkDev.swapchainImages.size())
    };
    VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &alloc_info, &vkDev.commandBuffers[0]));
    
    // Compute
    const VkCommandPoolCreateInfo cmd_pool_cmp_ci =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = 0,
        .queueFamilyIndex = vkDev.computeFamily
    };
    VK_CHECK(vkCreateCommandPool(vkDev.device, &cmd_pool_cmp_ci, nullptr, &vkDev.computeCommandPool));

    const VkCommandBufferAllocateInfo alloc_cmp_ci =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = vkDev.computeCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &alloc_cmp_ci, &vkDev.computeCommandBuffer));

    vkDev.useCompute = true;
    return true;
}

void destroyVulkanRenderDevice(VulkanRenderDevice &vkDev)
{
    for(size_t i = 0; i < vkDev.swapchainImages.size(); i++)
    {
        vkDestroyImageView(vkDev.device, vkDev.swapchainImageViews[i], nullptr);
    }
    vkDestroySwapchainKHR(vkDev.device, vkDev.swapchain, nullptr);
    vkDestroyCommandPool(vkDev.device, vkDev.commandPool, nullptr);
    vkDestroySemaphore(vkDev.device, vkDev.semaphore, nullptr);
    vkDestroySemaphore(vkDev.device, vkDev.renderSemaphore, nullptr);
    vkDestroyDevice(vkDev.device, nullptr);
}

void destroyVulkanInstance(VulkanInstance &vk)
{
    vkDestroySurfaceKHR(vk.instance, vk.surface, nullptr);
    // vkDestroyDebugReportCallbackEXT(vk.instance, vk.reportCallback, nullptr);
    // vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.messenger, nullptr);
    destroyDebugCallbacks(vk.instance, vk.messenger, vk.reportCallback);
    vkDestroyInstance(vk.instance, nullptr);
}

uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

bool createBuffer(
        VkDevice device, 
        VkPhysicalDevice physicalDevice, 
        VkDeviceSize size, 
        VkBufferUsageFlags usage, 
        VkMemoryPropertyFlags properties, 
        VkBuffer &buffer, 
        VkDeviceMemory &bufferMemory
    )
{
    const VkBufferCreateInfo bufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };
    VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    const VkMemoryAllocateInfo allocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
    };
    VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));
    vkBindBufferMemory(device, buffer, bufferMemory, 0);

    return true;
}

bool createSharedBuffer(VulkanRenderDevice &vkDev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    const size_t familyCount = vkDev.deviceQueueIndices.size();
    if(familyCount < 2u)
    {
        return createBuffer(vkDev.device, vkDev.physicalDevice, size, usage, properties, buffer, bufferMemory);
    }

    const VkBufferCreateInfo buffer_info =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = (familyCount > 1u) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = static_cast<uint32_t>(familyCount),
        .pQueueFamilyIndices = (familyCount > 1u) ? vkDev.deviceQueueIndices.data() : nullptr
    };
    VK_CHECK(vkCreateBuffer(vkDev.device, &buffer_info, nullptr, &buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkDev.device, buffer, &memRequirements);

    const VkMemoryAllocateInfo alloc_info =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(vkDev.physicalDevice, memRequirements.memoryTypeBits, properties),
    };

    VK_CHECK(vkAllocateMemory(vkDev.device, &alloc_info, nullptr, &bufferMemory));
    vkBindBufferMemory(vkDev.device, buffer, bufferMemory, 0);

    return true;
}

void copyBuffer(
        VulkanRenderDevice& vkDev,
        VkBuffer srcBuffer, VkBuffer dstBuffer, 
        VkDeviceSize size
    )
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vkDev);

    const VkBufferCopy copyRegion =
    {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTImeCommands(vkDev, commandBuffer);
}

bool createUniformBuffer(VulkanRenderDevice &vkDev, VkBuffer &buffer, VkDeviceMemory &bufferMemory, VkDeviceSize bufferSize)
{
    return createBuffer(vkDev.device, vkDev.physicalDevice, bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer, bufferMemory);
}

void uploadBufferData(VulkanRenderDevice &vkDev, const VkDeviceMemory &bufferMemory, VkDeviceSize deviceOffset, const void *data, const size_t dataSize)
{
    
    void* mappedData = nullptr;
    vkMapMemory(vkDev.device, bufferMemory, deviceOffset, dataSize, 0, &mappedData);
        memcpy(mappedData, data, dataSize);
    vkUnmapMemory(vkDev.device, bufferMemory);
}

VkCommandBuffer beginSingleTimeCommands(VulkanRenderDevice &vkDev)
{
    VkCommandBuffer commandBuffer;
    
    const VkCommandBufferAllocateInfo allocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = vkDev.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers(vkDev.device, &allocInfo, &commandBuffer);

    const VkCommandBufferBeginInfo beginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void endSingleTImeCommands(VulkanRenderDevice &vkDev, VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    const VkSubmitInfo submitInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };
    vkQueueSubmit(vkDev.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkDev.graphicsQueue);

    vkFreeCommandBuffers(vkDev.device, vkDev.commandPool, 1, &commandBuffer);
}

bool createImage(
        VkDevice device, VkPhysicalDevice physicalDevice, 
        uint32_t width, uint32_t height, 
        VkFormat format, VkImageTiling tiling, 
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
        VkImage &image, VkDeviceMemory &imageMemory,
        VkImageCreateFlags flags, uint32_t mipLevels
    )
{
    const VkImageCreateInfo imageInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = VkExtent3D{ .width = width, .height = height, .depth = 1 },
        .mipLevels = mipLevels,
        .arrayLayers = (uint32_t)((flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? 6 : 1),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    VK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    const VkMemoryAllocateInfo ai = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
    };
    VK_CHECK(vkAllocateMemory(device, &ai, nullptr, &imageMemory));
    vkBindImageMemory(device, image, imageMemory, 0);

    return true;
}

bool createImageView(VkDevice device, VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView *imageView, VkImageViewType viewType, uint32_t layerCount, uint32_t miplevels)
{
    const VkImageViewCreateInfo viewInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = image,
        .viewType = viewType,
        .format = format,
        .subresourceRange =
            {
                .aspectMask = aspectFlags,
                .baseMipLevel = 0,
                .levelCount = miplevels,
                .baseArrayLayer = 0,
                .layerCount = layerCount
            }
    };

    return (vkCreateImageView(device, &viewInfo, nullptr, imageView) == VK_SUCCESS);
}

bool createTextureSampler(VkDevice device, VkSampler *sampler)
{
    const VkSamplerCreateInfo samplerInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .maxAnisotropy = 1,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f, 
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };
    VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, sampler));

    return true;
}

VkFormat findSupportedFormat(
        VkPhysicalDevice device, 
        const std::vector<VkFormat> &candidates, 
        VkImageTiling tiling, 
        VkFormatFeatureFlags features
    )
{
    for(VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);

        if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    printf("failed to find supported format!\n");
    exit(EXIT_FAILURE);
}

VkFormat findDepthFormat(VkPhysicalDevice device)
{
    return findSupportedFormat(
        device, 
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool createDepthResources(VulkanRenderDevice &vkDev, uint32_t width, uint32_t height, VulkanImage &depth)
{
    VkFormat depthFormat = findDepthFormat(vkDev.physicalDevice);

    if(!createImage(
        vkDev.device, 
        vkDev.physicalDevice, 
        width, height, depthFormat, 
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        depth.image, depth.imageMemory))
    {
        return false;
    }

    if(!createImageView(vkDev.device, depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &depth.imageView))
    {
        return false;
    }

    transitionImageLayout(vkDev, depth.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    
    return true;
}

void copyBufferToImage(VulkanRenderDevice &vkDev, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vkDev);

    const VkBufferImageCopy region =
    {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = VkImageSubresourceLayers{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = layerCount
            },
        .imageOffset = VkOffset3D{ .x = 0, .y = 0, .z = 0 },
        .imageExtent = VkExtent3D{ .width = width, .height = height, .depth = 1 }
    };
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTImeCommands(vkDev, commandBuffer);
}

void transitionImageLayoutCmd(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount, uint32_t mipLevels)
{
    VkImageMemoryBarrier barrier =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = VkImageSubresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = layerCount
        },
    };

    VkPipelineStageFlags srcStage, dstStage;

    if(newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ||
            (format == VK_FORMAT_D16_UNORM) ||
            (format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
            (format == VK_FORMAT_D32_SFLOAT) ||
            (format == VK_FORMAT_S8_UINT) ||
            (format == VK_FORMAT_D16_UNORM_S8_UINT) ||
            (format == VK_FORMAT_D24_UNORM_S8_UINT)
        )
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if(hasStencilComponent(format))
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} 
    else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) 
    {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
    {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	/* Convert back from read-only to updateable */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	/* Convert from updateable texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	/* Convert depth texture from undefined state to depth-stencil buffer */
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	/* Wait for render pass to complete */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
    {
		barrier.srcAccessMask = 0; // VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = 0;
/*
		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
///		destinationStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
*/
		srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	/* Convert back from read-only to color attachment */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	/* Convert from updateable texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	/* Convert back from read-only to depth attachment */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dstStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	/* Convert from updateable depth texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

    vkCmdPipelineBarrier(
		commandBuffer,
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void transitionImageLayout(VulkanRenderDevice &vkDev, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vkDev);
        transitionImageLayoutCmd(commandBuffer, image, format, oldLayout, newLayout, layerCount, mipLevels); 
    endSingleTImeCommands(vkDev, commandBuffer);
}

void destroyVulkanImage(VkDevice device, VulkanImage &img)
{
    vkDestroyImageView(device, img.imageView, nullptr);
    vkDestroyImage(device, img.image, nullptr);
    vkFreeMemory(device, img.imageMemory, nullptr);
}

void destroyVulkanTexture(VkDevice device, VulkanTexture &texture)
{
    destroyVulkanImage(device, texture.image);
    vkDestroySampler(device, texture.sampler, nullptr);
}

bool createTextureImage(VulkanRenderDevice &vkDev, const char *filename, VkImage &textureImage, VkDeviceMemory &textureImageMemory, uint32_t *outTexWidth, uint32_t *outTexHeight)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if(!pixels)
    {
        printf("Failed to load [%s] texture\n", filename); 
        fflush(stdout);
        return false;
    }

    VkDeviceSize imageSize = texWidth * texHeight * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    createBuffer(
        vkDev.device, vkDev.physicalDevice, 
        imageSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory
    );

    void* data;
    vkMapMemory(vkDev.device, stagingMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(vkDev.device, stagingMemory);

    createImage(
        vkDev.device, vkDev.physicalDevice, 
        texWidth, texHeight,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        textureImage, textureImageMemory
    );

    transitionImageLayout(
        vkDev, 
        textureImage,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    copyBufferToImage(
        vkDev, 
        stagingBuffer, textureImage, 
        static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)
    );

    transitionImageLayout(
        vkDev, 
        textureImage, 
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
    vkFreeMemory(vkDev.device, stagingMemory, nullptr);
    stbi_image_free(pixels);
    
    return true;
}

bool createTextureImageFromData(
    VulkanRenderDevice &vkDev, 
    VkImage &textureImage, VkDeviceMemory &textureImageMemory, void *imageData, 
    uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, 
    uint32_t layerCount, VkImageCreateFlags flags)
{
    createImage(
        vkDev.device, vkDev.physicalDevice, 
        texWidth, texHeight, texFormat, 
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        textureImage, textureImageMemory, flags);

    return updateTextureImage(vkDev, textureImage, textureImageMemory, texWidth, texHeight, texFormat, layerCount, imageData);
}

bool updateTextureImage(
    VulkanRenderDevice &vkDev, 
    VkImage &textureImage, VkDeviceMemory &textureImageMemory, 
    uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, 
    const void *imageData, VkImageLayout sourceImageLayout)
{
    uint32_t bytesPerPixel = bytesPerTexFormat(texFormat);

    VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
    VkDeviceSize imageSize = layerSize * layerCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        vkDev.device, vkDev.physicalDevice, 
        imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    uploadBufferData(vkDev, stagingBufferMemory, 0, imageData, imageSize);

    transitionImageLayout(vkDev, textureImage, texFormat, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount);
        copyBufferToImage(
            vkDev, 
            stagingBuffer, textureImage, 
            static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight),
            layerCount);
    transitionImageLayout(vkDev, textureImage, texFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layerCount);

    vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
    vkFreeMemory(vkDev.device, stagingBufferMemory, nullptr);
        
    return true;
}

static void float24to32(int w, int h, const float* img24, float* img32)
{
    const int numPixels = w * h;
    for(int i = 0; i != numPixels; i++)
    {
        *img32++ = *img24++;
        *img32++ = *img24++;
        *img32++ = *img24++;
        *img32++ = 1.0f;
    }
}

bool createCubeTextureImage(VulkanRenderDevice &vkDev, const char *filename, VkImage &textureImage, VkDeviceMemory &textureImageMemory, uint32_t *width, uint32_t *height)
{
    int w, h, comp;
    const float* img = stbi_loadf(filename, &w, &h, &comp, 3);
    std::vector<float> img32(w * h * 4);
    float24to32(w, h, img, img32.data());

    if(!img)
    {
        std::printf("Failed to load [%s] texture\n", filename); fflush(stdout);
        return false;
    }

    stbi_image_free((void*)img);

    Bitmap in(w, h, 4, eBitmapFormat_Float, img32.data());
    Bitmap out = convertEquirectangularMapToVerticalCross(in);

    Bitmap cube = convertVerticalCrossToCubeMapFaces(out);

    if(width && height)
    {
        *width = w;
        *height = h;
    }

    return createTextureImageFromData(
        vkDev, textureImage, textureImageMemory,
        cube.data_.data(), cube.w_, cube.h_, 
        VK_FORMAT_R32G32B32A32_SFLOAT, 6, 
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
    );
}

uint32_t bytesPerTexFormat(VkFormat fmt)
{
    switch (fmt)
    {
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8_UNORM:
        return 1;
    case VK_FORMAT_R16_SFLOAT:
        return 2;
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R16G16_SNORM:
        return 4;
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
        return 4;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return 4 * sizeof(uint16_t);
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 4 * sizeof(float);
    default:
        break;
    }
    return 0;
}

size_t allocateVertexBuffer(
    VulkanRenderDevice &vkDev,
    VkBuffer *storageBuffer, VkDeviceMemory *storageBufferMemory,
    size_t vertexDataSize, const void *vertexData, 
    size_t indexDataSize, const void *indexData)
{
    VkDeviceSize bufferSize = vertexDataSize + indexDataSize;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        vkDev.device, vkDev.physicalDevice, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory
    );

    void* data;
    vkMapMemory(vkDev.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertexData, vertexDataSize);
        memcpy((unsigned char*)data + vertexDataSize, indexData, indexDataSize);
    vkUnmapMemory(vkDev.device, stagingBufferMemory);

    createBuffer(
        vkDev.device, vkDev.physicalDevice, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *storageBuffer, *storageBufferMemory
    );

    copyBuffer(vkDev, stagingBuffer, *storageBuffer, bufferSize);

    vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
    vkFreeMemory(vkDev.device, stagingBufferMemory, nullptr);

    return bufferSize;
}

bool createTexturedVertexBuffer(
    VulkanRenderDevice &vkDev,
    const char *fileName,
    VkBuffer *storageBuffer, VkDeviceMemory *storageBufferMemory, 
    size_t *vertexBufferSize, size_t *indexBufferSize)
{
    const aiScene* scene = aiImportFile(fileName, aiProcess_Triangulate);
    if(!scene || !scene->HasMeshes())
    {
        printf("Unable to load %s\n", fileName);
        exit(255);
    }

    const aiMesh* mesh = scene->mMeshes[0];
    struct VertexData
    {
        vec3 pos;
        vec2 tc;
    };

    std::vector<VertexData> vertices;
    for(unsigned int i = 0; i != mesh->mNumVertices; i++)
    {
        const aiVector3D v = mesh->mVertices[i];
        const aiVector3D t = mesh->mTextureCoords[0][i];
        vertices.push_back({ .pos = vec3(v.x, v.z, v.y), .tc = vec2(t.x, 1.0f-t.y) });
    }

    std::vector<unsigned int> indices;
    for(unsigned int i = 0; i != mesh->mNumFaces; i++)
    {
        for(unsigned j = 0; j != 3; j++)
        {
            indices.push_back(mesh->mFaces[i].mIndices[j]);
        }
    }
    aiReleaseImport(scene);

    *vertexBufferSize = sizeof(VertexData)* vertices.size();
    *indexBufferSize = sizeof(unsigned int)* indices.size();

    allocateVertexBuffer(
        vkDev, 
        storageBuffer, storageBufferMemory, 
        *vertexBufferSize, vertices.data(), 
        *indexBufferSize, indices.data()
    );

    return true;
}

bool createDescriptorPool(VulkanRenderDevice &vkDev, uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, VkDescriptorPool *descriptorPool)
{
    const uint32_t imageCount = static_cast<uint32_t>(vkDev.swapchainImages.size());

    std::vector<VkDescriptorPoolSize> poolSizes;

    if(uniformBufferCount)
    {
        poolSizes.push_back(
            VkDescriptorPoolSize
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
                .descriptorCount = imageCount * uniformBufferCount 
            }
        );
    }

    if(storageBufferCount)
    {
        poolSizes.push_back(
            VkDescriptorPoolSize
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = imageCount * storageBufferCount
            }
        );
    }

    if(samplerCount)
    {
        poolSizes.push_back(
            VkDescriptorPoolSize
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = imageCount * samplerCount
            }
        );
    }

    const VkDescriptorPoolCreateInfo poolInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = static_cast<uint32_t>(imageCount),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data()
    };

    VK_CHECK(vkCreateDescriptorPool(vkDev.device, &poolInfo, nullptr, descriptorPool));

    return true;
}

bool createPipelineLayout(VkDevice device, VkDescriptorSetLayout dsLayout, VkPipelineLayout *pipelineLayout)
{
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &dsLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    return (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, pipelineLayout) == VK_SUCCESS);
}

bool createPipelineLayoutWithConstants(VkDevice device, VkDescriptorSetLayout dsLayout, VkPipelineLayout* pipelineLayout, uint32_t vtxConstSize, uint32_t fragConstSize)
{
    const VkPushConstantRange ranges[] =
    {
        {
            VK_SHADER_STAGE_VERTEX_BIT, // flag
            0,                          // offset
            vtxConstSize                // size
        },
        {
            VK_SHADER_STAGE_FRAGMENT_BIT,   // flag
            vtxConstSize,                   // offset
            fragConstSize                   // size
        }
    };

    uint32_t constSize = (vtxConstSize > 0) + (fragConstSize > 0);
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &dsLayout,
        .pushConstantRangeCount = constSize,
        .pPushConstantRanges = (constSize == 0) ? nullptr : (vtxConstSize > 0 ? ranges : &ranges[1])
    };

    return (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, pipelineLayout) == VK_SUCCESS);
}

bool createColorAndDepthRenderPass(VulkanRenderDevice &vkDev, bool useDepth, VkRenderPass *renderPass, const RenderPassCreateInfo &ci, VkFormat colorFormat)
{
    const bool offscreenInt = ci.flags & ERenderPassBit::ERPB_OFFSCREEN_INTERNAL;
    const bool first = ci.flags & ERenderPassBit::ERPB_FIRST;
    const bool last = ci.flags & ERenderPassBit::ERPB_LAST;

    VkAttachmentDescription colorAttachment =
    {
        .flags = 0,
        .format = colorFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = offscreenInt ? VK_ATTACHMENT_LOAD_OP_LOAD : (ci.clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD),
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = first ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInt ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
        .finalLayout = last ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL 
    };
    const VkAttachmentReference colorAttachmentRef =
    {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depthAttachment =
    {
        .flags = 0,
        .format = useDepth ? findDepthFormat(vkDev.physicalDevice) : VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = offscreenInt ? VK_ATTACHMENT_LOAD_OP_LOAD : (ci.clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD),
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = ci.clearDepth ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInt ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ),
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    const VkAttachmentReference depthAttachmentRef =
    {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    std::vector<VkSubpassDependency> dependencies =
    {
        VkSubpassDependency 
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0
        }
    };

    if(ci.flags & ERenderPassBit::ERPB_OFFSCREEN)
    {
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        dependencies.resize(2);
        dependencies[0] =
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
        };

        dependencies[1] =
        {
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
        };
    }

    const VkSubpassDescription subpass =
    {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = useDepth ? &depthAttachmentRef : nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    const VkRenderPassCreateInfo renderPassInfo =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(useDepth ? 2 : 1),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data()
    };

    return (vkCreateRenderPass(vkDev.device, &renderPassInfo, nullptr, renderPass) == VK_SUCCESS);
}

bool createColorAndDepthFramebuffer(VulkanRenderDevice &vkDev, uint32_t width, uint32_t height, VkRenderPass renderPass, VkImageView colorImageView, VkImageView depthImageView, VkFramebuffer *framebuffer)
{
    std::array<VkImageView, 2> attachments = { colorImageView, depthImageView };

    const VkFramebufferCreateInfo framebufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderPass = renderPass,
        .attachmentCount = (depthImageView == VK_NULL_HANDLE) ? 1u : 2u,
        .pAttachments = attachments.data(),
        .width = vkDev.framebufferWidth,
        .height = vkDev.framebufferHeight,
        .layers = 1
    };

    return (vkCreateFramebuffer(vkDev.device, &framebufferInfo, nullptr, framebuffer) == VK_SUCCESS);
}

bool createColorAndDepthFramebuffers(VulkanRenderDevice &vkDev, VkRenderPass renderPass, VkImageView depthImageView, std::vector<VkFramebuffer> &framebuffers)
{
    framebuffers.resize(vkDev.swapchainImageViews.size());
    for(size_t i = 0; i < vkDev.swapchainImages.size(); i++)
    {
        std::array<VkImageView, 2> attachments =
        {
            vkDev.swapchainImageViews[i],
            depthImageView
        };

        const VkFramebufferCreateInfo framebufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = renderPass,
            .attachmentCount = static_cast<uint32_t>((depthImageView == VK_NULL_HANDLE) ? 1 : 2),
            .pAttachments = attachments.data(),
            .width = vkDev.framebufferWidth,
            .height = vkDev.framebufferHeight,
            .layers = 1
        };

        VK_CHECK(vkCreateFramebuffer(vkDev.device, &framebufferInfo, nullptr, &framebuffers[i]));
    }
    return true;
}

// bool createGraphicsPipeline(
//         VulkanRenderDevice &vkDev,
//         uint32_t width, uint32_t height, 
//         VkRenderPass renderPass, 
//         VkPipelineLayout pipelineLayout, 
//         const std::vector<VkPipelineShaderStageCreateInfo> &shaderStages, 
//         VkPipeline *pipeline
//     )
// {
//     const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
//     {
//         .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
//     };

//     const VkPipelineInputAssemblyStateCreateInfo inputAssembly =
//     {
//         .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
//         .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
//         .primitiveRestartEnable = VK_FALSE
//     };

//     const VkViewport viewport = 
//     { 
//         .x = 0.0f, .y = 0.0f,
//         .width = static_cast<float>(width),
//         .height = static_cast<float>(height),
//         .minDepth = 0.0f,
//         .maxDepth = 1.0f
//     };

//     const VkRect2D scissor =
//     {
//         .offset = { 0, 0 },
//         .extent = { width, height }
//     };

//     const VkPipelineViewportStateCreateInfo viewportState =
//     {
//         .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
//         .viewportCount = 1,
//         .pViewports = &viewport,
//         .scissorCount = 1,
//         .pScissors = &scissor
//     };

//     const VkPipelineRasterizationStateCreateInfo rasterizer =
//     {
//         .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
//         .polygonMode = VK_POLYGON_MODE_FILL,
//         .cullMode = VK_CULL_MODE_NONE,
//         .frontFace = VK_FRONT_FACE_CLOCKWISE,
//         .lineWidth = 1.0f
//     };

//     const VkPipelineMultisampleStateCreateInfo multisampling =
//     {
//         .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
//         .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
//         .sampleShadingEnable = VK_FALSE,
//         .minSampleShading = 1.0f
//     };

//     const VkPipelineColorBlendAttachmentState colorBlendAttachment =
//     {
//         .blendEnable = VK_FALSE,
//         .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
//                           VK_COLOR_COMPONENT_G_BIT |
//                           VK_COLOR_COMPONENT_B_BIT |
//                           VK_COLOR_COMPONENT_A_BIT
//     };

//     const VkPipelineColorBlendStateCreateInfo colorBlending =
//     {
//         .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
//         .logicOpEnable = VK_FALSE,
//         .logicOp = VK_LOGIC_OP_COPY,
//         .attachmentCount = 1,
//         .pAttachments = &colorBlendAttachment,
//         .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
//     };

//     const VkPipelineDepthStencilStateCreateInfo depthStencil =
//     {
//         .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
//         .depthTestEnable = VK_TRUE,
//         .depthWriteEnable = VK_TRUE,
//         .depthCompareOp = VK_COMPARE_OP_LESS,
//         .depthBoundsTestEnable = VK_FALSE,
//         .minDepthBounds = 0.0f,
//         .maxDepthBounds = 1.0f
//     };

//     const VkGraphicsPipelineCreateInfo pipelineInfo =
//     {
//         .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
//         .stageCount = static_cast<uint32_t>(shaderStages.size()),
//         .pStages = shaderStages.data(),
//         .pVertexInputState = &vertexInputInfo,
//         .pInputAssemblyState = &inputAssembly,
//         .pTessellationState = nullptr,
//         .pViewportState = &viewportState,
//         .pRasterizationState = &rasterizer,
//         .pMultisampleState = &multisampling,
//         .pDepthStencilState = &depthStencil,
//         .pColorBlendState = &colorBlending,
//         .layout = pipelineLayout,
//         .renderPass = renderPass,
//         .subpass = 0,
//         .basePipelineHandle = VK_NULL_HANDLE,
//         .basePipelineIndex = -1
//     };

//     VK_CHECK(vkCreateGraphicsPipelines(vkDev.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline));
//     return true;
// }

bool createGraphicsPipeline(VulkanRenderDevice &vkDev, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, const std::vector<const char *> &shaderFiles, VkPipeline *pipeline, VkPrimitiveTopology topology, bool useDepth, bool useBlending, bool dynamicScissorState, int32_t customWidth, int32_t customHeight, uint32_t numPatchControlPoints)
{
    std::vector<VulkanShaderModule> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    shaderModules.resize(shaderFiles.size());
    shaderStages.resize(shaderFiles.size());

    for(size_t i = 0; i < shaderFiles.size(); i++)
    {
        const char* file = shaderFiles[i];
        VK_CHECK(createShaderModule(vkDev.device, &shaderModules[i], file));

        VkShaderStageFlagBits stage = getVkShaderStageFromFileName(file);

        shaderStages[i] = shaderStageInfo(stage, shaderModules[i], "main");
    }

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssembly =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = topology,
        .primitiveRestartEnable = VK_FALSE
    };

    const VkViewport viewport =
    {
        .x = 0.f,
        .y = 0.f,
        .width = static_cast<float>(customWidth > 0 ? customWidth : vkDev.framebufferWidth),
        .height = static_cast<float>(customHeight > 0 ? customHeight : vkDev.framebufferHeight),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    const VkRect2D scissor =
    {
        .offset = { 0, 0},
        .extent = 
        { 
            customWidth > 0 ? customWidth : vkDev.framebufferWidth, 
            customHeight > 0 ? customHeight : vkDev.framebufferHeight
        }
    };

    const VkPipelineViewportStateCreateInfo viewportState =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    const VkPipelineRasterizationStateCreateInfo rasterizer =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f
    };

    const VkPipelineMultisampleStateCreateInfo multisampling =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f
    };

    const VkPipelineColorBlendAttachmentState colorBlendAttachment =
    {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = useBlending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = 
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT
    };

    const VkPipelineColorBlendStateCreateInfo colorBlending =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = { 0.f, 0.f, 0.f, 0.f }
    };

    const VkPipelineDepthStencilStateCreateInfo depthStencil =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = useDepth? VK_TRUE : VK_FALSE,
        .depthWriteEnable = useDepth ? VK_TRUE : VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds = 0.f,
        .maxDepthBounds = 1.0f
    };

    std::array<VkDynamicState, 1> dynamicStates = { VK_DYNAMIC_STATE_SCISSOR };
    const VkPipelineDynamicStateCreateInfo dynamicState =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    const VkPipelineTessellationStateCreateInfo tessellationState =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .patchControlPoints = numPatchControlPoints
    };

    const VkGraphicsPipelineCreateInfo pipelineInfo =
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = (topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) ? &tessellationState : nullptr,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = useDepth? &depthStencil : nullptr,
        .pColorBlendState = &colorBlending,
        .pDynamicState = dynamicScissorState ? &dynamicState : nullptr,
        .layout = pipelineLayout,
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };
    VK_CHECK(vkCreateGraphicsPipelines(vkDev.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline));

    for(auto module : shaderModules)
    {
        vkDestroyShaderModule(vkDev.device, module.ShaderModule, nullptr);
    }

    return true;
}

VkResult createComputePipeline(VkDevice device, VkShaderModule computeShader, VkPipelineLayout pipelineLayout, VkPipeline *pipeline)
{
    VkPipelineShaderStageCreateInfo shader_stage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = computeShader,
        .pName = "main",
        .pSpecializationInfo = nullptr
    };
    VkComputePipelineCreateInfo comp_pipeline_info =
    {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = shader_stage,
        .layout = pipelineLayout,
        .basePipelineHandle = 0,
        .basePipelineIndex = 0
    };

    return vkCreateComputePipelines(device, 0, 1, &comp_pipeline_info, nullptr, pipeline);
}

bool executeComputeShader(VulkanRenderDevice &vkDev, VkPipeline pipeline, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet, uint32_t xsize, uint32_t ysize, uint32_t zsize)
{
    VkCommandBuffer cmdBuffer = vkDev.computeCommandBuffer;

    VkCommandBufferBeginInfo cmd_begin_info =
    {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        0,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        0
    };
    VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmd_begin_info));

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, 0);
    vkCmdDispatch(cmdBuffer, xsize, ysize, zsize);

    VkMemoryBarrier readoutBarrier =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
    };

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &readoutBarrier, 0, nullptr, 0, nullptr);

    VK_CHECK(vkEndCommandBuffer(cmdBuffer));

    VkSubmitInfo submit_info =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };
    VK_CHECK(vkQueueSubmit(vkDev.computeQueue, 1, &submit_info, VK_NULL_HANDLE));

    VK_CHECK(vkQueueWaitIdle(vkDev.computeQueue));
    return true;
}
