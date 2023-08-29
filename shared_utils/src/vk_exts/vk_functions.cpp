#include "vk_functions.h"


namespace vk_ext
{
    #define INSTANCE_VK_FUNC(name) PFN_##name name;

    #define INSTANCE_VK_EXT_FUNC(name, extension) PFN_##name name;

    #define DEVICE_VK_FUNC(name) PFN_##name name;

    #define DEVICE_VK_EXT_FUNC(name, extension) PFN_##name name;

    #include "vk_functions.inl"
}