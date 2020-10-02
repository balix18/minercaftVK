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
	createInstance();
}

void App::createInstance()
{
	vk::ApplicationInfo appInfo{};
	appInfo.pApplicationName = "minercaftVK";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	vk::InstanceCreateInfo createInfo{};
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<std::string> glfwInstanceExtensions;
	for (int i = 0; i < glfwExtensionCount; i++) {
		glfwInstanceExtensions.emplace_back(glfwExtensions[i]);
	}

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;
	createInfo.enabledLayerCount = 0;

	vk::Result result = vk::createInstance(&createInfo, nullptr, &instance);
	if (result != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create instance!");
	}

	const char* layerName = nullptr;
	uint32_t extensionCount = 0;
	vk::enumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr);

	std::vector<vk::ExtensionProperties> extensions(extensionCount);
	vk::enumerateInstanceExtensionProperties(layerName, &extensionCount, extensions.data());

	fmt::print("avaliable extensions:");
	for (auto const& ext : extensions) {
		fmt::print("  - {}\n", ext.extensionName);
	}

	for (auto const& glfwExt : glfwInstanceExtensions) {
		auto resIt = std::find_if(extensions.begin(), extensions.end(), [&](vk::ExtensionProperties const& p) { 
			return p.extensionName == glfwExt; 
		});
		if (resIt == extensions.end()) {
			throw std::runtime_error(fmt::format("glfw required extension missing: ", glfwExt));
		}
	}
}

void App::mainLoop()
{
	while (!glfwWindowShouldClose(window.get())) {
		glfwPollEvents();
	}
}

void App::cleanup()
{
	instance.destroy();
	window.reset();
	glfwTerminate();
}

void App::GLFWwindowDeleter::operator()(GLFWwindow* ptr)
{
	if (ptr) {
		glfwDestroyWindow(ptr);
	}
}
