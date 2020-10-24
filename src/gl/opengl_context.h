#pragma once

#include "../camera.h"
#include "../utils.h"

struct OpenGlContext
{
	OpenGlContext();

	void initWindowGL(GLFWwindow* newWindow, Utils::WindowSize newWindowSize);
	void initCameraGL(Camera* newCamera);
	void initGL();
	void initGlfwimGL();
	void drawFrameGL();
	void cleanupGL();

private:
	bool useGlDebugCallback;

	GLFWwindow* window;
	Camera* camera;
	Utils::WindowSize windowSize;

	void initGlad();
	void initGlDebugCallback();
};