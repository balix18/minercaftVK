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
#ifdef _DEBUG
	bool enableValidationLayers = true;
#else
	bool enableValidationLayers = false;
#endif

	std::vector<std::string> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	if (enableValidationLayers && !checkValidationLayerSupport(validationLayers)) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

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

	auto ConvertToCStrVector = [](std::vector<std::string> const& input)
	{
		std::vector<const char*> output(input.size());
		std::transform(input.begin(), input.end(), output.begin(), [](std::string const& p) {
			return p.c_str();
		});
		return output;
	};

	auto rawValidationLayers = ConvertToCStrVector(validationLayers);
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(rawValidationLayers.size());
		createInfo.ppEnabledLayerNames = rawValidationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	vk::Result result = vk::createInstance(&createInfo, nullptr, &instance);
	if (result != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create instance!");
	}

	vk::Optional<const std::string> layerName(nullptr);
	auto extensions = vk::enumerateInstanceExtensionProperties(layerName);

	fmt::print("avaliable extensions:\n");
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

bool App::checkValidationLayerSupport(std::vector<std::string> const& validationLayers)
{
	auto availableLayers = vk::enumerateInstanceLayerProperties();

	for (auto const& layer : validationLayers) {
		auto resIt = std::find_if(availableLayers.begin(), availableLayers.end(), [&](vk::LayerProperties const& p) {
			return p.layerName == layer;
		});
		if (resIt == availableLayers.end()) {
			return false;
		}
	}

	return true;
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
