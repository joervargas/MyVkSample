#include "VulkanRendererBase.h"
#include <stdio.h>



VulkanRendererBase::~VulkanRendererBase()
{
    for(VkBuffer buf: m_uniformBuffers)
    {
        vkDestroyBuffer(*p_dev, buf, nullptr);
    }
    for(VkDeviceMemory mem : m_uniformBuffersMemory)
    {
        vkFreeMemory(*p_dev, mem, nullptr);
    }
    if (m_descriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(*p_dev, m_descriptorSetLayout, nullptr);
    }
    if ( m_descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(*p_dev, m_descriptorPool, nullptr);
    }
    for(VkFramebuffer framebuffer : m_swapchainFramebuffers)
    {
        vkDestroyFramebuffer(*p_dev, framebuffer, nullptr);
    }
    vkDestroyRenderPass(*p_dev, m_renderPass, nullptr);
    vkDestroyPipelineLayout(*p_dev, m_pipelineLayout, nullptr);
    vkDestroyPipeline(*p_dev, m_graphicsPipeline, nullptr);
}

void VulkanRendererBase::beginRenderPass(VkCommandBuffer commandBuffer, size_t currentImage)
{
    const VkRect2D screenRect =
    {
        .offset = { 0, 0 },
        .extent = 
        {
            .width = *p_framebufferWidth,
            .height = *p_framebufferHeight
        },
    };

    const VkRenderPassBeginInfo renderPassInfo =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = m_renderPass,
        .framebuffer = m_swapchainFramebuffers[currentImage],
        .renderArea = screenRect
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    
    vkCmdBindDescriptorSets(
        commandBuffer, 
        VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 
        0, 1, &m_descriptorSets[currentImage], 
        0, nullptr
    );
}

bool VulkanRendererBase::createUniformBuffers(VulkanRenderDevice &vkDev, size_t uniformDataSize)
{
    m_uniformBuffers.resize(vkDev.swapchainImages.size());
    m_uniformBuffersMemory.resize(vkDev.swapchainImages.size());

    for(size_t i = 0; i < vkDev.swapchainImages.size(); i++)
    {
        if(!createUniformBuffer(vkDev, m_uniformBuffers[i], m_uniformBuffersMemory[i], uniformDataSize))
        {
            printf("Cannot create uniform buffer \n");
            return false;
        }
    }
    return true;
}
