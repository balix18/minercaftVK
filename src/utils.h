#pragma once

struct Utils
{
	static std::vector<char> readBinaryFile(std::string const& fileName);

	struct GLFWwindowDeleter
	{
		void operator()(GLFWwindow* ptr);
	};

	struct WindowSize
	{
		int width, height;
	};
};
