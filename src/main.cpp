#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <fmt/format.h>

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto* window = glfwCreateWindow(800, 600, "minercaftVK-App", nullptr, nullptr);

	const char* layerName = nullptr;
	uint32_t extensionCount = 0;
	vk::enumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr);
	
	fmt::print("{} extensions supported\n", extensionCount);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}
