#include "app.h"

App::App(int width, int height) : 
	width(width),
	height(height)
{
}

void App::run()
{
	initWindow();
	initVK();
	mainLoop();
	cleanup();
}

void App::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window.reset(glfwCreateWindow(width, height, "minercaftVK-App", nullptr, nullptr));
}

void App::initVK()
{
	const char* layerName = nullptr;
	uint32_t extensionCount = 0;
	vk::enumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr);

	fmt::print("{} extensions supported\n", extensionCount);
}

void App::mainLoop()
{
	while (!glfwWindowShouldClose(window.get())) {
		glfwPollEvents();
	}
}

void App::cleanup()
{
	window.reset();
	glfwTerminate();
}

void App::GLFWwindowDeleter::operator()(GLFWwindow* ptr)
{
	if (ptr) {
		glfwDestroyWindow(ptr);
	}
}
