#pragma once

#include <vulkan/vulkan.h>
#include "VKUtils.h"

#include <vector>


class VulkanRendererBase
{
public:

    explicit VulkanRendererBase(VulkanRenderDevice& vkDev, VulkanImage depthTexture) :
        p_dev(&vkDev.device),
        p_framebufferWidth(&vkDev.framebufferWidth),
        p_framebufferHeight(&vkDev.framebufferHeight),
        m_depthTexture(depthTexture)
    {}

    virtual ~VulkanRendererBase();
    virtual void fillCommandBuffer(const VkCommandBuffer& commandBuffer, size_t currentImage) = 0;

    inline VulkanImage getDepthTexture() const { return m_depthTexture; }

protected:

    void beginRenderPass(VkCommandBuffer commandBuffer, size_t currentImage);
    bool createUniformBuffers(VulkanRenderDevice& vkDev, size_t uniformDataSize);

    uint32_t* p_framebufferWidth = nullptr;
    uint32_t* p_framebufferHeight = nullptr;
    VkDevice* p_dev = nullptr;

    VkDescriptorSetLayout m_descriptorSetLayout = nullptr;
    VkDescriptorPool m_descriptorPool = nullptr;
    std::vector<VkDescriptorSet> m_descriptorSets;

    std::vector<VkFramebuffer> m_swapchainFramebuffers;

    VulkanImage m_depthTexture;
    VkRenderPass m_renderPass = nullptr;

    VkPipelineLayout m_pipelineLayout = nullptr;
    VkPipeline m_graphicsPipeline = nullptr;

    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
};