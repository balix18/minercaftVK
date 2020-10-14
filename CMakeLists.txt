cmake_minimum_required(VERSION 3.17)
project(minercaftVK-App)

add_executable(${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}/")

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W4     # turn on level 4 warnings
        /wd4201 # nameless struct/union
        /wd4324 # structure was padded due to __declspec(align())
        /wd4100 # unreferenced formal parameter
        /wd4458 # hides class member
        /std:c++latest
    )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

add_subdirectory(thirdparty/EZPZLogger)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Stb REQUIRED)
find_package(Boost REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${Boost_INCLUDE_DIRS}
    thirdparty
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glm
    fmt::fmt
    tinyobjloader::tinyobjloader
    Vulkan::Vulkan
    ${Boost_LIBRARIES}
    EZPZLogger
)

target_precompile_headers(${PROJECT_NAME} PRIVATE "src/pch.h")

target_sources(${PROJECT_NAME} PRIVATE
    "src/pch.cpp"
    "src/main.cpp"
    "src/app.cpp"
    "src/utils.cpp"
)
