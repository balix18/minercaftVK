#include "vulkan_context.h"

#include "../utils.h"
#include "../runcfg.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	theLogger.LogError("Validation error: {}", pCallbackData->pMessage);

	return VK_FALSE;
}

VulkanContext::VulkanContext() :
	instance{ nullptr },
	physicalDevice{ nullptr },
	device{ nullptr },
	graphicsQueue{ nullptr },
	presentQueue{ nullptr },
	surface{ nullptr },
	swapChain{ nullptr },
	dispatcher{ nullptr },
	framebufferResized{ false },
	msaaSamples{ vk::SampleCountFlagBits::e1 },
	debugMessenger{ nullptr },
	enableValidationLayers{ false }
{
	currentFrame = 0;
	maxFramesInFlight = 2;

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

void VulkanContext::initWindowVK(GLFWwindow* newWindow)
{
	window = newWindow;
}

void VulkanContext::initVK()
{
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createColorResources();
	createDepthResources();
	createFramebuffers();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
	loadModel();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	createSyncObjects();
}

void VulkanContext::initGlfwimVK()
{
	theInputManager.initialize(window);

	theInputManager.registerWindowResizeHandler([&](int width, int height) {
		framebufferResized = true;
	});

	theInputManager.registerUtf8KeyHandler("r", Modifier::None, Action::Press, [&]() {
		theLogger.LogInfo("reset called");
	});
}

void VulkanContext::cleanupVK()
{
	cleanupSwapChain();

	device.destroySampler(textureSampler);
	device.destroyImageView(textureImageView);
	device.destroyImage(textureImage);
	device.freeMemory(textureImageMemory);

	device.destroyDescriptorSetLayout(descriptorSetLayout);

	device.destroyBuffer(indexBuffer);
	device.freeMemory(indexBufferMemory);

	device.destroyBuffer(vertexBuffer);
	device.freeMemory(vertexBufferMemory);

	for (int i = 0; i < maxFramesInFlight; i++) {
		device.destroySemaphore(renderFinishedSemaphores[i]);
		device.destroySemaphore(imageAvailableSemaphores[i]);
		device.destroyFence(inFlightFences[i]);
	}

	device.destroyCommandPool(commandPool);

	device.destroy();

	if (enableValidationLayers) {
		instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr, *dispatcher);
	}
	dispatcher.reset();

	instance.destroySurfaceKHR(surface);
	instance.destroy();
}

void VulkanContext::drawFrameVK()
{
	auto noTimeout = std::numeric_limits<uint64_t>::max();

	auto handleErrorOutOfDateKHR = [](std::exception& e) {
		static auto outOfDateErrorStr = "vk::Queue::presentKHR: ErrorOutOfDateKHR";
		if (std::strcmp(e.what(), outOfDateErrorStr) != 0) {
			throw e;
		}
	};

	device.waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, noTimeout);

	vk::ResultValue<uint32_t> acquireRes{ vk::Result::eIncomplete, 0 };
	try {
		acquireRes = device.acquireNextImageKHR(swapChain, noTimeout, imageAvailableSemaphores[currentFrame], nullptr);
	}
	catch (std::exception& e) {
		handleErrorOutOfDateKHR(e);
	}

	if (acquireRes.result == vk::Result::eErrorOutOfDateKHR || acquireRes.result == vk::Result::eSuboptimalKHR) {
		recreateSwapChain();
		return;
	}
	else if (acquireRes.result != vk::Result::eSuccess) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	uint32_t imageIndex = acquireRes.value;
	if (imagesInFlight[imageIndex]) {
		device.waitForFences(1, &imagesInFlight[imageIndex], VK_TRUE, noTimeout);
	}
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	updateUniformBuffer(imageIndex);

	std::vector<vk::Semaphore> waitSemaphores = { imageAvailableSemaphores[currentFrame] };
	std::vector<vk::Semaphore> signalSemaphores = { renderFinishedSemaphores[currentFrame] };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	vk::SubmitInfo submitInfo{};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	device.resetFences(1, &inFlightFences[currentFrame]);

	graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]);

	std::vector<vk::SwapchainKHR> swapChains = { swapChain };

	vk::PresentInfoKHR presentInfo{};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores.data();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains.data();
	presentInfo.pImageIndices = &imageIndex;

	vk::Result presentRes{ vk::Result::eIncomplete };
	try {
		presentRes = presentQueue.presentKHR(presentInfo);
	}
	catch (std::exception& e) {
		handleErrorOutOfDateKHR(e);
	}

	if (presentRes == vk::Result::eErrorOutOfDateKHR || presentRes == vk::Result::eSuboptimalKHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
	}
	else if (presentRes != vk::Result::eSuccess) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void VulkanContext::initCameraVK(Camera* newCamera)
{
	camera = newCamera;
	auto& cam = *camera;

	cam.Init(true);
	cam.UpdatePosition(glm::vec3(2.0f, 2.0f, 2.0f));
	cam.UpdateDirection(glm::vec3(0.0f, 0.0f, 0.0f) - cam.GetPosition()); // hogy az origoba nezzen
	cam.parameters.clippingDistance = { 0.1f, 10.0f };

	// nem kell parameters.UpdateWindowSize-t hivni kezzel, mert automatikusan meghivodik 
	// a createSwapChain vegen init-kor vagy ha resize callback van
}

vk::Device VulkanContext::GetDevice()
{
	return device;
}

void VulkanContext::createInstance()
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

	std::stringstream ssExt;
	ssExt << fmt::format("Avaliable extensions:\n");
	for (auto const& extensionProp : extensionProps) {
		ssExt << fmt::format("  - {}\n", extensionProp.extensionName);
	}
	theLogger.LogInfo(ssExt.str());

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

void VulkanContext::setupDebugMessenger()
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

void VulkanContext::createSurface()
{
	VkSurfaceKHR vkSurface{};
	auto res = glfwCreateWindowSurface(instance, window, nullptr, &vkSurface);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}

	surface = vkSurface;
}

std::vector<const char*> VulkanContext::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool VulkanContext::checkValidationLayerSupport(std::vector<const char*> const& layers)
{
	auto availableLayers = vk::enumerateInstanceLayerProperties();

	for (auto const& layer : layers) {
		auto resIt = std::find_if(availableLayers.begin(), availableLayers.end(), [&](vk::LayerProperties const& p) {
			return std::strcmp(p.layerName, layer) == 0;
		});
		if (resIt == availableLayers.end()) {
			return false;
		}
	}

	return true;
}

bool VulkanContext::checkDeviceExtensionSupport(vk::PhysicalDevice const& targetPhysicalDevice, std::vector<const char*> const& requiredDeviceExtensions)
{
	auto availableExtensions = targetPhysicalDevice.enumerateDeviceExtensionProperties();

	std::vector<std::string> tmp;
	std::set<const char*> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());
	for (auto const& ext : availableExtensions) {
		tmp.push_back(std::string{ ext.extensionName });
		auto resIt = std::find_if(requiredExtensions.begin(), requiredExtensions.end(), [&](const char* p) {
			return std::strcmp(ext.extensionName, p) == 0;
		});
		if (resIt != requiredExtensions.end()) {
			requiredExtensions.erase(resIt);
		}
	}

	return requiredExtensions.empty();
}

void VulkanContext::pickPhysicalDevice()
{
	auto physicalDevices = instance.enumeratePhysicalDevices();

	for (auto const& currentPhysicalDevice : physicalDevices) {
		if (isDeviceSuitable(currentPhysicalDevice)) {
			physicalDevice = currentPhysicalDevice;
			msaaSamples = getMaxUsableSampleCount();
			break;
		}
	}

	if (!physicalDevice) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	auto deviceName = physicalDevice.getProperties().deviceName;
	theLogger.LogInfo("Using {} as physical device", deviceName);
	theLogger.LogInfo("MSAA samples: {}", msaaSamples);
}

bool VulkanContext::isDeviceSuitable(vk::PhysicalDevice const& targetPhysicalDevice)
{
	auto deviceProps = targetPhysicalDevice.getProperties();
	auto deviceFeatures = targetPhysicalDevice.getFeatures();
	auto familyIndices = findQueueFamilies(targetPhysicalDevice);
	auto extensionsSupported = checkDeviceExtensionSupport(targetPhysicalDevice, deviceExtensions);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		auto swapChainSupport = querySwapChainSupport(targetPhysicalDevice);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return deviceProps.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
		&& familyIndices.isComplete()
		&& extensionsSupported
		&& swapChainAdequate
		&& deviceFeatures.samplerAnisotropy;
}

QueueFamilyIndices VulkanContext::findQueueFamilies(vk::PhysicalDevice const& targetPhysicalDevice)
{
	QueueFamilyIndices familyIndices;

	int idx = 0;
	auto queueFamilies = targetPhysicalDevice.getQueueFamilyProperties();
	for (auto const& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			familyIndices.graphicsFamily = idx;
		}

		auto presentSupport = targetPhysicalDevice.getSurfaceSupportKHR(idx, surface);
		if (presentSupport) {
			familyIndices.presentFamily = idx;
		}

		if (familyIndices.isComplete()) {
			break;
		}

		idx++;
	}

	return familyIndices;
}

void VulkanContext::createLogicalDevice()
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
	deviceFeatures.samplerAnisotropy = VK_TRUE;

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

SwapChainSupportDetails VulkanContext::querySwapChainSupport(vk::PhysicalDevice const& targetPhysicalDevice)
{
	SwapChainSupportDetails details;
	details.capabilities = targetPhysicalDevice.getSurfaceCapabilitiesKHR(surface);
	details.formats = targetPhysicalDevice.getSurfaceFormatsKHR(surface);
	details.presentModes = targetPhysicalDevice.getSurfacePresentModesKHR(surface);

	return details;
}

vk::SurfaceFormatKHR VulkanContext::chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
{
	for (auto const& availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return availableFormat;
		}
	}

	throw std::runtime_error("best surface format unavailable");
}

vk::PresentModeKHR VulkanContext::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes)
{
	for (auto const& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
			return availablePresentMode;
		}
	}

	throw std::runtime_error("best present mode unavailable");
}

vk::Extent2D VulkanContext::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities)
{
	auto specialValue = std::numeric_limits<uint32_t>::max();
	if (capabilities.currentExtent.width != specialValue && capabilities.currentExtent.height != specialValue) {
		return capabilities.currentExtent;
	}

	throw std::runtime_error("best extent unavailable");
}

void VulkanContext::recreateSwapChain()
{
	// alljunk meg a renderelessel amig le van minimalizalva az ablak
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	device.waitIdle();

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createColorResources();
	createDepthResources();
	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
}

void VulkanContext::cleanupSwapChain()
{
	device.destroyImageView(colorImageView);
	device.destroyImage(colorImage);
	device.freeMemory(colorImageMemory);

	device.destroyImageView(depthImageView);
	device.destroyImage(depthImage);
	device.freeMemory(depthImageMemory);

	for (auto const& framebuffer : swapChainFramebuffers) {
		device.destroyFramebuffer(framebuffer);
	}

	device.freeCommandBuffers(commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	device.destroyPipeline(graphicsPipeline);
	device.destroyPipelineLayout(pipelineLayout);
	device.destroyRenderPass(renderPass);

	for (auto const& imageView : swapChainImageViews) {
		device.destroyImageView(imageView);
	}

	device.destroySwapchainKHR(swapChain);

	for (auto i = 0; i < swapChainImages.size(); i++) {
		device.destroyBuffer(uniformBuffers[i]);
		device.freeMemory(uniformBuffersMemory[i]);
	}

	device.destroyDescriptorPool(descriptorPool);
}

void VulkanContext::createSwapChain()
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

	auto& cam = *camera;
	cam.parameters.UpdateWindowSize((float)swapChainExtent.width, (float)swapChainExtent.height);
}

vk::ImageView VulkanContext::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	vk::ImageViewCreateInfo createInfo{};
	createInfo.image = image;
	createInfo.viewType = vk::ImageViewType::e2D;
	createInfo.format = format;
	createInfo.components.r = vk::ComponentSwizzle::eIdentity;
	createInfo.components.g = vk::ComponentSwizzle::eIdentity;
	createInfo.components.b = vk::ComponentSwizzle::eIdentity;
	createInfo.components.a = vk::ComponentSwizzle::eIdentity;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = mipLevels;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	return device.createImageView(createInfo);
}

void VulkanContext::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());
	for (int i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, vk::ImageAspectFlagBits::eColor, 1);
	}
}

void VulkanContext::createRenderPass()
{
	vk::AttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = msaaSamples;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = msaaSamples;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;		// will not be used after drawing has finished
	depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
	depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = swapChainImageFormat;
	colorAttachmentResolve.samples = vk::SampleCountFlagBits::e1;
	colorAttachmentResolve.loadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachmentResolve.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachmentResolve.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachmentResolve.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachmentResolve.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachmentResolve.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference	colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;	// attachmentDescription index
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass{};
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	vk::SubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.srcAccessMask = {};
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

	std::array<vk::AttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

	vk::RenderPassCreateInfo renderPassInfo{};
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	renderPass = device.createRenderPass(renderPassInfo);
}

void VulkanContext::createDescriptorSetLayout()
{
	vk::DescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	descriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
}

void VulkanContext::createGraphicsPipeline()
{
	auto vertShaderCode = Utils::ReadBinaryFile((theRuncfg.shadersDir / "vert.spv").string());
	auto fragShaderCode = Utils::ReadBinaryFile((theRuncfg.shadersDir / "frag.spv").string());

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

	auto bindingDesc = Vertex::getBindingDescription();
	auto attributeDesc = Vertex::getAttributeDescriptions();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDesc.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDesc.data();

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	vk::Viewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
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
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.depthBiasEnable = VK_FALSE;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = msaaSamples;

	vk::PipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = vk::CompareOp::eLess;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

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
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;	// subpass index

	graphicsPipeline = static_cast<vk::Pipeline&>(device.createGraphicsPipeline(nullptr, pipelineInfo));

	device.destroyShaderModule(vertShaderModule);
	device.destroyShaderModule(fragShaderModule);
}

void VulkanContext::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (int i = 0; i < swapChainImageViews.size(); i++) {
		std::array<vk::ImageView, 3> attachments{
			colorImageView,
			depthImageView,
			swapChainImageViews[i]
		};

		vk::FramebufferCreateInfo framebufferInfo{};
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
	}
}

void VulkanContext::createCommandPool()
{
	auto queueFamilies = findQueueFamilies(physicalDevice);

	vk::CommandPoolCreateInfo poolInfo{};
	poolInfo.queueFamilyIndex = queueFamilies.graphicsFamily.value();
	poolInfo.flags = {};

	commandPool = device.createCommandPool(poolInfo);
}

void VulkanContext::createDepthResources()
{
	vk::Format depthFormat = findDepthFormat();

	createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory);
	depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);

	transitionImageLayout(depthImage, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
}

void VulkanContext::createTextureImage()
{
	auto fileName = (theRuncfg.texturesDir / "viking_room.png").string();

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(fileName.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	vk::DeviceSize imageSize = texWidth * texHeight * sizeof(uint32_t);
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	// staging buffer
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingBufferMemory;
	auto stagingUsage = vk::BufferUsageFlagBits::eTransferSrc;
	auto stagingMemoryProps = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	createBuffer(imageSize, stagingUsage, stagingMemoryProps, stagingBuffer, stagingBufferMemory);

	// map staging buffer to cpu, and fill it
	auto dataPtr = device.mapMemory(stagingBufferMemory, 0, imageSize);
	std::memcpy(dataPtr, pixels, static_cast<size_t>(imageSize));
	device.unmapMemory(stagingBufferMemory);

	// free the stb image
	stbi_image_free(pixels);

	auto textureUsage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	auto textureMemoryProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
	createImage(texWidth, texHeight, mipLevels, vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, textureUsage, textureMemoryProps, textureImage, textureImageMemory);

	transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels);
	copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);

	device.destroyBuffer(stagingBuffer);
	device.freeMemory(stagingBufferMemory);

	generateMipmaps(textureImage, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, mipLevels);
}

void VulkanContext::createTextureImageView()
{
	textureImageView = createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mipLevels);
}

void VulkanContext::createTextureSampler()
{
	vk::SamplerCreateInfo samplerInfo{};
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16.0f;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.minLod = 0;
	samplerInfo.maxLod = static_cast<float>(mipLevels);
	samplerInfo.mipLodBias = 0.0f;

	textureSampler = device.createSampler(samplerInfo);
}

void VulkanContext::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
{
	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	buffer = device.createBuffer(bufferInfo);

	vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(buffer);
	auto memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	bufferMemory = device.allocateMemory(allocInfo);
	device.bindBufferMemory(buffer, bufferMemory, 0);
}

void VulkanContext::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory)
{
	vk::ImageCreateInfo imageInfo{};
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = usage;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;
	imageInfo.samples = numSamples;

	image = device.createImage(imageInfo);

	auto memRequirements = device.getImageMemoryRequirements(image);
	auto memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	imageMemory = device.allocateMemory(allocInfo);
	device.bindImageMemory(image, imageMemory, 0);
}

void VulkanContext::loadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	auto modelFileName = (theRuncfg.texturesDir / "viking_room.obj").string();

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelFileName.c_str())) {
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32_t, Vertex::Hasher> uniqueVertices;

	for (auto const& shape : shapes) {
		for (auto const& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.find(vertex) == uniqueVertices.end()) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void VulkanContext::createVertexBuffer()
{
	vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	// staging buffer
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingBufferMemory;
	auto stagingUsage = vk::BufferUsageFlagBits::eTransferSrc;
	auto stagingMemoryProps = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	createBuffer(bufferSize, stagingUsage, stagingMemoryProps, stagingBuffer, stagingBufferMemory);

	// map staging buffer to cpu, and fill it
	auto dataPtr = device.mapMemory(stagingBufferMemory, 0, bufferSize);
	std::memcpy(dataPtr, vertices.data(), static_cast<size_t>(bufferSize));
	device.unmapMemory(stagingBufferMemory);

	// vertex buffer
	auto bufferUsage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;
	auto bufferMemoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
	createBuffer(bufferSize, bufferUsage, bufferMemoryProperties, vertexBuffer, vertexBufferMemory);

	// copy the vertex data into the vertex buffer
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	// cleanup
	device.destroyBuffer(stagingBuffer);
	device.freeMemory(stagingBufferMemory);
}

void VulkanContext::createIndexBuffer()
{
	vk::DeviceSize bufferSize = sizeof(uint32_t) * indices.size();

	// staging buffer
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingBufferMemory;
	auto stagingUsage = vk::BufferUsageFlagBits::eTransferSrc;
	auto stagingMemoryProps = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	createBuffer(bufferSize, stagingUsage, stagingMemoryProps, stagingBuffer, stagingBufferMemory);

	// map staging buffer to cpu, and fill it
	auto dataPtr = device.mapMemory(stagingBufferMemory, 0, bufferSize);
	std::memcpy(dataPtr, indices.data(), static_cast<size_t>(bufferSize));
	device.unmapMemory(stagingBufferMemory);

	// index buffer
	auto bufferUsage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer;
	auto bufferMemoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
	createBuffer(bufferSize, bufferUsage, bufferMemoryProperties, indexBuffer, indexBufferMemory);

	// copy the index data into the index buffer
	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	// cleanup
	device.destroyBuffer(stagingBuffer);
	device.freeMemory(stagingBufferMemory);
}

void VulkanContext::createUniformBuffers()
{
	vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

	uniformBuffers.resize(swapChainImages.size());
	uniformBuffersMemory.resize(swapChainImages.size());

	for (auto i = 0; i < swapChainImages.size(); i++) {
		auto usage = vk::BufferUsageFlagBits::eUniformBuffer;
		auto memoryProps = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		createBuffer(bufferSize, usage, memoryProps, uniformBuffers[i], uniformBuffersMemory[i]);
	}
}

void VulkanContext::createDescriptorPool()
{
	vk::DescriptorPoolSize uniformBufferPool{};
	uniformBufferPool.type = vk::DescriptorType::eUniformBuffer;
	uniformBufferPool.descriptorCount = static_cast<uint32_t>(swapChainImages.size());

	vk::DescriptorPoolSize combinedImageSampler{};
	combinedImageSampler.type = vk::DescriptorType::eCombinedImageSampler;
	combinedImageSampler.descriptorCount = static_cast<uint32_t>(swapChainImages.size());

	std::array<vk::DescriptorPoolSize, 2> poolSizes{ uniformBufferPool, combinedImageSampler };

	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

	descriptorPool = device.createDescriptorPool(poolInfo);
}

void VulkanContext::createDescriptorSets()
{
	std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets = device.allocateDescriptorSets(allocInfo);

	for (auto i = 0; i < swapChainImages.size(); i++) {
		vk::DescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		vk::DescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		vk::WriteDescriptorSet uniformDescriptorWrite{};
		uniformDescriptorWrite.dstSet = descriptorSets[i];
		uniformDescriptorWrite.dstBinding = 0;
		uniformDescriptorWrite.dstArrayElement = 0;
		uniformDescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		uniformDescriptorWrite.descriptorCount = 1;
		uniformDescriptorWrite.pBufferInfo = &bufferInfo;

		vk::WriteDescriptorSet samplerDescriptorWrite{};
		samplerDescriptorWrite.dstSet = descriptorSets[i];
		samplerDescriptorWrite.dstBinding = 1;
		samplerDescriptorWrite.dstArrayElement = 0;
		samplerDescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		samplerDescriptorWrite.descriptorCount = 1;
		samplerDescriptorWrite.pImageInfo = &imageInfo;

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites{ uniformDescriptorWrite, samplerDescriptorWrite };

		device.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void VulkanContext::createColorResources()
{
	vk::Format colorFormat = swapChainImageFormat;

	auto usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment;
	auto memoryProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
	createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, vk::ImageTiling::eOptimal, usage, memoryProps, colorImage, colorImageMemory);
	colorImageView = createImageView(colorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
}

void VulkanContext::generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	// Check if image format supports linear blitting
	vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(imageFormat);
	if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::ImageMemoryBarrier barrier{};
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	// loop start at i = 1
	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {},
			0, nullptr,
			0, nullptr,
			1, &barrier);

		vk::ImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		commandBuffer.blitImage(
			image, vk::ImageLayout::eTransferSrcOptimal,
			image, vk::ImageLayout::eTransferDstOptimal,
			1, &blit,
			vk::Filter::eLinear);

		barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {},
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {},
		0, nullptr,
		0, nullptr,
		1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

vk::CommandBuffer VulkanContext::beginSingleTimeCommands()
{
	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	std::vector<vk::CommandBuffer> allocCommandBuffers = device.allocateCommandBuffers(allocInfo);
	vk::CommandBuffer commandBuffer = allocCommandBuffers[0];

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	commandBuffer.begin(beginInfo);

	return commandBuffer;
}

void VulkanContext::endSingleTimeCommands(vk::CommandBuffer commandBuffer)
{
	commandBuffer.end();

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	graphicsQueue.submit(1, &submitInfo, nullptr);
	graphicsQueue.waitIdle();

	device.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

void VulkanContext::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
	auto commandBuffer = beginSingleTimeCommands();

	vk::BufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

void VulkanContext::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
{
	auto commandBuffer = beginSingleTimeCommands();

	vk::BufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
	vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

vk::Format VulkanContext::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
	for (vk::Format format : candidates) {
		auto formatProps = physicalDevice.getFormatProperties(format);

		// chech format based on tiling
		if (tiling == vk::ImageTiling::eLinear && (formatProps.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && (formatProps.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

vk::Format VulkanContext::findDepthFormat()
{
	std::vector<vk::Format> candidates = { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint };
	return findSupportedFormat(candidates, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

bool VulkanContext::hasStencilComponent(vk::Format format)
{
	return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

void VulkanContext::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());

	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.commandPool = commandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	commandBuffers = device.allocateCommandBuffers(allocInfo);

	for (int i = 0; i < commandBuffers.size(); i++) {
		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = {};

		std::array<vk::ClearValue, 2> clearValues;
		clearValues[0].color = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		vk::RenderPassBeginInfo renderPassInfo{};
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		commandBuffers[i].begin(beginInfo);
		commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

		vk::Buffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		commandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers, offsets);
		commandBuffers[i].bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
		commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

		commandBuffers[i].drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		commandBuffers[i].endRenderPass();
		commandBuffers[i].end();
	}
}

void VulkanContext::createSyncObjects()
{
	imageAvailableSemaphores.resize(maxFramesInFlight);
	renderFinishedSemaphores.resize(maxFramesInFlight);
	inFlightFences.resize(maxFramesInFlight);

	vk::SemaphoreCreateInfo semaphoreInfo{};

	vk::FenceCreateInfo fenceInfo{};
	fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	for (int i = 0; i < maxFramesInFlight; i++) {
		imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
		renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
		inFlightFences[i] = device.createFence(fenceInfo);
	}

	imagesInFlight.resize(swapChainImages.size());
}

vk::ShaderModule VulkanContext::createShaderModule(std::vector<char> const& code)
{
	vk::ShaderModuleCreateInfo createInfo{};
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	return device.createShaderModule(createInfo);
}

void VulkanContext::updateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	// create the mvp matrices
	UniformBufferObject ubo{};
	glm::mat4 identity{ 1.0f };
	ubo.model = glm::rotate(identity, time * glm::radians(22.5f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.view = camera->V();
	ubo.proj = camera->P();

	// map uniform buffer to cpu, and fill it
	auto dataPtr = device.mapMemory(uniformBuffersMemory[currentImage], 0, sizeof(ubo));
	std::memcpy(dataPtr, &ubo, sizeof(ubo));
	device.unmapMemory(uniformBuffersMemory[currentImage]);
}

void VulkanContext::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels)
{
	auto commandBuffer = beginSingleTimeCommands();

	vk::ImageMemoryBarrier barrier{};
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	}

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	commandBuffer.pipelineBarrier(
		sourceStage, destinationStage, {},
		0, nullptr,		// memory barrier
		0, nullptr,		// buffer memory barrier
		1, &barrier		// image memory barrier
	);

	endSingleTimeCommands(commandBuffer);
}

vk::SampleCountFlagBits VulkanContext::getMaxUsableSampleCount()
{
	auto physicalDeviceProperties = physicalDevice.getProperties();

	vk::SampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

	// Select the highest available sample count
	if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
	if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
	if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
	if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
	if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
	if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

	return vk::SampleCountFlagBits::e1;
}

bool QueueFamilyIndices::isComplete()
{
	return graphicsFamily.has_value()
		&& presentFamily.has_value();
}

vk::VertexInputBindingDescription Vertex::getBindingDescription()
{
	vk::VertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = vk::VertexInputRate::eVertex;

	return bindingDescription;
}

std::array<vk::VertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions()
{
	std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};

	auto& posDesc = attributeDescriptions[0];
	posDesc.binding = 0;
	posDesc.location = 0;
	posDesc.format = vk::Format::eR32G32B32Sfloat;
	posDesc.offset = offsetof(Vertex, pos);

	auto& colorDesc = attributeDescriptions[1];
	colorDesc.binding = 0;
	colorDesc.location = 1;
	colorDesc.format = vk::Format::eR32G32B32Sfloat;
	colorDesc.offset = offsetof(Vertex, color);

	auto& textCoordDesc = attributeDescriptions[2];
	textCoordDesc.binding = 0;
	textCoordDesc.location = 2;
	textCoordDesc.format = vk::Format::eR32G32Sfloat;
	textCoordDesc.offset = offsetof(Vertex, texCoord);

	return attributeDescriptions;
}

bool Vertex::operator==(const Vertex& other) const
{
	return pos == other.pos
		&& color == other.color
		&& texCoord == other.texCoord;
}

std::size_t Vertex::Hasher::operator()(Vertex const& vertex) const noexcept
{
	std::size_t seed = 0;
	boost::hash_combine(seed, std::hash<glm::vec3>()(vertex.pos));
	boost::hash_combine(seed, std::hash<glm::vec3>()(vertex.color));
	boost::hash_combine(seed, std::hash<glm::vec2>()(vertex.texCoord));
	return seed;
}
