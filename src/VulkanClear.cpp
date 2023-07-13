#include "VulkanClear.h"
#include <stdio.h>



VulkanClear::VulkanClear(VulkanRenderDevice &vkDev, VulkanImage depthTexture) :
    VulkanRendererBase(vkDev, depthTexture),
    b_shouldClearDepth(depthTexture.image != VK_NULL_HANDLE)
{
    const RenderPassCreateInfo rpci =
    {
        .clearColor = true,
        .clearDepth = true,
        .flags = ERenderPassBit::ERPB_FIRST
    };
    if(!createColorAndDepthRenderPass(vkDev, b_shouldClearDepth, &m_renderPass, rpci))
    {
        printf("VulkanClear: failed to create render pass\n");
        exit(EXIT_FAILURE);
    }

    createColorAndDepthFramebuffers(vkDev, m_renderPass, m_depthTexture.imageView, m_swapchainFramebuffers);
}

void VulkanClear::fillCommandBuffer(const VkCommandBuffer& commandBuffer, size_t currentImage)
{
    const VkClearValue clearValues[2] =
    {
        VkClearValue{ .color = { 1.0f, 1.0f, 1.0f, 1.0f } },
        VkClearValue{ .depthStencil = { 1.0f, 0 } }
    };

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
        .renderArea = screenRect,
        .clearValueCount = b_shouldClearDepth ? 2u : 1u,
        .pClearValues = &clearValues[0]
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(commandBuffer);
}
