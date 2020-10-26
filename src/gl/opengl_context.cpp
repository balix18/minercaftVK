#include "opengl_context.h"

#include "gl_wrapper.h"

OpenGlContext::OpenGlContext() :
	useGlDebugCallback{ true }
{
}

void OpenGlContext::initWindowGL(GLFWwindow* newWindow, Utils::WindowSize newWindowSize)
{
	window = newWindow;
	windowSize = newWindowSize;
}

void OpenGlContext::initCameraGL(Camera* newCamera)
{
	camera = newCamera;
	auto& cam = *camera;

	cam.Init(true);
	cam.UpdatePosition(glm::vec3(2.0f, 2.0f, 2.0f));
	cam.UpdateDirection(glm::vec3(0.0f, 0.0f, 0.0f) - cam.GetPosition()); // hogy az origoba nezzen
	cam.parameters.clippingDistance = { 0.1f, 10.0f };

	// TODO valszeg kell a parameters.UpdateWindowSize-t hivni kezzel a gl-ben
}

void OpenGlContext::initGL()
{
	initGlad();
	initGlDebugCallback();

	simpleShader = std::make_unique<SimpleShader>();

	simpleScene.Create(windowSize);
}

void OpenGlContext::initGlfwimGL()
{
	theInputManager.initialize(window);
}

void OpenGlContext::drawFrameGL()
{
	glViewport(0, 0, windowSize.width, windowSize.height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_BACK);	// ez a default, de azert inkabb itt hagyom hogy egyertelmu legyen

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	for (auto const& object3d : simpleScene.drawableObjects) {
		object3d.Draw(*simpleShader, *camera);
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	glfwSwapBuffers(window);
}

void OpenGlContext::cleanupGL()
{
	// egyelore semmit nem kell kitakaritani
}

void OpenGlContext::initGlad()
{
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		theLogger.LogError("gladLoadGLLoader failed!");
		exit(EXIT_FAILURE);
	}

	theLogger.LogInfo("GL Vendor    : {}", GlWrapper::GetString(GL_VENDOR));
	theLogger.LogInfo("GL Renderer  : {}", GlWrapper::GetString(GL_RENDERER));
	theLogger.LogInfo("GL Version (string)     : {}", GlWrapper::GetString(GL_VERSION));
	theLogger.LogInfo("GL Version (integer)    : {}.{}", GlWrapper::GetIntegerv(GL_MAJOR_VERSION), GlWrapper::GetIntegerv(GL_MINOR_VERSION));
	theLogger.LogInfo("GLSL Version : {}", GlWrapper::GetString(GL_SHADING_LANGUAGE_VERSION));
}

void OpenGlContext::initGlDebugCallback()
{
	if (!useGlDebugCallback) return;

	auto debugCallback = (GLDEBUGPROC)[](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	{
		static std::unordered_set<uint> dontCareIds = { 0x20071 };
		if (dontCareIds.find(id) != dontCareIds.end()) return;

		auto rType = GlWrapper::ResolveDebugType(type);
		auto rSeverity = GlWrapper::ResolveDebugSeverity(severity);
		auto rSource = GlWrapper::ResolveDebugSource(source);

		theLogger.LogError("Debug message callback:\n  - id: {:#x}\n  - type: {}\n  - severity: {}\n  - souce = {}\n  - message = {}", id, rType, rSeverity, rSource, message);
	};

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(debugCallback, 0);
}
