#pragma once

#include "vk_functions.h"

#include <vector>
#include <cstdlib>

namespace vk_ext
{
    static bool load_vk_instance_functions(VkInstance& Instance, std::vector<const char *> enabled_extensions)
    {
        #define INSTANCE_VK_FUNC(name) \
            name = (PFN_##name)vkGetInstanceProcAddr(Instance, #name); \
            if(name == nullptr) \
            { \
                printf("\nCould not load vulkan instance function named %s:\n\n", #name); fflush(stdout); \
                return false; \
            } \

        #define INSTANCE_VK_EXT_FUNC(name, extension) \
            for( auto& enabled_extension : enabled_extensions) \
            { \
                if(std::string(enabled_extension) == std::string(extension)) \
                { \
                    name = (PFN_##name)vkGetInstanceProcAddr(Instance, #name); \
                } \
                if(&name == nullptr) \
                { \
                    printf("\nCould not load vulkan instance extension function named %s:\n\n", #name); fflush(stdout); \
                    return false; \
                } \
            } \

        #include "vk_functions.inl"
        return true;
    }

    static bool load_vk_device_functions(VkDevice& Device, std::vector<const char*> enabled_extensions)
    {
        #define DEVICE_VK_FUNC(name) \
            name = (PFN_##name)vkGetDeviceProcAddr(Device, #name); \
            if(name == nullptr) \
            { \
                printf("\nCould not load vk device function named: %s\n\n", #name); \
                retur false; \
            } \

        #define DEVICE_VK_EXT_FUNC(name, extension) \
            for(auto& enabled_extension : enabled_extensions) \
            { \
                if(std::string(enabled_extension) == std::string(extension)) \
                { \
                    name = (PFN_##name)vkGetDeviceProcAddr(Device, #name); \
                } \
                if(&name == nullptr) \
                { \
                    printf("\nCould not load vk device extension function named: %s\n\n", #name); \
                    return false; \
                } \
            } \
        
        #include "vk_functions.inl"
        return true;
    }
    
} // namespace vk_exts
