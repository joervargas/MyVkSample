#pragma once

#include "VulkanRendererBase.h"

class VulkanClear : public VulkanRendererBase
{
public:

    VulkanClear(VulkanRenderDevice& vkDev, VulkanImage depthTexture);

    virtual void fillCommandBuffer(const VkCommandBuffer& commandBuffer, size_t currentImage) override;

private:

    bool b_shouldClearDepth;

};