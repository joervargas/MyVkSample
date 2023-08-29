#include "VulkanMultiMeshRenderer.h"
#include <stdio.h>


VulkanMultiMeshRenderer::VulkanMultiMeshRenderer(VulkanRenderDevice &vkDev, const char *meshFile, const char *drawDataFile, const char *materialFile, const char *vtxShaderFile, const char *fragShaderFile) :
    vkDev(vkDev),
    VulkanRendererBase(vkDev, VulkanImage())
{

}

VulkanMultiMeshRenderer::~VulkanMultiMeshRenderer()
{
    vkDestroyBuffer(vkDev.device, m_storageBuffer, nullptr);
    vkFreeMemory(vkDev.device, m_storageBufferMemory, nullptr);

    for(size_t i = 0; i < m_swapchainFramebuffers.size(); i++)
    {
        vkDestroyBuffer(vkDev.device, m_instanceBuffers[i], nullptr);
        vkFreeMemory(vkDev.device, m_instanceBuffersMemory[i], nullptr);

        vkDestroyBuffer(vkDev.device, m_indirectBuffers[i], nullptr);
        vkFreeMemory(vkDev.device, m_indirectBuffersMemory[i], nullptr);
    }

    vkDestroyBuffer(vkDev.device, m_materialBuffer, nullptr);
    vkFreeMemory(vkDev.device, m_materialBufferMemory, nullptr);

    destroyVulkanImage(vkDev.device, m_depthTexture);
}

void VulkanMultiMeshRenderer::fillCommandBuffer(const VkCommandBuffer &commandBuffer, size_t currentImage)
{
    beginRenderPass(commandBuffer, currentImage);
        vkCmdDrawIndirect(commandBuffer, m_indirectBuffers[currentImage], 0, m_maxInstances, sizeof(VkDrawIndirectCommand));
    vkCmdEndRenderPass(commandBuffer);
}

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
    VkDrawIndirectCommand* data = nullptr;
    vkMapMemory(vkDev.device, m_indirectBuffersMemory[currentImage], 0, 2 * sizeof(VkDrawIndirectCommand), 0, (void**)&data);
        for(uint32_t i = 0; i < m_maxInstances; i++)
        {
            const uint32_t j = instances[i].meshIndex;
            data[i] = 
            {
                .vertexCount = static_cast<uint32_t>(meshes[j].getLODIndicesCount(instances[i].LOD) / sizeof(uint32_t)),
                .instanceCount = 1,
                .firstVertex = static_cast<uint32_t>(meshes[j].streamOffset[0] / meshes[j].streamElementSize[0]),
                .firstInstance = i
            };
        }
    vkUnmapMemory(vkDev.device, m_indirectBuffersMemory[currentImage]);
}

void VulkanMultiMeshRenderer::updateGeometryBuffers(VulkanRenderDevice &vkDev, uint32_t vertexCount, const void *vertices, uint32_t indexCount, const void *indices)
{
    uploadBufferData(vkDev, m_storageBufferMemory, 0, vertices, vertexCount);
    uploadBufferData(vkDev, m_storageBufferMemory, m_maxVertexBufferSize, indices, indexCount);
}

void VulkanMultiMeshRenderer::loadInstanceData(const char *instanceFile)
{
    FILE* f = fopen(instanceFile, "rb");
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    m_maxInstances = static_cast<uint32_t>(fsize / sizeof(InstanceData));
    instances.resize(m_maxInstances);
    if(fread(instances.data(), sizeof(InstanceData), m_maxInstances, f) != m_maxInstances)
    {
        printf("Unable to read instance data\n");
        exit(255);
    }
    fclose(f);
}

MeshFileHeader VulkanMultiMeshRenderer::loadMeshData(const char *meshFile)
{
    MeshFileHeader header;
    FILE* f = fopen(meshFile, "rb");
    if(fread(&header, 1, sizeof(header), f) != sizeof(header))
    {
        printf("Unable to read mesh file header\n");
        exit(255);
    }
    meshes.resize(header.meshCount);

    if(fread(meshes.data(), sizeof(Mesh), header.meshCount, f) != header.meshCount)
    {
        printf("Counld not read mesh descriptors\n");
        exit(255);
    }

    indexData.resize(header.indexDataSize / sizeof(uint32_t));
    vertexData.resize(header.vertexDataSize / sizeof(float));

    if( (fread(indexData.data(), 1, header.vertexDataSize, f) != header.indexDataSize) ||
        (fread(vertexData.data(), 1, header.vertexDataSize, f) != header.vertexDataSize))
    {
        printf("Unable to read index/vertexdata\n");
        exit(255);
    }
    fclose(f);

    return header;
}
