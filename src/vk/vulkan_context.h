#pragma once

#include "../camera.h"
#include "../model_loader.h"

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

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct VulkanContext
{
	VulkanContext();

	void initWindowVK(GLFWwindow* newWindow);
	void initVK();
	void initGlfwimVK();
	void cleanupVK();
	void drawFrameVK();
	void initCameraVK(Camera* newCamera);

	// Access
	vk::Device GetDevice();

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
	vk::RenderPass renderPass;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;
	std::vector<vk::Framebuffer> swapChainFramebuffers;
	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;
	std::vector<vk::Semaphore> imageAvailableSemaphores, renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences, imagesInFlight;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	vk::Buffer vertexBuffer, indexBuffer;
	uint32_t mipLevels;
	vk::Image textureImage;
	vk::DeviceMemory vertexBufferMemory, indexBufferMemory, textureImageMemory;
	std::vector<vk::Buffer> uniformBuffers;
	std::vector<vk::DeviceMemory> uniformBuffersMemory;
	vk::ImageView textureImageView;
	vk::Sampler textureSampler;
	vk::Image depthImage;
	vk::DeviceMemory depthImageMemory;
	vk::ImageView depthImageView;
	vk::DescriptorPool descriptorPool;
	std::vector<vk::DescriptorSet> descriptorSets;
	vk::SampleCountFlagBits msaaSamples;
	vk::Image colorImage;
	vk::DeviceMemory colorImageMemory;
	vk::ImageView colorImageView;
	size_t currentFrame;
	bool framebufferResized;
	std::unique_ptr<vk::DispatchLoaderDynamic> dispatcher;

	vk::DebugUtilsMessengerEXT debugMessenger;
	bool enableValidationLayers;
	int maxFramesInFlight;

	std::vector<const char*> validationLayers;
	std::vector<const char*> deviceExtensions;

	GLFWwindow* window;
	Camera* camera;

	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport(std::vector<const char*> const& validationLayers);
	bool checkDeviceExtensionSupport(vk::PhysicalDevice const& targetPhysicalDevice, std::vector<const char*> const& requiredDeviceExtensions);
	void pickPhysicalDevice();
	bool isDeviceSuitable(vk::PhysicalDevice const& targetPhysicalDevice);
	QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice const& targetPhysicalDevice);
	void createLogicalDevice();
	SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice const& targetPhysicalDevice);
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes);
	vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities);
	void recreateSwapChain();
	void cleanupSwapChain();
	void createSwapChain();
	vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);
	void createImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createDepthResources();
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory);
	void loadModel();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createColorResources();
	void generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	vk::CommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(vk::CommandBuffer commandBuffer);
	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
	void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
	vk::Format findDepthFormat();
	bool hasStencilComponent(vk::Format format);
	void createCommandBuffers();
	void createSyncObjects();
	vk::ShaderModule createShaderModule(std::vector<char> const& code);
	void updateUniformBuffer(uint32_t currentImage);
	void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);
	vk::SampleCountFlagBits getMaxUsableSampleCount();
	vk::VertexInputBindingDescription getVertexBindingDescription();
	std::array<vk::VertexInputAttributeDescription, 3> getVertexAttributeDescriptions();
};
