#pragma once

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete();
};

struct SwapChainSupportDetails
{
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

struct App
{
	int width, height;

	struct GLFWwindowDeleter
	{
		void operator()(GLFWwindow* ptr);
	};

	std::unique_ptr<GLFWwindow, GLFWwindowDeleter> window;

	App(int width, int height);
	~App() = default;

	void run();

private:
	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	vk::SurfaceKHR surface;
	vk::SwapchainKHR swapChain;
	std::vector<vk::Image> swapChainImages;
	vk::Format swapChainImageFormat;
	vk::Extent2D swapChainExtent;
	std::vector<vk::ImageView> swapChainImageViews;
	vk::PipelineLayout pipelineLayout;
	std::unique_ptr<vk::DispatchLoaderDynamic> dispatcher;

	vk::DebugUtilsMessengerEXT debugMessenger;
	bool enableValidationLayers;

	std::vector<const char*> validationLayers;
	std::vector<const char*> deviceExtensions;

	void initWindow();
	void initVK();
	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport(std::vector<const char*> const& validationLayers);
	bool checkDeviceExtensionSupport(vk::PhysicalDevice const& device, std::vector<const char*> const& deviceExtensions);
	void pickPhysicalDevice();
	bool isDeviceSuitable(vk::PhysicalDevice const& device);
	QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice const& device);
	void createLogicalDevice();
	SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice const& device);
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes);
	vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities);
	void createSwapChain();
	void createImageViews();
	void createGraphicsPipeline();
	vk::ShaderModule createShaderModule(std::vector<char> const& code);
	void mainLoop();
	void cleanup();
};
