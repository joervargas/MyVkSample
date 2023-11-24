#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <functional>

#define VK_CHECK(value) VK_ASSERT(value == VK_SUCCESS, __FILE__, __LINE__);
#define VK_CHECK_RET(value) VK_ASSERT(value == VK_SUCCESS, __FILE__, __LINE__); return value;
#define BL_CHECK(value)

void VK_ASSERT(bool check, const char *fileName, int lineNumber);

// const std::vector<const char *> ValidationLayers =
//     {
//         "VK_LAYER_KHRONOS_validation"
//     };

// const std::vector<const char *> Instance_Exts =
//     {
//         VK_KHR_SURFACE_EXTENSION_NAME,
// #if defined(WIN32)
//         "VK_KHR_win32_surface",
// #endif
// #if defined(__APPLE__)
//         "VK_MVK_macos_surface",
// #endif
// #if defined(__linux__)
//         "VK_KHR_xcb_surface",
// #endif
//         VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
//         VK_EXT_DEBUG_REPORT_EXTENSION_NAME
//     };

// const std::vector<const char *> device_exts =
//     {
//         VK_KHR_SWAPCHAIN_EXTENSION_NAME
//         // VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
// };

struct VulkanInstance final
{
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDebugUtilsMessengerEXT messenger;
    VkDebugReportCallbackEXT reportCallback;
};

struct VulkanRenderDevice final
{
    uint32_t framebufferWidth;
    uint32_t framebufferHeight;

    VkDevice device;
    VkQueue graphicsQueue;
    VkPhysicalDevice physicalDevice;

    uint32_t graphicsFamily;

    VkSwapchainKHR swapchain;
    VkSemaphore semaphore;
    VkSemaphore renderSemaphore;

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    // Compute members
    bool useCompute = false;

    uint32_t computeFamily;
    VkQueue computeQueue;

    std::vector<uint32_t> deviceQueueIndices;
    std::vector<VkQueue> deviceQueues;

    VkCommandBuffer computeCommandBuffer;
    VkCommandPool computeCommandPool;
};

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct VulkanContextFeatures
{
    bool supportScreenshots_ = false;
    bool geometryShader_ = true;
    bool tessellationShader_ = false;
    bool vertexPipelineStoresAndAtomics_ = false;
    bool fragmentStoresAndAtomics_ = false;
};

struct VulkanContextCreator
{
    VulkanContextCreator();
    VulkanContextCreator(VulkanInstance &vk, VulkanRenderDevice &vkDev, void *window, int screenWidth, int screenHeight, const VulkanContextFeatures &ctxFeatures = VulkanContextFeatures());
    ~VulkanContextCreator();

    VulkanInstance &instance;
    VulkanRenderDevice &vkDev;
};

bool setupDebugCallbacks(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, VkDebugReportCallbackEXT* reportCallback);

void DestroyDebugCallbacks(VkInstance& instance, VkDebugUtilsMessengerEXT& messenger, VkDebugReportCallbackEXT& reportCallback);

void createInstance(VkInstance *instance);

VkResult createDevice(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 deviceFeatures, uint32_t graphicsFamily, VkDevice *device);

VkResult createDeviceWithCompute(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 deviceFeatrues, uint32_t graphicsFamily, uint32_t computeFamily, VkDevice* device);

bool isDeviceSuitable(VkPhysicalDevice device);

VkResult findSuitablePhysicalDevice(VkInstance instance, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDevice *physicalDevice);

uint32_t findQueueFamilies(VkPhysicalDevice device, VkQueueFlags desiredFlags);

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

uint32_t chooseSwapImageCount(const VkSurfaceCapabilitiesKHR &capabilities);

VkResult createSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t graphicsFamily, uint32_t width, uint32_t height, VkSwapchainKHR *swapchain, bool supportScreenshots = false);

size_t createSwapchainImages(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage> &swapchainImages, std::vector<VkImageView> &swapchainImageView);

VkResult createSemaphore(VkDevice device, VkSemaphore *outSemaphore);

bool initVulkanRenderDevice(
    VulkanInstance &vk, VulkanRenderDevice &vkDev,
    uint32_t width, uint32_t height,
    std::function<bool(VkPhysicalDevice)> selector,
    VkPhysicalDeviceFeatures2 deviceFeatures);

bool initVulkanRenderDeviceWithCompute(
    VulkanInstance& vk, VulkanRenderDevice& vkDev,
    uint32_t width, uint32_t height,
    VkPhysicalDeviceFeatures2 deviceFeatures);

void destroyVulkanRenderDevice(VulkanRenderDevice &vkDev);

void destroyVulkanInstance(VulkanInstance &vk);

uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

struct VulkanBuffer
{
    VkBuffer buffer;
    VkDeviceSize size;
    VkDeviceMemory memory;

    // Permanent mapping to CPU address space
    void *ptr;
};

bool createBuffer(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer &buffer,
    VkDeviceMemory &bufferMemory);

bool createSharedBuffer(
    VulkanRenderDevice& vkDev,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory
);

void copyBuffer(
    VulkanRenderDevice& vkDev,
    VkBuffer srcBuffer, VkBuffer dstBuffer,
    VkDeviceSize size);

bool createUniformBuffer(VulkanRenderDevice& vkDev, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize bufferSize);

void uploadBufferData(VulkanRenderDevice& vkDev, const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, const void* data, const size_t dataSize);

VkCommandBuffer beginSingleTimeCommands(VulkanRenderDevice& vkDev);

void endSingleTImeCommands(VulkanRenderDevice& vkDev, VkCommandBuffer commandBuffer);

struct VulkanImage final
{
    VkImage image = nullptr;
    VkDeviceMemory imageMemory = nullptr;
    VkImageView imageView = nullptr;
};

struct VulkanTexture final
{
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    VkFormat format;

    VulkanImage image;
    VkSampler sampler;

    // Offscreen buffers require VK_IMAGE_LAYOUT_GENERAL && static features have VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    VkImageLayout desiredLayout;
};

bool createImage(
    VkDevice device, VkPhysicalDevice physicalDevice, 
    uint32_t width, uint32_t height, 
    VkFormat format, VkImageTiling tiling, 
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
    VkImage& image, VkDeviceMemory& imageMemory,
    VkImageCreateFlags flags = 0, uint32_t mipLevels = 1);

bool createImageView(
    VkDevice device, 
    VkImage image, 
    VkFormat format, 
    VkImageAspectFlags aspectFlags, 
    VkImageView *imageView, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, 
    uint32_t layerCount = 1, uint32_t miplevels = 1);

bool createTextureSampler(VkDevice device, VkSampler* sampler);

VkFormat findSupportedFormat(
    VkPhysicalDevice device, 
    const std::vector<VkFormat>& candidates, 
    VkImageTiling tiling, 
    VkFormatFeatureFlags features);

VkFormat findDepthFormat(VkPhysicalDevice device);

bool hasStencilComponent(VkFormat format);

bool createDepthResources(VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VulkanImage& depth);

void copyBufferToImage(VulkanRenderDevice& vkDev, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount = 1);

void transitionImageLayout(
    VulkanRenderDevice& vkDev, 
    VkImage image, VkFormat format, 
    VkImageLayout oldLayout, VkImageLayout newLayout, 
    uint32_t layerCount = 1, uint32_t mipLevels = 1);

void destroyVulkanImage(VkDevice device, VulkanImage& img);
void destroyVulkanTexture(VkDevice device, VulkanTexture& texture);

bool createTextureImage(
    VulkanRenderDevice& vkDev, 
    const char* filename, 
    VkImage& textureImage, VkDeviceMemory& textureImageMemory, 
    uint32_t* outTexWidth = nullptr, uint32_t* outTexHeight = nullptr);

bool createTextureImageFromData(
    VulkanRenderDevice& vkDev,
    VkImage& textureImage, VkDeviceMemory& textureImageMemory, void* imageData,
    uint32_t texWidth, uint32_t texHeight, VkFormat texFormat,
    uint32_t layerCount = 1, VkImageCreateFlags flags = 0);

bool updateTextureImage(
    VulkanRenderDevice& vkDev, 
    VkImage& textureImage, VkDeviceMemory& textureImageMemory, 
    uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, 
    const void* imageData, VkImageLayout sourceImageLayout = VK_IMAGE_LAYOUT_UNDEFINED);

bool createCubeTextureImage(
    VulkanRenderDevice& vkDev, 
    const char* filename, 
    VkImage& textureImage, VkDeviceMemory& textureImageMemory, 
    uint32_t* width = nullptr, uint32_t* height = nullptr);

uint32_t bytesPerTexFormat(VkFormat fmt);

size_t allocateVertexBuffer(
    VulkanRenderDevice& vkDev, 
    VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, 
    size_t vertexDataSize, const void* vertexData, 
    size_t indexDataSize, const void* indexData);

bool createTexturedVertexBuffer(
    VulkanRenderDevice& vkDev,
    const char* fileName,
    VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory,
    size_t* vertexBufferSize, size_t* indexBufferSize);

bool createDescriptorPool(
    VulkanRenderDevice& vkDev, 
    uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, 
    VkDescriptorPool* descriptorPool);

inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags, uint32_t descriptorCount = 1)
{
    return VkDescriptorSetLayoutBinding
    {
        .binding = binding,
        .descriptorType = descriptorType,
        .descriptorCount = descriptorCount,
        .stageFlags = shaderStageFlags,
        .pImmutableSamplers = nullptr
    };
}

inline VkWriteDescriptorSet bufferWriteDescriptorSet(VkDescriptorSet ds, const VkDescriptorBufferInfo* bi, uint32_t bindIndex, VkDescriptorType dType)
{
    return VkWriteDescriptorSet
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = ds,
        .dstBinding = bindIndex,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = dType,
        .pImageInfo = nullptr,
        .pBufferInfo = bi,
        .pTexelBufferView = nullptr
    };
}

inline VkWriteDescriptorSet imageWriteDescriptorSet(VkDescriptorSet ds, const VkDescriptorImageInfo* ii, uint32_t bindIndex)
{
    return VkWriteDescriptorSet
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = ds,
        .dstBinding = bindIndex,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = ii,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };
}

bool createPipelineLayout(VkDevice device, VkDescriptorSetLayout dsLayout, VkPipelineLayout* VkPipelineLayout);

bool createPipelineLayoutWithConstants(VkDevice device, VkDescriptorSetLayout dsLayout, VkPipelineLayout* pipelineLayout, uint32_t vtxConstSize, uint32_t fragConstSize);

struct RenderPassCreateInfo final
{
    bool clearColor = false;
    bool clearDepth = false;
    uint8_t flags = 0;
};

struct RenderPass
{
    RenderPass() = default;
    explicit RenderPass(
        VulkanRenderDevice& vkDev, 
        bool useDepth = true, 
        const RenderPassCreateInfo& ci = RenderPassCreateInfo());

    RenderPassCreateInfo info;
    VkRenderPass handle = VK_NULL_HANDLE;
};

enum ERenderPassBit : uint8_t
{
    //clear the attachment
    ERPB_FIRST = 0x01,
    // transition to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    ERPB_LAST = 0X02,
    // transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    ERPB_OFFSCREEN = 0x04,
    // keep VK_IMAGE_LAYOUT_*_ATTACHMENT_OPTIMAL
    ERPB_OFFSCREEN_INTERNAL = 0x08
};

bool createColorAndDepthRenderPass(
    VulkanRenderDevice& vkDev, 
    bool useDepth, 
    VkRenderPass* renderPass, 
    const RenderPassCreateInfo& ci, 
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM);

bool createColorAndDepthFramebuffer(
    VulkanRenderDevice& vkDev, 
    uint32_t width, uint32_t height, 
    VkRenderPass renderPass, 
    VkImageView colorImageView, VkImageView depthImageView, 
    VkFramebuffer* framebuffer);

bool createColorAndDepthFramebuffers(
    VulkanRenderDevice& vkDev, 
    VkRenderPass renderPass, 
    VkImageView depthImageView, 
    std::vector<VkFramebuffer>& framebuffers);

bool createGraphicsPipeline(
    VulkanRenderDevice& vkDev,
    uint32_t width, uint32_t height,
    VkRenderPass renderPass,
    VkPipelineLayout pipelineLayout,
    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
    VkPipeline* pipeline);

bool createGraphicsPipeline(
    VulkanRenderDevice& vkDev,
    VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
    const std::vector<const char*>& shaderFiles,
    VkPipeline* pipeline,
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    bool useDepth = true,
    bool useBlending = true,
    bool dynamicScissorState = false,
    int32_t customWidth = -1,
    int32_t customHeight = -1,
    uint32_t numPatchControlPoints = 0);

VkResult createComputePipeline(
    VkDevice device,
    VkShaderModule computeShader,
    VkPipelineLayout pipelineLayout,
    VkPipeline* pipeline);

bool executeComputeShader(
    VulkanRenderDevice& vkDev,
    VkPipeline pipeline, VkPipelineLayout pipelineLayout,
    VkDescriptorSet descriptorSet,
    uint32_t xsize, uint32_t ysize, uint32_t zsize);