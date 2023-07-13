#pragma once

#include "VulkanRendererBase.h"

#include <glm/glm.hpp>
using glm::mat4;

class VulkanCubeRenderer : public VulkanRendererBase
{
public:

    VulkanCubeRenderer(VulkanRenderDevice& vkDev, VulkanImage inDepthTexture, const char* textureFile);

    virtual ~VulkanCubeRenderer();

    virtual void fillCommandBuffer(const VkCommandBuffer& commandBuffer, size_t currentImage) override;

    void updateUniformBuffer(VulkanRenderDevice& vkDev, uint32_t currentImage, const mat4& m);

private:

    VkSampler textureSampler;

    VulkanImage texture;

    bool createDescriptorSet(VulkanRenderDevice& vkDev);

};