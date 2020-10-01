cmake_minimum_required(VERSION 3.17)
project(minercaftVK-App)

add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glm
    fmt::fmt
    Vulkan::Vulkan
)

target_precompile_headers(${PROJECT_NAME} PRIVATE "src/pch.h")

target_sources(${PROJECT_NAME} PRIVATE 
    "src/main.cpp"
    "src/app.cpp"
)
