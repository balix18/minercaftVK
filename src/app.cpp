#include "app.h"

#include "runcfg.h"

App::App(Utils::WindowSize windowSize) :
	windowSize{ windowSize }
{
}

void App::run()
{
	initLogger();
	initRuncfg();
	initWindow();
	initGlfwim();
	initCamera();
	initContext();
	mainLoop();
	cleanup();
	cleanupWindow();
}

void App::initRuncfg()
{
	theRuncfg.Init();

	if (theRuncfg.currentRenderer == "vk") renderer = Renderer::VK;
	if (theRuncfg.currentRenderer == "gl") renderer = Renderer::GL;

	theLogger.LogInfo("Using {} renderer", theRuncfg.currentRendererName);
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
	auto loggerIniPath = theRuncfg.projectSourceDir / "logger.ini";
	std::cerr << theLogger.Init(loggerIniPath.string());
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

	auto title = fmt::format("minercaftVK-App (renderer: {})", theRuncfg.currentRenderer);

	auto glfwWindow = glfwCreateWindow(windowSize.width, windowSize.height, title.c_str(), nullptr, nullptr);
	if (!glfwWindow) {
		theLogger.LogError("Failed to open GLFW window");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	window.reset(glfwWindow);

	if (IsOpenGl()) {
		glfwMakeContextCurrent(window.get());

		enum struct VSync { DISABLED, ENABLED } vsync = VSync::ENABLED;
		glfwSwapInterval((int)vsync);
	}

	if (IsVulkan()) {
		vkCtx.initWindowVK(window.get());
	}

	if (IsOpenGl()) {
		glCtx.initWindowGL(window.get(), windowSize);
	}
}

void App::initGlfwim()
{
	if (IsVulkan()) {
		vkCtx.initGlfwimVK();
	}

	if (IsOpenGl()) {
		glCtx.initGlfwimGL();
	}
}

void App::initContext()
{
	if (IsVulkan()) {
		vkCtx.initVK();
	}

	if (IsOpenGl()) {
		glCtx.initGL();
	}
}

void App::initCamera()
{
	if (IsVulkan()) {
		vkCtx.initCameraVK(&camera);
	}

	if (IsOpenGl()) {
		glCtx.initCameraGL(&camera);
	}
}

void App::mainLoop()
{
	while (!glfwWindowShouldClose(window.get()) && glfwGetKey(window.get(), GLFW_KEY_ESCAPE) != GLFW_PRESS) {
		theInputManager.pollEvents();

		auto currentTime = static_cast<float>(glfwGetTime());
		camera.Update(currentTime);
		camera.Control();

		drawFrame();
	}

	if (IsVulkan()) {
		vkCtx.GetDevice().waitIdle();
	}
}

void App::drawFrame()
{
	if (IsVulkan()) {
		vkCtx.drawFrameVK();
	}

	if (IsOpenGl()) {
		glCtx.drawFrameGL();
	}
}

void App::cleanup()
{
	if (IsVulkan()) {
		vkCtx.cleanupVK();
	}

	if (IsOpenGl()) {
		glCtx.cleanupGL();
	}
}

void App::cleanupWindow()
{
	window.reset();
	glfwTerminate();
}
