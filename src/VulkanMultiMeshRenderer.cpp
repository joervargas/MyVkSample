#include "VulkanMultiMeshRenderer.h"


void VulkanMultiMeshRenderer::updateUniformBuffer(VulkanRenderDevice &vkDev, size_t currentImage, const mat4 &m)
{
    uploadBufferData(vkDev, m_uniformBuffersMemory[currentImage], 0, glm::value_ptr(m), sizeof(mat4));
}

void VulkanMultiMeshRenderer::updateInstanceBuffer(VulkanRenderDevice &vkDev, size_t currentImage, uint32_t instanceSize, const void *instanceData)
{
    uploadBufferData(vkDev, m_instanceBuffersMemory[currentImage], 0, instanceData, instanceSize);
}

void VulkanMultiMeshRenderer::updateIndirectBuffers(VulkanRenderDevice &vkDev, size_t currentImage, bool *visibility)
{
    VkDrawIndexedIndirectCommand* data = nullptr;
    
}

void VulkanMultiMeshRenderer::updateGeometryBuffers(VulkanRenderDevice &vkDev, uint32_t vertexCount, const void *vertices, uint32_t indexCount, const void *indices)
{
    uploadBufferData(vkDev, m_storageBufferMemory, 0, vertices, vertexCount);
    uploadBufferData(vkDev, m_storageBufferMemory, m_maxVertexBufferSize, indices, indexCount);
}
