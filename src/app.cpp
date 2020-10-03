#include "app.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	fmt::print(std::cerr, "validation layer: {}\n", pCallbackData->pMessage);

	return VK_FALSE;
}

App::App(int width, int height) : 
	width{ width },
	height{ height },
	enableValidationLayers{ false }
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
	setupDebugMessenger();
	pickPhysicalDevice();
}

void App::createInstance()
{
#ifdef _DEBUG
	enableValidationLayers = true;
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

	auto reqExtensions = getRequiredExtensions();

	vk::InstanceCreateInfo createInfo{};
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(reqExtensions.size());
	createInfo.ppEnabledExtensionNames = reqExtensions.data();

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

	dispatcher = std::make_unique<vk::DispatchLoaderDynamic>(instance, vkGetInstanceProcAddr);

	vk::Optional<const std::string> layerName(nullptr);
	auto extensionProps = vk::enumerateInstanceExtensionProperties(layerName);

	fmt::print("avaliable extensions:\n");
	for (auto const& extensionProp : extensionProps) {
		fmt::print("  - {}\n", extensionProp.extensionName);
	}

	// check if all required extensions are available
	for (auto const& ext : reqExtensions) {
		auto resIt = std::find_if(extensionProps.begin(), extensionProps.end(), [&](vk::ExtensionProperties const& p) { 
			return std::strcmp(p.extensionName, ext) == 0; 
		});
		if (resIt == extensionProps.end()) {
			throw std::runtime_error(fmt::format("required extension missing: ", ext));
		}
	}
}

void App::setupDebugMessenger()
{
	if (!enableValidationLayers) {
		return;
	}

	vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
	createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
	createInfo.pfnUserCallback = vkDebugCallback;
	createInfo.pUserData = nullptr;

	auto res = instance.createDebugUtilsMessengerEXT(&createInfo, nullptr, &debugMessenger, *dispatcher);
	if (res != vk::Result::eSuccess) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

std::vector<const char*> App::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
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

void App::pickPhysicalDevice()
{
	vk::PhysicalDevice physicalDevice = nullptr;
	auto physicalDevices = instance.enumeratePhysicalDevices();

	for (auto const& device : physicalDevices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}

	if (!physicalDevice) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	auto deviceName = physicalDevice.getProperties().deviceName;
	fmt::print("Using {} as physical device\n", deviceName);
}

bool App::isDeviceSuitable(vk::PhysicalDevice const& device)
{
	auto deviceProps = device.getProperties();
	auto deviceFeatures = device.getFeatures();
	auto indices = findQueueFamilies(device);

	return deviceProps.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
		&& deviceFeatures.geometryShader
		&& indices.isComplete();
}

QueueFamilyIndices App::findQueueFamilies(vk::PhysicalDevice const& device)
{
	QueueFamilyIndices indices;

	int count = 0;
	auto queueFamilies = device.getQueueFamilyProperties();
	for (auto const& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = count;
			break;
		}
		count++;
	}

	return indices;
}

void App::mainLoop()
{
	while (!glfwWindowShouldClose(window.get())) {
		glfwPollEvents();
	}
}

void App::cleanup()
{
	if (enableValidationLayers) {
		instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr, *dispatcher);
	}

	dispatcher.reset();
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

bool QueueFamilyIndices::isComplete()
{
	return graphicsFamily.has_value();
}
