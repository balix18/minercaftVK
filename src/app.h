#pragma once

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

	void initWindow();
	void initVK();
	void createInstance();
	void mainLoop();
	void cleanup();
};
