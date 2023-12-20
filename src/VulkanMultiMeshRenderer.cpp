#include "VulkanMultiMeshRenderer.h"
#include <stdio.h>
#include <vector>
#include <array>

#include <glm/glm.hpp>

VulkanMultiMeshRenderer::VulkanMultiMeshRenderer(VulkanRenderDevice &vkDev, const char *meshFile, const char *drawDataFile, const char *materialFile, const char *vertShaderFile, const char *fragShaderFile) :
    vkDev(vkDev),
    VulkanRendererBase(vkDev, VulkanImage())
{
    if(!createColorAndDepthRenderPass(vkDev, false, &m_renderPass, RenderPassCreateInfo()))
    {
        printf("Failed to create render pass\n");
        exit(EXIT_FAILURE);
    }

    p_framebufferWidth = &vkDev.framebufferWidth;
    p_framebufferHeight = &vkDev.framebufferHeight;

    createDepthResources(vkDev, *p_framebufferWidth, *p_framebufferHeight, m_depthTexture);
    
    // loadInstanceData(instanceDataFile);
    loadDrawData(drawDataFile);

    MeshFileHeader header = loadMeshData(meshFile, m_meshData);

    const uint32_t indirectDataSize = m_maxShapes * sizeof(VkDrawIndirectCommand);
    m_maxDrawDataSize = m_maxShapes * sizeof(DrawData);
    m_maxMaterialSize = 1024;

    m_countBuffers.resize(vkDev.swapchainImages.size());
    m_countBuffersMemory.resize(vkDev.swapchainImages.size());

    m_drawDataBuffers.resize(vkDev.swapchainImages.size());
    m_drawDataBuffersMemory.resize(vkDev.swapchainImages.size());

    m_indirectBuffers.resize(vkDev.swapchainImages.size());
    m_indirectBuffersMemory.resize(vkDev.swapchainImages.size());

    if(!createBuffer(vkDev.device, vkDev.physicalDevice, m_maxMaterialSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_materialBuffer, m_materialBufferMemory))
    {
        printf("Cannot create material buffer\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    m_maxVertexBufferSize = header.vertexDataSize;
    m_maxIndexBufferSize = header.indexDataSize;

    // fetch device properties to find the minimum storage buffer alignment value
    VkPhysicalDeviceProperties devProps;
    vkGetPhysicalDeviceProperties(vkDev.physicalDevice, &devProps);
    const uint32_t offsetAlignment = static_cast<uint32_t>(devProps.limits.minStorageBufferOffsetAlignment);
    // If vertex data doesn't meet alignment requirements, add zeros
    if((m_maxVertexBufferSize & (offsetAlignment -1)) != 0)
    {
        int floats = (offsetAlignment - (m_maxVertexBufferSize & (offsetAlignment - 1))) / sizeof(float);
        for(int ii = 0; ii < floats; ii++)
        {
            m_meshData.vertexData.push_back(0);
        }
        // update the max vertex size
        m_maxVertexBufferSize = (m_maxVertexBufferSize + offsetAlignment) & ~(offsetAlignment - 1);
    }

    if(!createBuffer(vkDev.device, vkDev.physicalDevice, m_maxVertexBufferSize + m_maxIndexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_storageBuffer, m_storageBufferMemory))
    {
        printf("Cannot create vertex/index buffer\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    updateGeometryBuffers(vkDev, header.vertexDataSize, m_meshData.vertexData.data(), header.indexDataSize, m_meshData.indexData.data());

    for(size_t i = 0; i < vkDev.swapchainImages.size(); i++)
    {
        if(!createBuffer(vkDev.device, vkDev.physicalDevice, indirectDataSize, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_indirectBuffers[i], m_indirectBuffersMemory[i]))
        {
            printf("Cannot create indirect buffer at index %lu\n", i);
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
        updateIndirectBuffers(vkDev, i);

        if(!createBuffer(vkDev.device, vkDev.physicalDevice, m_maxDrawDataSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_drawDataBuffers[i], m_drawDataBuffersMemory[i]))
        {
            printf("Cannot create draw data buffer at index %lu", i);
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
        updateDrawDataBuffer(vkDev, i, m_maxDrawDataSize, m_shapes.data());
        
        if(!createBuffer(vkDev.device, vkDev.physicalDevice, sizeof(uint32_t), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_countBuffers[i], m_countBuffersMemory[i]))
        {
            printf("Cannot create count buffer at index %lu", i);
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
        updateCountBuffer(vkDev, i, m_maxShapes);
    }

    if (
            !createUniformBuffers(vkDev, sizeof(glm::mat4)) ||
            !createColorAndDepthFramebuffers(vkDev, m_renderPass, VK_NULL_HANDLE, m_swapchainFramebuffers) ||
            !createDescriptorPool(vkDev, 1, 4, 0, &m_descriptorPool) ||
            !createDescriptorSet(vkDev) ||
            !createPipelineLayout(vkDev.device, m_descriptorSetLayout, &m_pipelineLayout) ||
            !createGraphicsPipeline(vkDev, m_renderPass, m_pipelineLayout, {vertShaderFile, fragShaderFile}, &m_graphicsPipeline)
        )
    {
        printf("Failed to create a MultiMeshRenderer pipeline\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
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
        vkCmdDrawIndirect(commandBuffer, m_indirectBuffers[currentImage], 0, m_maxShapes, sizeof(VkDrawIndirectCommand));
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
        for(uint32_t i = 0; i < m_maxShapes; i++)
        {
            const uint32_t j = m_shapes[i].meshIndex;
            const uint32_t lod = m_shapes[i].LOD;
            data[i] = 
            {
                .vertexCount = static_cast<uint32_t>(m_meshData.meshes[j].getLODIndicesCount(lod)),
                .instanceCount = visibility ? (visibility[i] ? 1u : 0u) : 1u,
                .firstVertex = 0,
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

void VulkanMultiMeshRenderer::updateDrawDataBuffer(VulkanRenderDevice &vkDev, size_t currentImage, uint32_t drawDataSize, const void *drawData)
{
    uploadBufferData(vkDev, m_drawDataBuffersMemory[currentImage], 0, drawData, drawDataSize);
}

void VulkanMultiMeshRenderer::updateCountBuffer(VulkanRenderDevice &vkDev, size_t currentImage, uint32_t itemCount)
{
    uploadBufferData(vkDev, m_countBuffersMemory[currentImage], 0, &itemCount, sizeof(uint32_t));
}

// void VulkanMultiMeshRenderer::loadInstanceData(const char *instanceFile)
// {
//     FILE* f = fopen(instanceFile, "rb");
//     fseek(f, 0, SEEK_END);
//     size_t fsize = ftell(f);
//     fseek(f, 0, SEEK_SET);

//     m_maxInstances = static_cast<uint32_t>(fsize / sizeof(InstanceData));
//     m_instances.resize(m_maxInstances);
//     if(fread(m_instances.data(), sizeof(InstanceData), m_maxInstances, f) != m_maxInstances)
//     {
//         printf("Unable to read instance data\n");
//         exit(255);
//     }
//     fclose(f);
// }

bool VulkanMultiMeshRenderer::createDescriptorSet(VulkanRenderDevice &vkDev)
{
    const std::array<VkDescriptorSetLayoutBinding, 5> bindings =
    {
        descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        // vertices of this->storagebuffer
        descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        // indices of this->storagebuffer
        descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        // draw data this->drawdatabuffer
        descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        // material data
        descriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
    };

    const VkDescriptorSetLayoutCreateInfo layout_info =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    VK_CHECK(vkCreateDescriptorSetLayout(vkDev.device, &layout_info, nullptr, &m_descriptorSetLayout));
    
    std::vector<VkDescriptorSetLayout> layouts(vkDev.swapchainImages.size(), m_descriptorSetLayout);
    const VkDescriptorSetAllocateInfo alloc_info =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(vkDev.swapchainImages.size()),
        .pSetLayouts = layouts.data()
    };

    m_descriptorSets.resize(vkDev.swapchainImages.size());
    VK_CHECK(vkAllocateDescriptorSets(vkDev.device, &alloc_info, m_descriptorSets.data()));
    
    for( size_t i = 0; i < vkDev.swapchainImages.size(); i++)
    {
        VkDescriptorSet ds = m_descriptorSets[i];

        const VkDescriptorBufferInfo bufferInfo1 = { m_uniformBuffers[i], 0, sizeof(glm::mat4) };
        const VkDescriptorBufferInfo bufferInfo2 = { m_storageBuffer, 0, m_maxVertexBufferSize };
        const VkDescriptorBufferInfo bufferInfo3 = { m_storageBuffer, m_maxVertexBufferSize, m_maxIndexBufferSize };
        const VkDescriptorBufferInfo bufferInfo4 = { m_drawDataBuffers[i], 0, m_maxDrawDataSize };
        const VkDescriptorBufferInfo bufferInfo5 = { m_materialBuffer, 0, m_maxMaterialSize };

        const std::array<VkWriteDescriptorSet, 5> descriptor_writes =
        {
            bufferWriteDescriptorSet(ds, &bufferInfo1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
            bufferWriteDescriptorSet(ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
            bufferWriteDescriptorSet(ds, &bufferInfo3, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
            bufferWriteDescriptorSet(ds, &bufferInfo4, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
            bufferWriteDescriptorSet(ds, &bufferInfo5, 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        };
        vkUpdateDescriptorSets(vkDev.device, static_cast<uint32_t>(descriptor_writes.size()),descriptor_writes.data(), 0, nullptr);
    }

    return true;
}

void VulkanMultiMeshRenderer::loadDrawData(const char *drawDataFile)
{
    FILE* f = fopen(drawDataFile, "rb");
    if(!f)
    {
        printf("Unable to open draw data file. Run MeshConvert first.\n");
        exit(EXIT_FAILURE);
    }

    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    m_maxShapes = static_cast<uint32_t>(fsize / sizeof(DrawData));

    printf("Reading draw data itmes: %d\n", (int)m_maxShapes); fflush(stdout);
    m_shapes.resize(m_maxShapes);

    if(fread(m_shapes.data(), sizeof(DrawData), m_maxShapes, f) != m_maxShapes)
    {
        printf("Unable to read draw data\n");
        exit(EXIT_FAILURE);
    }

    fclose(f);
}
