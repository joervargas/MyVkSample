cmake_minimum_required(VERSION 3.22)

project(meshconvert)

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false 
	src/*.cpp 
	src/*.c 
)

file(GLOB_RECURSE HDR_FILES LIST_DIRECTORIES false 
	src/*.h 
	src/*.hpp 
)

add_executable( ${PROJECT_NAME} ${SRC_FILES})

target_link_libraries( ${PROJECT_NAME} 
    assimp 
    meshoptimizer
    shared_utils
)