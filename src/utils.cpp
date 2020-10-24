#include "utils.h"

std::vector<char> Utils::readBinaryFile(std::string const& fileName)
{
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file");
	}

	auto fileSize = file.tellg();
	std::vector<char> buff(fileSize);

	file.seekg(0);
	file.read(buff.data(), fileSize);
	file.close();

	return buff;
}

void Utils::GLFWwindowDeleter::operator()(GLFWwindow* ptr)
{
	if (ptr) {
		glfwDestroyWindow(ptr);
	}
}
