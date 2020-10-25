#pragma once

struct Utils
{
	static std::vector<char> ReadBinaryFile(std::string const& fileName);
	static std::string ReadTextFile(std::string const& fileName);

	struct GLFWwindowDeleter
	{
		void operator()(GLFWwindow* ptr);
	};

	struct WindowSize
	{
		int width, height;
	};
};
