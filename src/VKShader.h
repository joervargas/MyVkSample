#pragma once

#include <vulkan/vulkan.h>
#include <glslang/Include/glslang_c_shader_types.h>

#include <string>
#include <vector>

struct ShaderModule
{
    std::vector<unsigned int> SPIRV;
    VkShaderModule ShaderModule = nullptr;
};

int endsWith(const char* s, const char* part);

std::string readShaderFile(const char* fileName);

void printShaderSource(const char* text);

glslang_stage_t glslsangShaderStageFromFileName(const char* fileName);

VkShaderStageFlagBits glslangShaderStageToVulkan(glslang_stage_t stage);

VkShaderStageFlagBits getVkShaderStageFromFileName(const char* fileName);

size_t compileShaderFile(const char* file, ShaderModule& shaderModule);

void saveSPIRVBinaryFile(const char* fileName, unsigned int* code, size_t size);

VkResult createShaderModule(VkDevice device, ShaderModule* sm, const char* fileName);

inline VkPipelineShaderStageCreateInfo shaderStageInfo(VkShaderStageFlagBits shaderStage, ShaderModule& module, const char* entryPoint)
{
	return VkPipelineShaderStageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = shaderStage,
		.module = module.ShaderModule,
		.pName = entryPoint,
		.pSpecializationInfo = nullptr
	};
}