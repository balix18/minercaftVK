cmake_minimum_required(VERSION 3.17)
project(minercaftVK-App)

add_executable(${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W4     # turn on level 4 warnings
        /wd4201 # nameless struct/union
        /wd4324 # structure was padded due to __declspec(align())
        /wd4100 # unreferenced formal parameter
        /wd4458 # hides class member
        /wd5054 # operator '|': deprecated between enumerations of different types
        /std:c++latest
    )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Stb REQUIRED)
find_package(Boost REQUIRED)

add_subdirectory(thirdparty/EZPZLogger)
add_subdirectory(thirdparty/glfwim)
add_subdirectory(thirdparty/glad)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${Boost_INCLUDE_DIRS}
    ${RAPIDJSON_INCLUDE_DIRS}
    thirdparty
    thirdparty/Glad/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glm
    fmt::fmt
    tinyobjloader::tinyobjloader
    Vulkan::Vulkan
    ${Boost_LIBRARIES}
    EZPZLogger
    glfwim
    glad
)

target_precompile_headers(${PROJECT_NAME} PRIVATE "src/pch.h")

target_sources(${PROJECT_NAME} PRIVATE
    "src/pch.cpp"
    "src/main.cpp"
    "src/app.cpp"
    "src/camera.cpp"
    "src/image_cache.cpp"
    "src/model_loader.cpp"
    "src/utils.cpp"
    "src/runcfg.cpp"
    "src/vk/vulkan_context.cpp"
    "src/gl/gl_object_3d.cpp"
    "src/gl/gl_gpu_program.cpp"
    "src/gl/gl_simple_shader.cpp"
    "src/gl/gl_wrapper.cpp"
    "src/gl/opengl_context.cpp"
    "src/gl/render_state.cpp"
    "src/gl/simple_scene.cpp"
    "src/gl/surface_texture.cpp")
