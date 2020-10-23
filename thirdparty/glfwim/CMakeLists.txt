set(libName glfwim)

add_library(${libName} "input_manager.cpp")

set_target_properties(${libName} PROPERTIES CXX_STANDARD 20)

target_compile_definitions(${libName} PRIVATE INPUT_MANAGER_USE_AS_SINGLETON INPUT_MANAGER_CUSTOM_SINGLETON_NAME=theInputManager)
target_link_libraries(${libName} PRIVATE glfw)
