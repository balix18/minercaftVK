#pragma once

#include "camera.h"
#include "utils.h"
#include "vulkan_context.h"

struct App
{
	Utils::WindowSize windowSize;
	std::unique_ptr<GLFWwindow, Utils::GLFWwindowDeleter> window;
	Camera camera;

	App(Utils::WindowSize windowSize);
	~App() = default;

	void run();

private:

	enum class Renderer { VK, GL } renderer;
	bool useGlDebugCallback;
	VulkanContext vkCtx;

	bool IsVulkan();
	bool IsOpenGl();
	void initLogger();
	void initRuncfg();
	void initWindow();
	void initGlfwim();
	void cleanupWindow();
	void mainLoop();

	// Vulkan specific
	void initVK();
	void cleanupVK();
	void drawFrameVK();
	void initCameraVK();

	// OpenGl specific
	void initGlad();
	void initGlDebugCallback();
	void drawFrameGL();
};
