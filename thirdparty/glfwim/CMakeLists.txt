add_library(glfwim "input_manager.cpp")

set_target_properties(glfwim PROPERTIES CXX_STANDARD 20)

target_compile_definitions(glfwim PRIVATE INPUT_MANAGER_USE_AS_SINGLETON INPUT_MANAGER_CUSTOM_SINGLETON_NAME=theInputManager)
target_link_libraries(glfwim PRIVATE glfw)
