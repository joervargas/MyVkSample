#pragma once

#include "VulkanRendererBase.h"
#include "glm/glm.hpp"

class VulkanCanvas : public VulkanRendererBase
{
public:

    explicit VulkanCanvas(VulkanRenderDevice& vkDev, VulkanImage depth);

    virtual ~VulkanCanvas();

    virtual void fillCommandBuffer(const VkCommandBuffer& commandBuffer, size_t currentImage) override;

    void updateBuffer(VulkanRenderDevice& vkDev, size_t currentImage);

    void updateUniformBuffer(VulkanRenderDevice& vkDev, const glm::mat4& m, float time, uint32_t currentImage);

    void clear();

    void line(const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& color);

    void plane3d(
        const glm::vec3& origin, 
        const glm::vec3& v1, const glm::vec3& v2, 
        int n1, int n2, 
        float s1, float s2, 
        const glm::vec4& color, const glm::vec4& outlineColor
    );

private:

    struct VertexData
    {
        glm::vec3 position;
        glm::vec4 color;
    };

    struct UniformBuffer
    {
        glm::mat4 mvp;
        float time;
    };

    std::vector<VertexData> m_lines;
    std::vector<VkBuffer> m_storageBuffer;
    std::vector<VkDeviceMemory> m_storageBufferMemory;

    bool createDescriptorSet(VulkanRenderDevice& vkDev);

    static constexpr uint32_t kMaxLinesCount = 65536;
    static constexpr uint32_t kMaxLinesDataSize = 2 * kMaxLinesCount * sizeof(VulkanCanvas::VertexData);

};