#include "opengl_context.h"

#include "gl_wrapper.h"

OpenGlContext::OpenGlContext() :
	useGlDebugCallback{ false }
{
}

inline void OpenGlContext::initWindowGL(GLFWwindow* newWindow)
{
	window = newWindow;
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
}

void OpenGlContext::initGlfwimGL()
{
	// TODO
}

void OpenGlContext::drawFrameGL()
{
	// TODO
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

	auto debugCallback = (GLDEBUGPROC)[](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
		std::string errorWrapper = type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "";
		theLogger.LogError("Gl debug callback: {} type = {:#x}, severity = {:#x}, message = {}", errorWrapper, type, severity, message);
	};

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(debugCallback, 0);
}
