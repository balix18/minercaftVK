#include "utils.h"

std::vector<char> Utils::ReadBinaryFile(std::string const& fileName)
{
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error(fmt::format("Failed to open file: {}", fileName));
	}

	auto fileSize = file.tellg();
	std::vector<char> buff(fileSize);

	file.seekg(0);
	file.read(buff.data(), fileSize);
	file.close();

	return buff;
}

std::string Utils::ReadTextFile(std::string const& fileName)
{
	std::ifstream ifs(fileName, std::ios::in);

	if (!ifs.is_open()) {
		throw std::runtime_error(fmt::format("Failed to open file: {}", fileName));
	}

	std::stringstream buffer;
	buffer << ifs.rdbuf();

	// kiolvassa a file teljes tartalmát
	return buffer.str();
}

void Utils::GLFWwindowDeleter::operator()(GLFWwindow* ptr)
{
	if (ptr) {
		glfwDestroyWindow(ptr);
	}
}
