#pragma once

#include "camera.h"
#include "utils.h"
#include "vk/vulkan_context.h"
#include "gl/opengl_context.h"

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
	
	VulkanContext vkCtx;
	OpenGlContext glCtx;

	bool IsVulkan();
	bool IsOpenGl();
	void initLogger();
	void initRuncfg();
	void initWindow();
	void initGlfwim();
	void initCamera();
	void initContext();
	void cleanupWindow();
	void animate(float currentTime);
	void drawFrame();
	void mainLoop();
	void cleanup();
};
