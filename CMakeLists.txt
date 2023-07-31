cmake_minimum_required(VERSION 3.26)

project(MyVkSample CXX C)
set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
if(VULKAN_FOUND)
    message("Vulkan found!")
else()
    message("***Vulkan not found***")
endif()


#output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# find_package(glslang CONFIG REQUIRED)
find_package(Threads REQUIRED)
# find_package(SPIRV-Tools REQUIRED)	

option(BUILD_WITH_EASY_PROFILER "Enable EasyProfiler usage" ON)
option(BUILD_WITH_OPTICK "Enable Optick usage" OFF)

# set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(WIN32)
	set(PYTHON_EXECUTABLE "python")
else()
	set(PYTHON_EXECUTABLE "python3")
endif()

if((NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/.bootstrap.json") OR ("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt" IS_NEWER_THAN "${CMAKE_CURRENT_SOURCE_DIR}/deps/.bootstrap.json"))
	execute_process(
		COMMAND ${PYTHON_EXECUTABLE} "bootstrap.py"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		RESULT_VARIABLE bootstrap_result
	)
	if (NOT (${bootstrap_result} EQUAL 0))
		message(FATAL_ERROR "Cannot run bootstrap.py")
	endif()
endif()

# Set dependecy properties before add dependencies
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")

set(EASY_PROFILER_NO_GUI ON CACHE BOOL "")
set(EASY_PROFILER_NO_SAMPLES ON CACHE BOOL "")

set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_DRACO OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "")

set(MESHOPT_BUILD_DEMO OFF CACHE BOOL "")
set(MESHOPT_BUILD_TOOLS OFF CACHE BOOL "")
set(MESHOPT_BUILD_SHARED_LIBS OFF CACHE BOOL "")

set(ENABLE_GLSLANG_BINARIES  OFF  CACHE BOOL "")
set(ENABLE_HLSL              OFF  CACHE BOOL "")
set(ENABLE_CTEST             OFF  CACHE BOOL "")
set(ENABLE_OPT               OFF  CACHE BOOL "")
set(SKIP_GLSLANG_INSTALL     ON   CACHE BOOL "")

set(DEPS_DIR ./deps/src)
# add_subdirectory(${DEPS_DIR}/etc2comp)
add_subdirectory(${DEPS_DIR}/glfw)
add_subdirectory(${DEPS_DIR}/glm)
add_subdirectory(${DEPS_DIR}/glslang)
add_subdirectory(${DEPS_DIR}/imgui)
# add_subdirectory(${DEPS_DIR}/meshoptimizer)
add_subdirectory(${DEPS_DIR}/stb)
# add_subdirectory(${DEPS_DIR}/taskflow)
add_subdirectory(${DEPS_DIR}/assimp)

if(BUILD_WITH_EASY_PROFILER)
	add_subdirectory(${DEPS_DIR}/easy_profiler)
endif()
if(BUILD_WITH_OPTICK)
	add_subdirectory(${DEPS_DIR}/optick)
endif()


include_directories(${Vulkan_INCLUDE_DIR})
set(DEPS_DIR ./deps/src)
# include_directories(${DEPS_DIR}/etc2comp)
include_directories(${DEPS_DIR}/glfw/include)
include_directories(${DEPS_DIR}/glm)
include_directories(
	${DEPS_DIR}/glslang
	${DEPS_DIR}/glslang/glslang
	${DEPS_DIR}/glslang/glslang/Include
	${DEPS_DIR}/glslang/StandAlone
)
include_directories(${DEPS_DIR}/imgui)
# include_directories(${DEPS_DIR}/meshoptimizer)
include_directories(${DEPS_DIR}/stb/include)
# include_directories(${DEPS_DIR}/taskflow)
include_directories(${DEPS_DIR}/assimp/include)

if(BUILD_WITH_EASY_PROFILER)
	message("Enabled EasyProfiler")
	add_definitions(-DBUILD_WITH_EASY_PROFILER=1)
	include_directories(${DEPS_DIR}/easy_profiler)
	# set_property(TARGET easy_profiler PROPERTY FOLDER "")
endif()
if(BUILD_WITH_OPTICK)
	message("Enabled Optick")
	add_definitions(-DBUILD_WITH_OPTICK=1)
	include_directories(${DEPS_DIR}/optick)
endif()

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

add_executable(${PROJECT_NAME} 
    ${SRC_FILES} 
    # ${HDR_FILES}
)

target_link_libraries(${PROJECT_NAME}
	Vulkan::Vulkan
	SPIRV-Tools
	# glslang::glslang
	# Vulkan::glslang
	# glslang::SPIRV
	# glslang::glslang-default-resource-limits
	glslang
	glslang-default-resource-limits
	GLFW
	glm
	stb_image
	imgui
	assimp
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(${PROJECT_NAME} easy_profiler)
endif() 
if(BUILD_WITH_OPTICK)
	target_link_libraries(${PROJECT_NAME} Optick)
endif()

add_custom_command(
    TARGET ${PROJECT_NAME}
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    VERBATIM
)