set(Target shared_utils)

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false 
	src/*.cpp 
	src/*.c 
	src/vk_exts/*.cpp  
	src/vk_exts/*.c
)
file(GLOB_RECURSE HDR_FILES LIST_DIRECTORIES false 
	src/*.h 
	src/*.hpp 
	src/vk_exts/*.h
	src/vk_exts/*.hpp
)

add_library(${Target} ${SRC_FILES} ${HDR_FILES})

target_include_directories(${Target}
	PUBLIC
		./src/
)

target_link_libraries(${Target}
	Vulkan::Vulkan
	glslang
	SPIRV
	glslang-default-resource-limits
	glm
	stb_image
	assimp
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(${Target} easy_profiler)
endif() 
if(BUILD_WITH_OPTICK)
	target_link_libraries(${Target} Optick)
endif()