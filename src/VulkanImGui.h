#pragma once

#include "VulkanRendererBase.h"
#include <VkUtils.h>

#include <imgui/imgui.h>


class VulkanImGui : public VulkanRendererBase
{
public:

    explicit VulkanImGui(VulkanRenderDevice& vkDev);
    explicit VulkanImGui(VulkanRenderDevice& vkDev, const std::vector<VulkanTexture>& textures);

    virtual ~VulkanImGui();

    virtual void fillCommandBuffer(const VkCommandBuffer& commandBuffer, size_t currentImage) override;

    void updateBuffers(VulkanRenderDevice& vkDev, uint32_t currentImage, const ImDrawData* imguiDrawData);


private:

    const ImDrawData* drawData = nullptr;

    std::vector<VulkanTexture> m_extTextures;

    VkDeviceSize m_bufferSize;
    std::vector<VkBuffer> m_storageBuffers;
    std::vector<VkDeviceMemory> m_storageBuffersMemory;

    VkSampler m_fontSampler;
    VulkanImage m_font;

    bool createDescriptorSet(VulkanRenderDevice& vkDev);

    bool createMultiDescriptorSet(VulkanRenderDevice& vkDev);
};