#pragma once

#include "VulkanRendererBase.h"
#include <string>
#include <stdio.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>


class VulkanQuadRenderer : public VulkanRendererBase
{
public:

    VulkanQuadRenderer(VulkanRenderDevice& vkDev, const std::vector<std::string>& textureFiles);
    virtual ~VulkanQuadRenderer();

    void updateBuffer(VulkanRenderDevice& vkDev, size_t i);
    void pushConstants(VkCommandBuffer cmdBuffer, uint32_t texture_index, const glm::vec2& offset);

    virtual void fillCommandBuffer(const VkCommandBuffer& cmdBuffer, size_t currentImage) override;

    void quad(float x1, float y1, float x2, float y2);
    void clear();

private:

    bool createDescriptorSet(VulkanRenderDevice& vkDev);

    struct ConstBuffer
    {
        glm::vec2 offset;
        uint32_t textureIndex;
    };

    struct VertexData
    {
        glm::vec3 pos;
        glm::vec2 tc;
    };

    VulkanRenderDevice& vkDev;

    std::vector<VertexData> quads;

    size_t vertexBufferSize;
    size_t indexBufferSize;

    std::vector<VkBuffer> storageBuffers;
    std::vector<VkDeviceMemory> storageBuffersMemory;

    std::vector<VulkanImage> textures;
    std::vector<VkSampler> textureSamplers;

};