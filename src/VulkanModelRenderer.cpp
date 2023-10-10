#include "VulkanModelRenderer.h"

#include <stdio.h>
#include <stdlib.h>
#include <array>

VulkanModelRenderer::VulkanModelRenderer(VulkanRenderDevice &vkDev, const char *modelFile, const char *textureFile, uint32_t uniformDataSize) :
    VulkanRendererBase(vkDev, VulkanImage())
{
    if(!createTexturedVertexBuffer(vkDev, modelFile, &m_storageBuffer, &m_storageBufferMemory, &m_vertexBufferSize, &m_indexBufferSize))
    {
        printf("VulkanModelRenderer: createTexturedVertexBuffer failed\n");
        exit(EXIT_FAILURE);
    }
    createTextureImage(vkDev, textureFile, m_texture.image, m_texture.imageMemory);
    createImageView(vkDev.device, m_texture.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &m_texture.imageView);
    createTextureSampler(vkDev.device, &m_textureSampler);


    std::vector<const char*> shaders = 
    {
        "shaders/VK02.vert",
        "shaders/VK02.frag",
        "shaders/VK02.geom"
    };

    if( !createDepthResources(vkDev, vkDev.framebufferWidth, vkDev.framebufferHeight, m_depthTexture) ||
        !createColorAndDepthRenderPass(vkDev, true, &m_renderPass, RenderPassCreateInfo()) ||
        !createUniformBuffers(vkDev, uniformDataSize) ||
        !createColorAndDepthFramebuffers(vkDev, m_renderPass, m_depthTexture.imageView, m_swapchainFramebuffers) ||
        !createDescriptorPool(vkDev, 1, 2, 1, &m_descriptorPool) ||
        !createDescriptorSet(vkDev, uniformDataSize) ||
        !createPipelineLayout(vkDev.device, m_descriptorSetLayout, &m_pipelineLayout) ||
        !createGraphicsPipeline(vkDev, m_renderPass, m_pipelineLayout, shaders, &m_graphicsPipeline))
    {
        printf("VulkanModelRenderer: failed to create pipeline\n");
        exit(EXIT_FAILURE);
    }
}

VulkanModelRenderer::~VulkanModelRenderer()
{
    vkDestroyBuffer(*p_dev, m_storageBuffer, nullptr);
    vkFreeMemory(*p_dev, m_storageBufferMemory, nullptr);

    vkDestroySampler(*p_dev, m_textureSampler, nullptr);
    destroyVulkanImage(*p_dev, m_texture);

    if(!bIsExternalDepth)
    {
        destroyVulkanImage(*p_dev, m_depthTexture);
    }
}

void VulkanModelRenderer::fillCommandBuffer(const VkCommandBuffer &commandBuffer, size_t currentImage)
{
    beginRenderPass(commandBuffer, currentImage);
    vkCmdDraw(commandBuffer, static_cast<uint32_t>(m_indexBufferSize/(sizeof(uint32_t))), 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
}

void VulkanModelRenderer::updateUniformBuffer(VulkanRenderDevice &vkDev, uint32_t currentImage, const void *data, size_t dataSize)
{
    uploadBufferData(vkDev, m_uniformBuffersMemory[currentImage], 0, data, dataSize);
}

bool VulkanModelRenderer::createDescriptorSet(VulkanRenderDevice &vkDev, uint32_t uniformDataSize)
{
    const std::array<VkDescriptorSetLayoutBinding, 4> bindings =
    {
        descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    const VkDescriptorSetLayoutCreateInfo layoutInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };
    VK_CHECK(vkCreateDescriptorSetLayout(vkDev.device, &layoutInfo, nullptr, &m_descriptorSetLayout));

    std::vector<VkDescriptorSetLayout> layouts(vkDev.swapchainImages.size(), m_descriptorSetLayout);
    const VkDescriptorSetAllocateInfo allocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };
    m_descriptorSets.resize(vkDev.swapchainImages.size());
    VK_CHECK(vkAllocateDescriptorSets(vkDev.device, &allocInfo, m_descriptorSets.data()));

    for(size_t i = 0; i < vkDev.swapchainImages.size(); i++)
    {
        VkDescriptorSet ds = m_descriptorSets[i];

        const VkDescriptorBufferInfo bufferInfo1 = { m_uniformBuffers[i], 0, uniformDataSize};
        const VkDescriptorBufferInfo bufferInfo2 = { m_storageBuffer, 0, m_vertexBufferSize };
        const VkDescriptorBufferInfo bufferInfo3 = { m_storageBuffer, m_vertexBufferSize, m_indexBufferSize };
        const VkDescriptorImageInfo imageInfo = { m_textureSampler, m_texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

        const std::array<VkWriteDescriptorSet, 4> descriptorWrites =
        {
            bufferWriteDescriptorSet(ds, &bufferInfo1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
            bufferWriteDescriptorSet(ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
            bufferWriteDescriptorSet(ds, &bufferInfo3, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
            imageWriteDescriptorSet(ds, &imageInfo, 3)
        };

        vkUpdateDescriptorSets(vkDev.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
    
    return true;
}
