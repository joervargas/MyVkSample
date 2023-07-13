#pragma once

#include "VulkanRendererBase.h"

class VulkanModelRenderer : public VulkanRendererBase
{
public:

    VulkanModelRenderer(VulkanRenderDevice& vkDev, const char* modelFile, const char* textureFile, uint32_t uniformDataSize);

    virtual ~VulkanModelRenderer();

    virtual void fillCommandBuffer(const VkCommandBuffer& VkCommandBuffer, size_t currentImage) override;

    void updateUniformBuffer(VulkanRenderDevice& vkDev, uint32_t currentImage, const void* data, size_t dataSize);

private:

    bool bIsExternalDepth = false;

    size_t m_vertexBufferSize;
    size_t m_indexBufferSize;
    VkBuffer m_storageBuffer;
    VkDeviceMemory m_storageBufferMemory;

    VkSampler m_textureSampler;
    VulkanImage m_texture;

    bool createDescriptorSet(VulkanRenderDevice& vkDev, uint32_t uniformDataSize);

};