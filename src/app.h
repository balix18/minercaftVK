#pragma once

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;

	bool isComplete();
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
	void run();

private:
	vk::Instance instance;
	std::unique_ptr<vk::DispatchLoaderDynamic> dispatcher;
	vk::DebugUtilsMessengerEXT debugMessenger;
	bool enableValidationLayers;

	void initWindow();
	void initVK();
	void createInstance();
	void setupDebugMessenger();
	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport(std::vector<std::string> const& validationLayers);
	void pickPhysicalDevice();
	bool isDeviceSuitable(vk::PhysicalDevice const& device);
	QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice const& device);
	void mainLoop();
	void cleanup();
};
