#include "VulkanCubeRenderer.h"
#include <cstdio>
#include <glm/ext.hpp>

VulkanCubeRenderer::VulkanCubeRenderer(VulkanRenderDevice &vkDev, VulkanImage inDepthTexture, const char *textureFile) :
    VulkanRendererBase(vkDev, inDepthTexture)
{
    createCubeTextureImage(vkDev, textureFile, texture.image, texture.imageMemory);
    createImageView(
        vkDev.device, texture.image, 
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &texture.imageView, 
        VK_IMAGE_VIEW_TYPE_CUBE, 6
    );
    createTextureSampler(vkDev.device, &textureSampler);

    std::vector<const char*> shaders = 
    {
        "shaders/VKCube.vert",
        "shaders/VKCube.frag"
    };

    if( !createColorAndDepthRenderPass(vkDev, true, &m_renderPass, RenderPassCreateInfo()) ||
        !createUniformBuffers(vkDev, sizeof(mat4)) ||
        !createColorAndDepthFramebuffers(vkDev, m_renderPass, m_depthTexture.imageView, m_swapchainFramebuffers) ||
        !createDescriptorPool(vkDev, 1, 0, 1, &m_descriptorPool) ||
        !createDescriptorSet(vkDev) ||
        !createPipelineLayout(vkDev.device, m_descriptorSetLayout, &m_pipelineLayout) ||
        !createGraphicsPipeline(vkDev, m_renderPass, m_pipelineLayout, shaders, &m_graphicsPipeline))
    {
        printf("VulkanCubeRenderer: failed to create pipeline\n");
        exit(EXIT_FAILURE);
    }
}

VulkanCubeRenderer::~VulkanCubeRenderer()
{
    vkDestroySampler(*p_dev, textureSampler, nullptr);
    destroyVulkanImage(*p_dev, texture);
}

void VulkanCubeRenderer::fillCommandBuffer(const VkCommandBuffer &commandBuffer, size_t currentImage)
{
    beginRenderPass(commandBuffer, currentImage);
    vkCmdDraw(commandBuffer, 36, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
}

void VulkanCubeRenderer::updateUniformBuffer(VulkanRenderDevice &vkDev, uint32_t currentImage, const mat4 &m)
{
    uploadBufferData(vkDev, m_uniformBuffersMemory[currentImage], 0, glm::value_ptr(m), sizeof(mat4));
}

bool VulkanCubeRenderer::createDescriptorSet(VulkanRenderDevice &vkDev)
{
    const std::array<VkDescriptorSetLayoutBinding, 2> bindings =
    {
        descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
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
        .descriptorSetCount = static_cast<uint32_t>(vkDev.swapchainImages.size()),
        .pSetLayouts = layouts.data()
    };
    m_descriptorSets.resize(vkDev.swapchainImages.size());
    VK_CHECK(vkAllocateDescriptorSets(vkDev.device, &allocInfo, m_descriptorSets.data()));

    for(size_t i = 0; i < vkDev.swapchainImages.size(); i++)
    {
        VkDescriptorSet ds = m_descriptorSets[i];

        const VkDescriptorBufferInfo bufferInfo = { m_uniformBuffers[i], 0, sizeof(mat4) };
        const VkDescriptorImageInfo imageInfo = { textureSampler, texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    
        const std::array<VkWriteDescriptorSet, 2> descriptorWrites =
        {
            bufferWriteDescriptorSet(ds, &bufferInfo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
            imageWriteDescriptorSet(ds, &imageInfo, 1)
        };

        vkUpdateDescriptorSets(vkDev.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    return true;
}
