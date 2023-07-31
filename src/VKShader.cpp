#include "VKShader.h"

#include <glslang/Public/resource_limits_c.h>
#include <glslang/Include/glslang_c_interface.h>

#include <glslang/Public/ShaderLang.h>

#include <assert.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <malloc.h>



int endsWith(const char *s, const char *part)
{
    return (strstr(s, part) - s) == (strlen(s) - strlen(part));
}

std::string readShaderFile(const char *fileName)
{
    FILE* file = fopen(fileName, "r");
    if(!file)
    {
        printf("I/O error. Cannot open '%s'\n", fileName);
        return std::string();
    }

    fseek(file, 0L, SEEK_END);
    const auto bytesinfile = ftell(file);
    fseek(file, 0L, SEEK_SET);

    char* buffer = (char*)alloca(bytesinfile + 1);
    const size_t bytesread = fread(buffer, 1, bytesinfile, file);
    fclose(file);

    buffer[bytesread] = 0;
    static constexpr unsigned char BOM[] = { 0xEF, 0xBB, 0xBF };
    if(bytesread > 3)
    {
        if(!memcmp(buffer, BOM, 3))
        {
            memset(buffer, ' ', 3);
        }
    }

    std::string code(buffer);

    while (code.find("#include ") != code.npos)
    {
        const auto pos = code.find("#include ");
        const auto p1 = code.find('<', pos);
        const auto p2 = code.find('>', pos);
        if(p1 == code.npos || p2 == code.npos || p2 <= p1)
        {
            printf("Error while loading shader program: %s\n", code.c_str());
            return std::string();
        }
        const std::string name = code.substr(p1 + 1, p2 - p1 - 1);
        const std::string include = readShaderFile(name.c_str());
        code.replace(pos, p2-pos+1, include.c_str());
    }

    return code;
}

void printShaderSource(const char *text)
{
    int line = 1;
    printf("\n(%3i) ", line);
    while(text && *text++)
    {
        if(*text == '\n')
        {
            printf("\n(%3i) ", ++line);
        }
        else if (*text == '\r')
        {}
        else
        {
            printf("%c", *text);
        }
        printf("\n");
    }
}

void saveSPIRVBinaryFile(const char *fileName, unsigned int *code, size_t size)
{
    FILE* file = fopen(fileName, "w");
    if(!file) return;

    fwrite(code, sizeof(uint32_t), size, file);
    fclose(file);
}

glslang_stage_t glslsangShaderStageFromFileName(const char* fileName)
{
    if(endsWith(fileName, ".vert")) { return GLSLANG_STAGE_VERTEX; }
    if(endsWith(fileName, ".frag")) { return GLSLANG_STAGE_FRAGMENT; }
    if(endsWith(fileName, ".geom")) { return GLSLANG_STAGE_GEOMETRY; }
    if(endsWith(fileName, ".comp")) { return GLSLANG_STAGE_COMPUTE; }
    if(endsWith(fileName, ".tesc")) { return GLSLANG_STAGE_TESSCONTROL; }
    if(endsWith(fileName, ".tese")) { return GLSLANG_STAGE_TESSEVALUATION; }

    return GLSLANG_STAGE_VERTEX;
}

VkShaderStageFlagBits glslangShaderStageToVulkan(glslang_stage_t stage)
{
    switch(stage)
    {
        case GLSLANG_STAGE_VERTEX:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case GLSLANG_STAGE_FRAGMENT:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case GLSLANG_STAGE_GEOMETRY:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case GLSLANG_STAGE_TESSCONTROL:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case GLSLANG_STAGE_TESSEVALUATION:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case GLSLANG_STAGE_COMPUTE:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            return VK_SHADER_STAGE_VERTEX_BIT;
    }
}

VkShaderStageFlagBits getVkShaderStageFromFileName(const char *fileName)
{
    return glslangShaderStageToVulkan(glslsangShaderStageFromFileName(fileName));
}

size_t compileShader(glslang_stage_t stage, const char* shaderSource, VulkanShaderModule& shaderModule)
{
    const glslang_input_t input =
    {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_2,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_3,
        .code = shaderSource,
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslang_default_resource(),
    };

    glslang_shader_t* shader = glslang_shader_create(&input);

    if(!glslang_shader_preprocess(shader, &input))
    {
        fprintf(stderr, "GLSL preprocessing failed\n");
        fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
        fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
        printShaderSource(input.code);
        return 0;
    }

    if(!glslang_shader_parse(shader, &input))
    {
        fprintf(stderr, "GLSL parsing failed\n");
        fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
        fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
        printShaderSource(glslang_shader_get_preprocessed_code(shader));
        return 0;
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if(!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
    {
        fprintf(stderr, "GLSL linking failed\n");
        fprintf(stderr, "\n%s", glslang_program_get_info_log(program));
        fprintf(stderr, "\n%s", glslang_program_get_info_debug_log(program));
        return 0;
    }

    glslang_program_SPIRV_generate(program, stage);

    shaderModule.SPIRV.resize(glslang_program_SPIRV_get_size(program));
    glslang_program_SPIRV_get(program, shaderModule.SPIRV.data());

    const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
    if(spirv_messages) { fprintf(stderr, "%s", spirv_messages); }

    glslang_program_delete(program);
    glslang_shader_delete(shader);

    return shaderModule.SPIRV.size();
}

size_t compileShaderFile(const char *file, VulkanShaderModule &shaderModule)
{
    std::string shaderSource = readShaderFile(file);
    if(!shaderSource.empty())
    {
        return compileShader(glslsangShaderStageFromFileName(file), shaderSource.c_str(), shaderModule);
    }
    return 0;
}

VkResult createShaderModule(VkDevice device, VulkanShaderModule *sm, const char *fileName)
{
    if(!compileShaderFile(fileName, *sm)) { return VK_NOT_READY; }

    const VkShaderModuleCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sm->SPIRV.size() * sizeof(unsigned int),
        .pCode = sm->SPIRV.data()
    };

    return vkCreateShaderModule(device, &createInfo, nullptr, &sm->ShaderModule);
}
