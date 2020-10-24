#pragma once

#include "../camera.h"

struct OpenGlContext
{
	OpenGlContext();

	void initWindowGL(GLFWwindow* newWindow);
	void initCameraGL(Camera* newCamera);
	void initGL();
	void initGlfwimGL();
	void drawFrameGL();
	void cleanupGL();

private:
	bool useGlDebugCallback;

	GLFWwindow* window;
	Camera* camera;

	void initGlad();
	void initGlDebugCallback();
};