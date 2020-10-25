#include "gl_gpu_program.h"

#include "../runcfg.h"
#include "../utils.h"
#include "gl_wrapper.h"

ShaderPath::ShaderPath(Type type, std::string const& path):
	type{ type },
	path{ path }
{
}

unsigned ShaderPath::GetGlType(Type type)
{
	switch (type)
	{
		case Type::VERT: return GL_VERTEX_SHADER;
		case Type::FRAG: return GL_FRAGMENT_SHADER;
		case Type::GEOM: return GL_GEOMETRY_SHADER;
		case Type::COMP: return GL_COMPUTE_SHADER;
		case Type::TESC: return GL_TESS_CONTROL_SHADER;
		case Type::TESE: return GL_TESS_EVALUATION_SHADER;
		default: throw std::runtime_error("Invalid type");
	}
}

GpuProgram::GpuProgram():
	programHandle{ 0 },
	debugPrintActiveNames{ true }
{
}

GpuProgram::~GpuProgram()
{
	glDeleteProgram(programHandle);
}

void GpuProgram::GetErrorInfo(unsigned handle)
{
	int logLength;
	glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0) {
		std::vector<char> log(logLength);
		glGetShaderInfoLog(handle, logLength, nullptr, log.data());
		theLogger.LogError("Shader error log:\n{}", log.data());
	}
}

void GpuProgram::CheckShader(unsigned shader, std::string const& message)
{
	int ok;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		theLogger.LogError("{}", message);
		GetErrorInfo(shader);
	}
}

void GpuProgram::CheckLinking(unsigned program)
{
	int ok;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok) {
		theLogger.LogError("Failed to link shader program");
		GetErrorInfo(program);
		throw std::runtime_error("Shader link failed");
	}
}

GLuint GpuProgram::CompileShader(GLenum shaderType, std::string const& path)
{
	auto shaderHandle = glCreateShader(shaderType);

	if (!shaderHandle) {
		throw std::runtime_error("Error in vertex shader creation");
	}

	auto shaderSource = Utils::ReadTextFile(path);
	auto shaderSourcePtr = shaderSource.c_str();
	glShaderSource(shaderHandle, 1, &shaderSourcePtr, nullptr);

	glCompileShader(shaderHandle);
	CheckShader(shaderHandle, fmt::format("Shader error in file: {}", path));

	return shaderHandle;
}

void GpuProgram::Create()
{
	programHandle = glCreateProgram();

	if (!programHandle) throw std::runtime_error("Error in shader program creation");

	auto shaderHandleList = std::vector<unsigned>();
	for (auto const& shaderPath : shaderPathList)
	{
		// TODO Check shader type by extension
		auto resolvedShaderPath = (theRuncfg.shadersDir / shaderPath.path).string();
		auto shaderHandle = CompileShader(ShaderPath::GetGlType(shaderPath.type), resolvedShaderPath);
		glAttachShader(programHandle, shaderHandle);

		shaderHandleList.push_back(shaderHandle);
	}

	glLinkProgram(programHandle);
	CheckLinking(programHandle);

	for (auto const& shaderHandle : shaderHandleList)
	{
		glDetachShader(programHandle, shaderHandle);
		glDeleteShader(shaderHandle);
	}

	PrintActiveNames();
}

void GpuProgram::PrintActiveNames()
{
	if (!debugPrintActiveNames) return;

	std::stringstream ss;
	ss << fmt::format("Shader info:\n\n{}:\n", GetPrettyName());

	auto attributesLength = GlWrapper::GetProgramiv(programHandle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH);
	auto uniformLength = GlWrapper::GetProgramiv(programHandle, GL_ACTIVE_UNIFORM_MAX_LENGTH);
	auto maxLength = std::max(attributesLength, uniformLength);

	std::vector<char> name(maxLength);
	int length, size;
	unsigned int type;	// TODO unordered_set for the type lookup

	int attributesCount = GlWrapper::GetProgramiv(programHandle, GL_ACTIVE_ATTRIBUTES);
	ss << fmt::format(" +-- Active Attributes: {}\n", attributesCount);

	for (int i = 0; i < attributesCount; i++)
	{
		glGetActiveAttrib(programHandle, i, maxLength, &length, &size, &type, name.data());
		ss << fmt::format(" |   +-- Attribute #{} Type: {}, Name: {}\n", i, type, name.data());
	}

	ss << fmt::format(" | \n");

	int uniformCount = GlWrapper::GetProgramiv(programHandle, GL_ACTIVE_UNIFORMS);
	ss << fmt::format(" +-- Active Uniforms: {}\n", uniformCount);

	for (int i = 0; i < uniformCount; i++)
	{
		glGetActiveUniform(programHandle, (GLuint)i, maxLength, &length, &size, &type, name.data());
		ss << fmt::format("     +-- Uniform #{} Type: {}, Name: {}\n", i, type, name.data());
	}

	theLogger.LogInfo(ss.str());
}
