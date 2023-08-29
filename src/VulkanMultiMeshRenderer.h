#pragma once

#include "VulkanRendererBase.h"

#include <VtxData.h>

#include <glm/glm.hpp>
using glm::mat4;

class VulkanMultiMeshRenderer: public VulkanRendererBase
{
public:

    VulkanMultiMeshRenderer(
        VulkanRenderDevice& vkDev,
        const char* meshFile,
        const char* drawDataFile,
        const char* materialFile,
        const char* vtxShaderFile,
        const char* fragShaderFile
    );

    ~VulkanMultiMeshRenderer();

    virtual void fillCommandBuffer(const VkCommandBuffer& commandBuffer, size_t currentImage) override;

    void updateUniformBuffer(VulkanRenderDevice& vkDev, size_t currentImage, const mat4& m);

    void updateInstanceBuffer(VulkanRenderDevice& vkDev, size_t currentImage, uint32_t instanceSize, const void* InstanceData);

    void updateIndirectBuffers(VulkanRenderDevice& vkDev, size_t currentImage, bool* visibility = nullptr);

    void updateGeometryBuffers(VulkanRenderDevice& vkDev, uint32_t vertexCount,  const void* vertices, uint32_t indexCount, const void* indices);

    void updateMaterialBuffer(VulkanRenderDevice& vkDev, uint32_t materialSize, const void* materialData);

    // void updateDrawDataBuffer(VulkanRenderDevice& vkDev, size_t currentImage, uint32_t drawDataSize, const void* drawData);

    void updateCountBuffer(VulkanRenderDevice& vkDev, size_t currentImage, uint32_t itemCount);

    std::vector<InstanceData> instances;
    std::vector<Mesh> meshes;

private:

    std::vector<uint32_t> indexData;
    std::vector<float> vertexData;

    // uint32_t m_vertexBufferSize;
    // uint32_t m_indexBufferSize;

    VulkanRenderDevice& vkDev;

    uint32_t m_maxVertexBufferSize;
    uint32_t m_maxIndexBufferSize;

    uint32_t m_maxInstances;
    uint32_t m_maxInstanceSize;

    // uint32_t m_maxShapes;

    // uint32_t m_maxDrawSize;
    uint32_t m_maxMaterialSize;

    VkBuffer m_storageBuffer;
    VkDeviceMemory m_storageBufferMemory;

    VkBuffer m_materialBuffer;
    VkDeviceMemory m_materialBufferMemory;

    std::vector<VkBuffer> m_indirectBuffers;
    std::vector<VkDeviceMemory> m_indirectBuffersMemory;

    std::vector<VkBuffer> m_instanceBuffers;
    std::vector<VkDeviceMemory> m_instanceBuffersMemory;

    // std::vector<VkBuffer> m_drawDataBuffers;
    // std::vector<VkDeviceMemory> m_drawDataBuffersMemory;

    // std::vector<VkBuffer> m_countBuffers;
    // std::vector<VkDeviceMemory> m_countBuffersMemory;

    // std::vector<DrawData> shapes;
    // MeshData m_meshData;

    bool createDescriptorSet(VulkanRenderDevice& vkDev);

    void loadInstanceData(const char* instanceFile);

    MeshFileHeader loadMeshData(const char* meshFile);

};