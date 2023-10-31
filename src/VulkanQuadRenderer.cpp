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
        createImageView(vkDev.device, textures[i].image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &texturex[i].imageView);
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
