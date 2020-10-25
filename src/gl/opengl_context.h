#pragma once

#include "../camera.h"
#include "../utils.h"
#include "gl_simple_shader.h"
#include "gl_object_3d.h"
#include "simple_scene.h"

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
	std::unique_ptr<SimpleShader> simpleShader;

	SimpleScene simpleScene;

	void initGlad();
	void initGlDebugCallback();
};