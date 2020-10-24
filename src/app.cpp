#include "app.h"

#include "gl_wrapper.h"

App::App(Utils::WindowSize windowSize) :
	windowSize{ windowSize },
	useGlDebugCallback{ false }
{
}

void App::run()
{
	initLogger();
	initRuncfg();
	initWindow();
	initGlfwim();
	initCameraVK();
	initVK();
	initGlad();
	initGlDebugCallback();
	mainLoop();
	cleanupVK();
	cleanupWindow();
}

void App::initRuncfg()
{
	std::string projectSourceDir = PROJECT_SOURCE_DIR;
	std::ifstream ifs(projectSourceDir + "runcfg.json");
	rapidjson::IStreamWrapper isw(ifs);

	rapidjson::Document d;
	d.ParseStream(isw);

	std::string currentRenderer = d["currentRenderer"].GetString();
	std::string currentRendererName = d["renderers"][currentRenderer.c_str()]["name"].GetString();

	if (currentRenderer == "vk") renderer = Renderer::VK;
	if (currentRenderer == "gl") renderer = Renderer::GL;

	theLogger.LogInfo("Using {} renderer", currentRendererName);
}

bool App::IsVulkan()
{
	return renderer == Renderer::VK;
}

bool App::IsOpenGl()
{
	return renderer == Renderer::GL;
}

void App::initLogger()
{
	std::string projectSourceDir = PROJECT_SOURCE_DIR;
	std::cerr << theLogger.Init(projectSourceDir + "logger.ini");
	theLogger.SetToString(ShortToString{});

	theLogger.LogInfo("Logger initialized");
}

void App::initWindow()
{
	struct { int major = 0, minor = 0, revision = 0; } glfwVersion;
	glfwGetVersion(&glfwVersion.major, &glfwVersion.minor, &glfwVersion.revision);
	theLogger.LogInfo("GLFW version {}.{}.{}", glfwVersion.major, glfwVersion.minor, glfwVersion.revision);

	if (!glfwInit()) {
		theLogger.LogError("Failed to initialize GLFW");
		exit(EXIT_FAILURE);
	}

	glfwSetErrorCallback([](int errorCode, const char* description) {
		theLogger.LogError("GLFW error code: {}, description: {}", errorCode, description);
	});

	if (IsVulkan()) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	}
	
	if (IsOpenGl()) {
		struct { int major = 4; int minor = 5; } glVersion;
		std::string glslVersion = "#version 450";
		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, glVersion.major);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glVersion.minor);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	}

	auto glfwWindow = glfwCreateWindow(windowSize.width, windowSize.height, "minercaftVK-App", nullptr, nullptr);
	if (!glfwWindow) {
		theLogger.LogError("Failed to open GLFW window");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	window.reset(glfwWindow);

	if (IsVulkan()) {
		vkCtx.initWindowVK(window.get());
	}

	if (IsOpenGl()) {
		glfwMakeContextCurrent(window.get());

		enum struct VSync { DISABLED, ENABLED } vsync = VSync::ENABLED;
		glfwSwapInterval((int)vsync);
	}
}

void App::initGlfwim()
{
	if (IsVulkan()) {
		vkCtx.initGlfwimVK();
	}

	if (IsOpenGl()) {
		// TODO
	}
}

void App::initVK()
{
	if (!IsVulkan()) return;

	vkCtx.initVK();
}

void App::initGlad()
{
	if (!IsOpenGl()) return;

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

void App::initGlDebugCallback()
{
	if (!IsOpenGl() || !useGlDebugCallback) return;

	auto debugCallback = (GLDEBUGPROC)[](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
		std::string errorWrapper = type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "";
		theLogger.LogError("Gl debug callback: {} type = {:#x}, severity = {:#x}, message = {}", errorWrapper, type, severity, message);
	};

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(debugCallback, 0);
}

void App::initCameraVK()
{
	if (!IsVulkan()) return;

	vkCtx.initCameraVK(&camera);
}

void App::mainLoop()
{
	while (!glfwWindowShouldClose(window.get()) && glfwGetKey(window.get(), GLFW_KEY_ESCAPE) != GLFW_PRESS) {
		theInputManager.pollEvents();

		auto currentTime = static_cast<float>(glfwGetTime());
		camera.Update(currentTime);
		camera.Control();

		if (IsVulkan()) {
			drawFrameVK();
		}

		if (IsOpenGl()) {
			drawFrameGL();
		}
	}

	if (IsVulkan()) {
		vkCtx.GetDevice().waitIdle();
	}
}

void App::drawFrameVK()
{
	if (!IsVulkan()) return;

	vkCtx.drawFrameVK();
}

void App::drawFrameGL()
{
	// TODO draw opengl frame
}

void App::cleanupVK()
{
	if (!IsVulkan()) return;

	vkCtx.cleanupVK();
}

void App::cleanupWindow()
{
	window.reset();
	glfwTerminate();
}
