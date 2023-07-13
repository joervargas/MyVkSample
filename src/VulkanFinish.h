#pragma once

#include "VulkanRendererBase.h"


class VulkanFinish : public VulkanRendererBase
{
public:

    VulkanFinish(VulkanRenderDevice& vkDev, VulkanImage depthTexture);

    virtual void fillCommandBuffer(const VkCommandBuffer& commandBuffer, size_t currentImage) override;

};