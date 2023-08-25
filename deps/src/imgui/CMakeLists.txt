
add_library(imgui
    src/imgui.cpp
    src/imgui_draw.cpp
    src/imgui_widgets.cpp
    src/imgui_tables.cpp
    src/imgui_impl_glfw.cpp
    src/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC include/ PRIVATE include/imgui/)

target_link_libraries(imgui Vulkan::Vulkan glfw)