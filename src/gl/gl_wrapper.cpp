#include "gl_wrapper.h"

#include "gl_gpu_program.h"

std::unordered_map<int, std::unordered_map<std::string, bool>> UniformTracker::trackedMap = std::unordered_map<int, std::unordered_map<std::string, bool>>();

bool UniformTracker::Contains(int gpuProgramId, std::string const& uniformName)
{
	auto gpuProgramIt = trackedMap.find(gpuProgramId);
	if (gpuProgramIt != trackedMap.end())
	{
		auto it = gpuProgramIt->second.find(uniformName);
		if (it != gpuProgramIt->second.end())
		{
			return true;
		}
	}

	return false;
}

void UniformTracker::Track(GpuProgram const& gpuProgram, std::string const& uniformName, std::string const& uniformType)
{
	if (!Contains(gpuProgram.programHandle, uniformName))
	{
		trackedMap[gpuProgram.programHandle][uniformName] = true;
		theLogger.LogInfo("[ shader: {} ] uniform({}) {} cannot be set", gpuProgram.GetPrettyName(), uniformType, uniformName);
	}
}

void GlWrapper::SetUniform(glm::mat4 const& subject, std::string const& uniformName, GpuProgram const& gpuProgram)
{
	auto location = glGetUniformLocation(gpuProgram.programHandle, uniformName.c_str());

	if (location < 0)
		UniformTracker::Track(gpuProgram, uniformName, "mat4");

	glUniformMatrix4fv(location, 1, false, glm::value_ptr(subject));
}

void GlWrapper::SetUniform(glm::mat3 const& subject, std::string const& uniformName, GpuProgram const& gpuProgram)
{
	auto location = glGetUniformLocation(gpuProgram.programHandle, uniformName.c_str());

	if (location < 0)
		UniformTracker::Track(gpuProgram, uniformName, "mat3");

	glUniformMatrix3fv(location, 1, false, glm::value_ptr(subject));
}

void GlWrapper::SetUniform(glm::vec3 const& subject, std::string const& uniformName, GpuProgram const& gpuProgram)
{
	auto location = glGetUniformLocation(gpuProgram.programHandle, uniformName.c_str());

	if (location < 0)
		UniformTracker::Track(gpuProgram, uniformName, "vec3");
	
	glUniform3fv(location, 1, glm::value_ptr(subject));
}

void GlWrapper::SetUniform(std::vector<glm::mat4> const& subject, std::string const& uniformName, GpuProgram const& gpuProgram)
{
	auto location = glGetUniformLocation(gpuProgram.programHandle, uniformName.c_str());

	if (location < 0)
		UniformTracker::Track(gpuProgram, uniformName, "vector<mat4>");

	glUniformMatrix4fv(location, (int)subject.size(), false, glm::value_ptr(subject[0]));
}

void GlWrapper::SetUniform(float const subject, std::string const& uniformName, GpuProgram const& gpuProgram)
{
	auto location = glGetUniformLocation(gpuProgram.programHandle, uniformName.c_str());

	if (location < 0)
		UniformTracker::Track(gpuProgram, uniformName, "float");
	
	glUniform1f(location, subject);
}

void GlWrapper::SetUniform(int const subject, std::string const& uniformName, GpuProgram const& gpuProgram)
{
	auto location = glGetUniformLocation(gpuProgram.programHandle, uniformName.c_str());

	if (location < 0)
		UniformTracker::Track(gpuProgram, uniformName, "int");
	
	glUniform1i(location, subject);
}

std::string GlWrapper::GetString(GLenum const name)
{
	return (const char*)glGetString(name);
}

GLint GlWrapper::GetIntegerv(GLenum const parameterName)
{
	GLint result;
	glGetIntegerv(parameterName, &result);
	return result;
}

GLint GlWrapper::GetProgramiv(GLuint const programHandle, GLenum const parameterName)
{
	GLint result;
	glGetProgramiv(programHandle, parameterName, &result);
	return result;
}

std::string GlWrapper::ResolveDebugSource(GLenum source)
{
	switch (source) {
		case GL_DEBUG_SOURCE_API:
			return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			return "WINDOW SYSTEM";
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			 return "SHADER COMPILER";
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			return "THIRD PARTY";
		case GL_DEBUG_SOURCE_APPLICATION:
			return "APPLICATION";
		case GL_DEBUG_SOURCE_OTHER:
			return "UNKNOWN";
		default:
			return "UNKNOWN";
	}
}

std::string GlWrapper::ResolveDebugType(GLenum type)
{
	switch (type) {
		case GL_DEBUG_TYPE_ERROR:
			return "ERROR";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			return "DEPRECATED BEHAVIOR";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			return "UDEFINED BEHAVIOR";
		case GL_DEBUG_TYPE_PORTABILITY:
			return "PORTABILITY";
		case GL_DEBUG_TYPE_PERFORMANCE:
			return "PERFORMANCE";
		case GL_DEBUG_TYPE_OTHER:
			return "OTHER";
		case GL_DEBUG_TYPE_MARKER:
			return "MARKER";
		default:
			return "UNKNOWN";
	}
}

std::string GlWrapper::ResolveDebugSeverity(GLenum severity)
{
	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:
			return "HIGH";
		case GL_DEBUG_SEVERITY_MEDIUM:
			return "MEDIUM";
		case GL_DEBUG_SEVERITY_LOW:
			return "LOW";
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			return "NOTIFICATION";
		default:
			return "UNKNOWN";
	}
}
