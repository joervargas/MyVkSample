

//////////////////////////////////////////////////////////

#ifndef INSTANCE_VK_FUNC
    #define INSTANCE_VK_FUNC(Function)
#endif



#undef INSTANCE_VK_FUNC

/////////////////////////////////////////////////////////

#ifndef INSTANCE_VK_EXT_FUNC
    #define INSTANCE_VK_EXT_FUNC(Function, Extension)
#endif


INSTANCE_VK_EXT_FUNC( vkGetPhysicalDeviceFeatures2KHR, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME )
INSTANCE_VK_EXT_FUNC( vkGetPhysicalDeviceProperties2KHR, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME )
INSTANCE_VK_EXT_FUNC( vkGetPhysicalDeviceMemoryProperties2KHR, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME )
INSTANCE_VK_EXT_FUNC( vkGetPhysicalDeviceFormatProperties2KHR, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME )


#undef INSTANCE_VK_EXT_FUNC

/////////////////////////////////////////////////////////

#ifndef DEVICE_VK_FUNC
    #define DEVICE_VK_FUNC(Function)
#endif



#undef DEVICE_VK_FUNC

///////////////////////////////////////////////////////////

#ifndef DEVICE_VK_EXT_FUNC
    #define DEVICE_VK_EXT_FUNC(Function, Extension)
#endif


// DEVICE_VK_EXT_FUNC( vkCmdPushDescriptorSetKHR, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME )
// DEVICE_VK_EXT_FUNC( vkCmdPushDescriptorSetWithTemplateKHR, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME )


#undef DEVICE_VK_EXT_FUNC