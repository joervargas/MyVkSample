#include "VulkanImGui.h"

#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

constexpr uint32_t ImGuiVtxBufferSize = 512 * 1024 * sizeof(ImDrawVert);
constexpr uint32_t ImGuiIdxBufferSize = 512 * 1024 * sizeof(uint32_t);

bool createFontTexture(
        ImGuiIO& io, 
        const char* fontFile, 
        VulkanRenderDevice& vkDev, 
        VkImage& textureImage, VkDeviceMemory& textureImageMemory
    )
{
    ImFontConfig cfg = ImFontConfig();
    cfg.FontDataOwnedByAtlas = false;
    cfg.RasterizerMultiply = 1.5f;
    cfg.SizePixels = 768.0f / 32.0f;
    cfg.OversampleH = 4;
    cfg.OversampleV = 4;

    ImFont* Font = io.Fonts->AddFontFromFileTTF(fontFile, cfg.SizePixels, &cfg);
    
    unsigned char* pixels = nullptr;
    int texWidth, texHeight;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &texWidth, &texHeight);

    if(!pixels ||
        !createTextureImageFromData(
            vkDev, textureImage, textureImageMemory, pixels, 
            texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM))
    {
        printf("Failed to load texture\n"); fflush(stdout);
        return false;
    }

    io.Fonts->TexID = (ImTextureID)0;
    io.FontDefault = Font;
    io.DisplayFramebufferScale = ImVec2(1, 1);

    return true;
}

void addImGuiItem(
        uint32_t width, uint32_t height, 
        VkCommandBuffer commandBuffer, const ImDrawCmd* pcmd, 
        ImVec2 clipOff, ImVec2 clipScale, 
        int idxOffset, int vtxOffset
    )
{
    if(pcmd->UserCallback) { return; }

    ImVec4 clipRect;
    clipRect.x = (pcmd->ClipRect.x - clipOff.x) * clipScale.x;
    clipRect.y = (pcmd->ClipRect.y - clipOff.y) * clipScale.y;
    clipRect.z = (pcmd->ClipRect.z - clipOff.x) * clipScale.x;
    clipRect.w = (pcmd->ClipRect.w - clipOff.y) * clipScale.y;

    if( clipRect.x < width && clipRect.y < height &&
        clipRect.z >= 0.0f && clipRect.w >= 0.0f)
    {
        if(clipRect.x < 0.0f) { clipRect.x = 0.0f; }
        if(clipRect.y < 0.0f) { clipRect.y = 0.0f; }

        const VkRect2D scissor =
        {
            .offset =
            {
                .x = (int32_t)(clipRect.x),
                .y = (int32_t)(clipRect.y)
            },
            .extent =
            {
                .width = (uint32_t)(clipRect.z - clipRect.x),
                .height = (uint32_t)(clipRect.w - clipRect.y)
            }
        };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        // TODO: add descriptor indexing here
        vkCmdDraw(commandBuffer, pcmd->ElemCount, 1, pcmd->IdxOffset + idxOffset, pcmd->VtxOffset + vtxOffset);
    }    
}

VulkanImGui::VulkanImGui(VulkanRenderDevice &vkDev) :
    VulkanRendererBase(vkDev, VulkanImage())
{
    ImGuiIO& io = ImGui::GetIO();
    createFontTexture(io, "assets/OpenSans-Light.ttf", vkDev, m_font.image, m_font.imageMemory);
    createImageView(vkDev.device, m_font.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &m_font.imageView);
    createTextureSampler(vkDev.device, &m_fontSampler);

    const size_t imgCount = vkDev.swapchainImages.size();
    m_storageBuffers.resize(imgCount);
    m_storageBuffersMemory.resize(imgCount);
    m_bufferSize = ImGuiVtxBufferSize + ImGuiIdxBufferSize;

    for(size_t i = 0; i < imgCount; i++)
    {
        if(!createBuffer(
                vkDev.device, vkDev.physicalDevice, m_bufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_storageBuffers[i], m_storageBuffersMemory[i])
            )
        {
            printf("VulkanImGui Renderer: createBuffer() failed\n");
            exit(EXIT_FAILURE);
        }
    }

    const std::vector<const char*> shaders =
    {
        "shaders/ImGui.vert",
        "shaders/ImGui.frag"
    };

    if( !createColorAndDepthRenderPass(vkDev, false, &m_renderPass, RenderPassCreateInfo()) ||
        !createColorAndDepthFramebuffers(vkDev, m_renderPass, VK_NULL_HANDLE, m_swapchainFramebuffers) ||
        !createUniformBuffers(vkDev, sizeof(mat4)) ||
        !createDescriptorPool(vkDev, 1, 2, 1, &m_descriptorPool) ||
        !createDescriptorSet(vkDev) ||
        !createPipelineLayout(vkDev.device, m_descriptorSetLayout, &m_pipelineLayout) ||
        // !createPipelineLayoutWithConstants(vkDev.device, m_descriptorSetLayout, &m_pipelineLayout, 0, sizeof(uint32_t)) ||
        !createGraphicsPipeline(vkDev, m_renderPass, m_pipelineLayout, shaders, &m_graphicsPipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true, true, true))
    {
        printf("VulkanImGui: pipeline creation failed\n");
        exit(EXIT_FAILURE);
    }
}

VulkanImGui::~VulkanImGui()
{
    for(size_t i = 0; i < m_swapchainFramebuffers.size(); i++)
    {
        vkDestroyBuffer(*p_dev, m_storageBuffers[i], nullptr);
        vkFreeMemory(*p_dev, m_storageBuffersMemory[i], nullptr);
    }
    vkDestroySampler(*p_dev, m_fontSampler, nullptr);
    destroyVulkanImage(*p_dev, m_font);
}

void VulkanImGui::fillCommandBuffer(const VkCommandBuffer &commandBuffer, size_t currentImage)
{
    beginRenderPass(commandBuffer, currentImage);

    ImVec2 clipOff = drawData->DisplayPos;
    ImVec2 clipScale = drawData->FramebufferScale;

    int vtxOffset = 0;
    int idxOffset = 0;

    for(int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        for(int cmd = 0; cmd < cmdList->CmdBuffer.Size; cmd++)
        {
            const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd];
            addImGuiItem(
                *p_framebufferWidth, *p_framebufferHeight,
                commandBuffer, pcmd,
                clipOff, clipScale,
                idxOffset, vtxOffset
            );
        }
        idxOffset += cmdList->IdxBuffer.Size;
        vtxOffset += cmdList->VtxBuffer.Size;
    }

    vkCmdEndRenderPass(commandBuffer);
}

void VulkanImGui::updateBuffers(VulkanRenderDevice &vkDev, uint32_t currentImage, const ImDrawData *imguiDrawData)
{
    drawData = imguiDrawData;
    const float LEFT = drawData->DisplayPos.x;
    const float RIGHT = drawData->DisplayPos.x + drawData->DisplaySize.x;
    const float TOP = drawData->DisplayPos.y;
    const float BOTTOM = drawData->DisplayPos.y + drawData->DisplaySize.y;

    const mat4 inMtx = glm::ortho(LEFT, RIGHT, TOP, BOTTOM);
    uploadBufferData(vkDev, m_uniformBuffersMemory[currentImage], 0, glm::value_ptr(inMtx), sizeof(mat4));

    void* data = nullptr;
    vkMapMemory(vkDev.device, m_storageBuffersMemory[currentImage], 0, m_bufferSize, 0, &data);
    ImDrawVert* vtx = (ImDrawVert*)data;
    for(int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        memcpy(vtx, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        vtx += cmdList->VtxBuffer.Size;
    }
    uint32_t* idx = (uint32_t*)((uint8_t*)data + ImGuiVtxBufferSize);
    for(int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        const uint16_t* src = (const uint16_t*)cmdList->IdxBuffer.Data;
        for(int j = 0; j < cmdList->IdxBuffer.Size; j++)
        {
            *idx++ = (uint32_t)*src++;
        }
    }
    vkUnmapMemory(vkDev.device, m_storageBuffersMemory[currentImage]);
}

bool VulkanImGui::createDescriptorSet(VulkanRenderDevice &vkDev)
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
        .descriptorSetCount = static_cast<uint32_t>(vkDev.swapchainImages.size()),
        .pSetLayouts = layouts.data()
    };
    m_descriptorSets.resize(vkDev.swapchainImages.size());
    VK_CHECK(vkAllocateDescriptorSets(vkDev.device, &allocInfo, m_descriptorSets.data()));

    for(size_t i = 0; i < vkDev.swapchainImages.size(); i++)
    {
        VkDescriptorSet ds = m_descriptorSets[i];
        const VkDescriptorBufferInfo bufferInfo1 = { m_uniformBuffers[i], 0, sizeof(mat4) };
        const VkDescriptorBufferInfo bufferInfo2 = { m_storageBuffers[i], 0, ImGuiVtxBufferSize };
        const VkDescriptorBufferInfo bufferInfo3 = { m_storageBuffers[i], ImGuiVtxBufferSize, ImGuiIdxBufferSize };
        const VkDescriptorImageInfo imageInfo = { m_fontSampler, m_font.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

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
