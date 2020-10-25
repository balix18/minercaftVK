#pragma once

struct GpuProgram;

struct UniformTracker
{
	static std::unordered_map<int, std::unordered_map<std::string, bool>> trackedMap;

	static bool Contains(int gpuProgramId, std::string const& uniformName);
	static void Track(GpuProgram const& gpuProgram, std::string const& uniformName, std::string const& uniformType);
};

struct GlWrapper
{
	static void SetUniform(glm::mat4 const& subject, std::string const& uniformName, GpuProgram const& gpuProgram);
	static void SetUniform(glm::mat3 const& subject, std::string const& uniformName, GpuProgram const& gpuProgram);
	static void SetUniform(glm::vec3 const& subject, std::string const& uniformName, GpuProgram const& gpuProgram);
	static void SetUniform(std::vector<glm::mat4> const& subject, std::string const& uniformName, GpuProgram const& gpuProgram);
	static void SetUniform(float subject, std::string const& uniformName, GpuProgram const& gpuProgram);
	static void SetUniform(int subject, std::string const& uniformName, GpuProgram const& gpuProgram);

	// Avoid explicit conversion
	template<typename T> static void SetUniform(T const& subject, std::string const& uniformName, GpuProgram const& gpuProgram) = delete;

	static std::string GetString(GLenum const name);
	static GLint GetIntegerv(GLenum const parameterName);
	static GLint GetProgramiv(GLuint const programHandle, GLenum const parameterName);
};

