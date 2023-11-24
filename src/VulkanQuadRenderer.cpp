#include "VulkanQuadRenderer.h"


static constexpr int MAX_QUADS = 256;

VulkanQuadRenderer::VulkanQuadRenderer(VulkanRenderDevice &vkDev, const std::vector<std::string> &textureFiles) : 
    vkDev(vkDev),
    VulkanRendererBase(vkDev, VulkanImage())
{
    const size_t imgCount = vkDev.swapchainImages.size();

    p_framebufferWidth = &vkDev.framebufferWidth;
    p_framebufferHeight = &vkDev.framebufferHeight;

    storageBuffers.resize(imgCount);
    storageBuffersMemory.resize(imgCount);

    vertexBufferSize = MAX_QUADS * 6 * sizeof(VertexData);

    for(size_t i = 0; i < imgCount; i++)
    {
        if(!createBuffer(
                vkDev.device, vkDev.physicalDevice, 
                vertexBufferSize, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                storageBuffers[i], storageBuffersMemory[i])
            )
        {
            printf("Cannot create vertex buffer\n");
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
    }

    if( !createUniformBuffers(vkDev, sizeof(ConstBuffer)))
    {
        printf("Cannot create data buffers\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    const size_t numTextureFiles = textureFiles.size();
    textures.resize(numTextureFiles);
    textureSamplers.resize(numTextureFiles);
    for(size_t i = 0; i < numTextureFiles; i++)
    {
        printf("\rLoading texture %u...", unsigned(i));
        createTextureImage(vkDev, textureFiles[i].c_str(), textures[i].image, textures[i].imageMemory);
        createImageView(vkDev.device, textures[i].image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &textures[i].imageView);
        createTextureSampler(vkDev.device, &textureSamplers[i]);
    }
    printf("\n");

    if ( !createDepthResources(vkDev, vkDev.framebufferWidth, vkDev.framebufferHeight, m_depthTexture) ||
         !createDescriptorPool(vkDev, 1, 1, 1 * static_cast<uint32_t>(textures.size()), &m_descriptorPool) ||
         !createDescriptorSet(vkDev) ||
         !createColorAndDepthRenderPass(vkDev, false, &m_renderPass, RenderPassCreateInfo()) ||
         !createPipelineLayoutWithConstants(vkDev.device, m_descriptorSetLayout, &m_pipelineLayout, sizeof(ConstBuffer), 0) ||
         !createGraphicsPipeline(vkDev, m_renderPass, m_pipelineLayout, { "shaders/VK02_texture_array.vert", "shaders/VK02_texture_array.frag" }, &m_graphicsPipeline)
        )
    {
        printf("Failed to create pipeline\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    createColorAndDepthFramebuffers(vkDev, m_renderPass, VK_NULL_HANDLE, m_swapchainFramebuffers);
}

VulkanQuadRenderer::~VulkanQuadRenderer()
{
    VkDevice device = vkDev.device;

    for (size_t i = 0; i < storageBuffers.size(); i++)
    {
        vkDestroyBuffer(device, storageBuffers[i], nullptr);
        vkFreeMemory(device, storageBuffersMemory[i], nullptr);
    }

    for (size_t i = 0; i < textures.size(); i++)
    {
        vkDestroySampler(device, textureSamplers[i], nullptr);
        destroyVulkanImage(device, textures[i]);
    }

    destroyVulkanImage(device, m_depthTexture);
}

void VulkanQuadRenderer::updateBuffer(VulkanRenderDevice &vkDev, size_t i)
{
    uploadBufferData(vkDev, storageBuffersMemory[i], 0, quads.data(), quads.size());
}

void VulkanQuadRenderer::pushConstants(VkCommandBuffer cmdBuffer, uint32_t texture_index, const glm::vec2 &offset)
{
    const ConstBuffer constBuffer = { offset, texture_index };
    vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ConstBuffer), &constBuffer);
}

void VulkanQuadRenderer::fillCommandBuffer(const VkCommandBuffer &cmdBuffer, size_t currentImage)
{
    if(quads.empty()) return;

    beginRenderPass(cmdBuffer, currentImage);

    vkCmdDraw(cmdBuffer, static_cast<uint32_t>(quads.size()), 1, 0, 0);
    vkCmdEndRenderPass(cmdBuffer);
}

void VulkanQuadRenderer::quad(float x1, float y1, float x2, float y2)
{
    VertexData v1 { { x1, y1, 0 }, { 0, 0 } };
    VertexData v2 { { x2, y1, 0 }, { 1, 0 } };
    VertexData v3 { { x2, y2, 0 }, { 1, 1 } };
    VertexData v4 { { x1, y2, 0 }, { 0, 1 } };

    quads.push_back( v1 );
    quads.push_back( v2 );
    quads.push_back( v3 );

    quads.push_back( v1 );
    quads.push_back( v3 );
    quads.push_back( v4 );
}

void VulkanQuadRenderer::clear()
{
    quads.clear();
}

bool VulkanQuadRenderer::createDescriptorSet(VulkanRenderDevice &vkDev)
{
    const std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
        descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, static_cast<uint32_t>(textures.size()))
    };

    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    VK_CHECK(vkCreateDescriptorSetLayout(vkDev.device, &layoutInfo, nullptr, &m_descriptorSetLayout));

    const std::vector<VkDescriptorSetLayout> layouts(vkDev.swapchainImages.size(), m_descriptorSetLayout);

    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(vkDev.swapchainImages.size()),
        .pSetLayouts = layouts.data()
    };

    m_descriptorSets.resize(vkDev.swapchainImages.size());

    VK_CHECK(vkAllocateDescriptorSets(vkDev.device, &allocInfo, m_descriptorSets.data()));

    std::vector<VkDescriptorImageInfo> textureDescriptors(textures.size());
    for (size_t i = 0; i < textures.size(); i++)
    {
        textureDescriptors[i] = VkDescriptorImageInfo {
            .sampler = textureSamplers[i],
            .imageView = textures[i].imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
    }

    for (size_t i = 0; i < vkDev.swapchainImages.size(); i++)
    {
        const VkDescriptorBufferInfo bufferInfo1 = { .buffer = m_uniformBuffers[i], .offset = 0, .range = sizeof(ConstBuffer) };
        const VkDescriptorBufferInfo bufferInfo2 = { .buffer = storageBuffers[i], .offset = 0, .range = vertexBufferSize };

        const std::array<VkWriteDescriptorSet, 3> descriptorWrites = {
            VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &bufferInfo1
            },
            VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &bufferInfo2
            },
            VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = static_cast<uint32_t>(textures.size()),
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = textureDescriptors.data()
            },
        };

        vkUpdateDescriptorSets(vkDev.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
    return true;
}
