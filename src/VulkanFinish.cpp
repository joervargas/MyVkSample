#include "VulkanFinish.h"
#include <stdio.h>

VulkanFinish::VulkanFinish(VulkanRenderDevice &vkDev, VulkanImage depthTexture) :
    VulkanRendererBase(vkDev, depthTexture)
{
    const RenderPassCreateInfo rpci =
    {
        .clearColor = false,
        .clearDepth = false,
        .flags = ERenderPassBit::ERPB_LAST
    };

    if(!createColorAndDepthRenderPass(vkDev, (depthTexture.image != VK_NULL_HANDLE), &m_renderPass, rpci))
    {
        printf("VulkanFinish: failed to create render pass \n");
        exit(EXIT_FAILURE);
    }

    createColorAndDepthFramebuffers(vkDev, m_renderPass, depthTexture.imageView, m_swapchainFramebuffers);
}

void VulkanFinish::fillCommandBuffer(const VkCommandBuffer& commandBuffer, size_t currentImage)
{
    const VkRect2D screenRect =
    {
        .offset = { 0, 0 },
        .extent =
        {
            .width = *p_framebufferWidth,
            .height = *p_framebufferHeight
        }
    };

    const VkRenderPassBeginInfo renderPassInfo =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_renderPass,
        .framebuffer = m_swapchainFramebuffers[currentImage],
        .renderArea = screenRect
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdEndRenderPass(commandBuffer);
}
