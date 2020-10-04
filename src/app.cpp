#include "app.h"

#include "utils.h"

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
	instance{ nullptr },
	physicalDevice{ nullptr },
	device{ nullptr },
	graphicsQueue{ nullptr },
	presentQueue{ nullptr },
	surface{ nullptr },
	swapChain{ nullptr },
	dispatcher{ nullptr },
	debugMessenger{ nullptr },
	enableValidationLayers{ false }
{
	validationLayers = {
		"VK_LAYER_KHRONOS_validation",
	};

	deviceExtensions = {
		"VK_KHR_swapchain"
	};

#ifdef _DEBUG
	enableValidationLayers = true;
#endif
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
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
}

void App::createInstance()
{
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

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	instance = vk::createInstance(createInfo);

	dispatcher = std::make_unique<vk::DispatchLoaderDynamic>(instance, vkGetInstanceProcAddr);

	auto extensionProps = vk::enumerateInstanceExtensionProperties();

	fmt::print("avaliable extensions:\n");
	for (auto const& extensionProp : extensionProps) {
		fmt::print("  - {}\n", extensionProp.extensionName);
	}

	// check if all required extensions are available
	for (auto const& ext : reqExtensions) {
		auto resIt = std::find_if(extensionProps.begin(), extensionProps.end(), [&](vk::ExtensionProperties const& p) { 
			return !!std::strcmp(p.extensionName, ext); 
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

void App::createSurface()
{
	VkSurfaceKHR vkSurface{};
	auto res = glfwCreateWindowSurface(instance, window.get(), nullptr, &vkSurface);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}

	surface = vkSurface;
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

bool App::checkValidationLayerSupport(std::vector<const char*> const& validationLayers)
{
	auto availableLayers = vk::enumerateInstanceLayerProperties();

	for (auto const& layer : validationLayers) {
		auto resIt = std::find_if(availableLayers.begin(), availableLayers.end(), [&](vk::LayerProperties const& p) {
			return !!std::strcmp(p.layerName, layer);
		});
		if (resIt == availableLayers.end()) {
			return false;
		}
	}

	return true;
}

bool App::checkDeviceExtensionSupport(vk::PhysicalDevice const& device, std::vector<const char*> const& deviceExtensions)
{
	auto availableExtensions = device.enumerateDeviceExtensionProperties();

	std::vector<std::string> tmp;
	std::set<const char*> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (auto const& ext : availableExtensions) {
		tmp.push_back(std::string{ ext.extensionName });
		auto resIt = std::find_if(requiredExtensions.begin(), requiredExtensions.end(), [&](const char* p) {
			return !!std::strcmp(ext.extensionName, p);
		});
		if (resIt != requiredExtensions.end()) {
			requiredExtensions.erase(resIt);
		}
	}

	return requiredExtensions.empty();
}

void App::pickPhysicalDevice()
{
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
	auto extensionsSupported = checkDeviceExtensionSupport(device, deviceExtensions);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		auto swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return deviceProps.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
		&& deviceFeatures.geometryShader
		&& indices.isComplete()
		&& extensionsSupported
		&& swapChainAdequate;
}

QueueFamilyIndices App::findQueueFamilies(vk::PhysicalDevice const& device)
{
	QueueFamilyIndices indices;

	int idx = 0;
	auto queueFamilies = device.getQueueFamilyProperties();
	for (auto const& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = idx;
		}

		auto presentSupport = device.getSurfaceSupportKHR(idx, surface);
		if (presentSupport) {
			indices.presentFamily = idx;
		}

		if (indices.isComplete()) {
			break;
		}

		idx++;
	}

	return indices;
}

void App::createLogicalDevice()
{
	auto familyIndices = findQueueFamilies(physicalDevice);

	if (familyIndices.graphicsFamily != familyIndices.presentFamily) {
		std::runtime_error("multiple queues needed, this is unoptimal");
	}

	vk::DeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.queueFamilyIndex = familyIndices.graphicsFamily.value();
	queueCreateInfo.queueCount = 1;
	auto queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	vk::PhysicalDeviceFeatures deviceFeatures{};

	vk::DeviceCreateInfo createInfo{};
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	device = physicalDevice.createDevice(createInfo);

	auto queueIndex = 0;
	graphicsQueue = device.getQueue(familyIndices.graphicsFamily.value(), queueIndex);
	presentQueue = device.getQueue(familyIndices.presentFamily.value(), queueIndex);
}

SwapChainSupportDetails App::querySwapChainSupport(vk::PhysicalDevice const& device)
{
	SwapChainSupportDetails details;
	details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
	details.formats = device.getSurfaceFormatsKHR(surface);
	details.presentModes = device.getSurfacePresentModesKHR(surface);

	return details;
}

vk::SurfaceFormatKHR App::chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
{
	for (auto const& availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return availableFormat;
		}
	}

	throw std::runtime_error("best surface format unavailable");
}

vk::PresentModeKHR App::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes)
{
	for (auto const& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
			return availablePresentMode;
		}
	}

	throw std::runtime_error("best present mode unavailable");
}

vk::Extent2D App::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities)
{
	auto specialValue = std::numeric_limits<uint32_t>::max();
	if (capabilities.currentExtent.width != specialValue && capabilities.currentExtent.height != specialValue) {
		return capabilities.currentExtent;
	}

	throw std::runtime_error("best extent unavailable");
}

void App::createSwapChain()
{
	auto swapChainSupport = querySwapChainSupport(physicalDevice);
	auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	auto extent = chooseSwapExtent(swapChainSupport.capabilities);

	auto const minImageCount = swapChainSupport.capabilities.minImageCount;
	auto const maxImageCount = swapChainSupport.capabilities.maxImageCount;

	auto imageCount = minImageCount + 1;
	if (maxImageCount > 0 && imageCount > maxImageCount) {
		imageCount = maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo{};
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

	auto queueFamilies = findQueueFamilies(physicalDevice);
	if (queueFamilies.graphicsFamily != queueFamilies.presentFamily) {
		throw std::runtime_error("needs concurrent sharing mode");
	}

	createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = nullptr;

	swapChain = device.createSwapchainKHR(createInfo);

	swapChainImages = device.getSwapchainImagesKHR(swapChain);
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void App::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());
	for (int i = 0; i < swapChainImages.size(); i++) {
		vk::ImageViewCreateInfo createInfo{};
		createInfo.image = swapChainImages[i];
		createInfo.viewType = vk::ImageViewType::e2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components.r = vk::ComponentSwizzle::eIdentity;
		createInfo.components.g = vk::ComponentSwizzle::eIdentity;
		createInfo.components.b = vk::ComponentSwizzle::eIdentity;
		createInfo.components.a = vk::ComponentSwizzle::eIdentity;
		createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		auto imageView = device.createImageView(createInfo);
		swapChainImageViews[i] = imageView;
	}
}

void App::createRenderPass()
{
	vk::AttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference	colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;	// attachmentDescription index
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass{};
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	vk::RenderPassCreateInfo renderPassInfo{};
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	renderPass = device.createRenderPass(renderPassInfo);
}

void App::createGraphicsPipeline()
{
	std::string projectPath = PROJECT_SOURCE_DIR;
	auto vertShaderCode = Utils::readBinaryFile(projectPath + "shaders/vert.spv");
	auto fragShaderCode = Utils::readBinaryFile(projectPath + "shaders/frag.spv");

	auto vertShaderModule = createShaderModule(vertShaderCode);
	auto fragShaderModule = createShaderModule(fragShaderCode);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {
		vertShaderStageInfo,
		fragShaderStageInfo
	};

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	vk::Viewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	rasterizer.frontFace = vk::FrontFace::eClockwise;
	rasterizer.depthBiasEnable = VK_FALSE;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

	device.destroyShaderModule(vertShaderModule);
	device.destroyShaderModule(fragShaderModule);
}

vk::ShaderModule App::createShaderModule(std::vector<char> const& code)
{
	vk::ShaderModuleCreateInfo createInfo{};
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	return device.createShaderModule(createInfo);
}

void App::mainLoop()
{
	while (!glfwWindowShouldClose(window.get())) {
		glfwPollEvents();
	}
}

void App::cleanup()
{
	device.destroyPipelineLayout(pipelineLayout);
	device.destroyRenderPass(renderPass);

	for (auto const& imageView : swapChainImageViews) {
		device.destroyImageView(imageView);
	}

	device.destroySwapchainKHR(swapChain);
	device.destroy();

	if (enableValidationLayers) {
		instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr, *dispatcher);
	}
	dispatcher.reset();

	instance.destroySurfaceKHR(surface);
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
	return graphicsFamily.has_value()
		&& presentFamily.has_value();
}
