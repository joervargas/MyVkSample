#include "VulkanCanvas.h"

#include <stdio.h>
#include <array>

using glm::vec3;
using glm::vec4;
using glm::mat4;



VulkanCanvas::VulkanCanvas(VulkanRenderDevice &vkDev, VulkanImage depth) :
    VulkanRendererBase(vkDev, depth)
{
    const size_t imgCount = vkDev.swapchainImages.size();
    m_storageBuffer.resize(imgCount);
    m_storageBufferMemory.resize(imgCount);

    for(size_t i = 0; i < imgCount; i++)
    {
        if(!createBuffer(
                vkDev.device, vkDev.physicalDevice, 
                kMaxLinesDataSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_storageBuffer[i], m_storageBufferMemory[i]
            ))
        {
            printf("VulkanCanvas: createBuffer() failed\n");
            exit(EXIT_FAILURE);
        }
    }
    
    std::vector<const char*> shaders = 
    {
        "shaders/Lines.vert",
        "shaders/Lines.frag"
    };

    // pipeline creation code skipped here
    if (!createColorAndDepthRenderPass(vkDev, (depth.image != VK_NULL_HANDLE), &m_renderPass, RenderPassCreateInfo()) ||
        !createUniformBuffers(vkDev, sizeof(UniformBuffer)) ||
        !createColorAndDepthFramebuffers(vkDev, m_renderPass, depth.imageView, m_swapchainFramebuffers) ||
        !createDescriptorPool(vkDev, 1, 1, 0, &m_descriptorPool) ||
        !createDescriptorSet(vkDev) ||
        !createPipelineLayout(vkDev.device, m_descriptorSetLayout, &m_pipelineLayout) ||
        !createGraphicsPipeline(vkDev, m_renderPass, m_pipelineLayout, shaders, &m_graphicsPipeline, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, (depth.image != VK_NULL_HANDLE), true))
    {
        printf("VulkanCanvas: failed to create pipeline\n");
        exit(EXIT_FAILURE);
    }
}

VulkanCanvas::~VulkanCanvas()
{
    for(size_t i = 0; i < m_swapchainFramebuffers.size(); i++)
    {
        vkDestroyBuffer(*p_dev, m_storageBuffer[i], nullptr);
        vkFreeMemory(*p_dev, m_storageBufferMemory[i], nullptr);
    }
}

void VulkanCanvas::fillCommandBuffer(const VkCommandBuffer &commandBuffer, size_t currentImage)
{
    if(m_lines.empty()) { return; }

    beginRenderPass(commandBuffer, currentImage);
    vkCmdDraw(commandBuffer, m_lines.size(), 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
}

void VulkanCanvas::updateBuffer(VulkanRenderDevice &vkDev, size_t currentImage)
{
    if(m_lines.empty()) { return; }

    VkDeviceSize bufferSize = m_lines.size() * sizeof(VertexData);
    uploadBufferData(vkDev, m_storageBufferMemory[currentImage], 0, m_lines.data(), bufferSize);
}

void VulkanCanvas::updateUniformBuffer(VulkanRenderDevice &vkDev, const glm::mat4 &mvp, float time, uint32_t currentImage)
{
    const UniformBuffer ubo =
    {
        .mvp = mvp,
        .time = time
    };
    uploadBufferData(vkDev, m_uniformBuffersMemory[currentImage], 0, &ubo, sizeof(ubo));
}

void VulkanCanvas::clear()
{
    m_lines.clear();
}

void VulkanCanvas::line(const vec3 &p1, const vec3 &p2, const vec4 &color)
{
    m_lines.push_back({ .position = p1, .color = color });
    m_lines.push_back({ .position = p2, .color = color });
}

void VulkanCanvas::plane3d(const vec3 &origin, const vec3 &v1, const vec3 &v2, int n1, int n2, float s1, float s2, const vec4 &color, const vec4 &outlineColor)
{
    // Draw four lines representing a plane segment
    // TODO: Figure out this formula
    line(
        origin - s1 / 2.0f * v1 - s2 / 2.0f * v2, 
        origin - s1 / 2.0f * v1 + s2 / 2.0f * v2,
        outlineColor
    );
    line(
        origin + s1 / 2.0f * v1 - s2 / 2.0f * v2,
        origin + s1 / 2.0f * v1 + s2 / 2.0f * v2,
        outlineColor
    );
    line(
        origin - s1 / 2.0f * v1 + s2 / 2.0f * v2,
        origin + s1 / 2.0f * v1 + s2 / 2.0f * v2,
        outlineColor
    );
    line(
        origin - s1 / 2.0f * v1 - s2 / 2.0f * v2,
        origin + s1 / 2.0f * v1 - s2 / 2.0f * v2,
        outlineColor
    );

    // Draw n1 horizontal lines and n2 vertical lines
    for(int ii = 1; ii < n1; ii++)
    {
        const float t = ((float)ii - (float)n1 / 2.0f) * s1 / (float)n1;
        const vec3 o1 = origin + t * v1;
        line(o1 - s2 / 2.0f * v2, o1 + s2 / 2.0f * v2, color);
    }
    for(int ii = 1; ii < n2; ii++)
    {
        const float t = ((float)ii - (float)n2 / 2.0f) * s2 / (float)n2;
        const vec3 o2 = origin + t * v2;
        line(o2 - s1 / 2.0f * v1, o2 + s1 / 2.0f * v1, color);
    }
}

bool VulkanCanvas::createDescriptorSet(VulkanRenderDevice &vkDev)
{
    const std::array <VkDescriptorSetLayoutBinding, 2> bindings =
    {
        descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
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

        const VkDescriptorBufferInfo bufferInfo1 = { m_uniformBuffers[i], 0, sizeof(UniformBuffer) };
        const VkDescriptorBufferInfo bufferInfo2 = { m_storageBuffer[i], 0, kMaxLinesDataSize };

        const std::array<VkWriteDescriptorSet, 2> descriptorWrites =
        {
            bufferWriteDescriptorSet(ds, &bufferInfo1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
            bufferWriteDescriptorSet(ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        };

        vkUpdateDescriptorSets(vkDev.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    return true;
}
