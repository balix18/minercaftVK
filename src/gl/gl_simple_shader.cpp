#include "gl_simple_shader.h"

#include "../runcfg.h"

SimpleShader::SimpleShader()
{
	shaderPathList.emplace_back(ShaderPath::Type::VERT, "simple.vert");
	shaderPathList.emplace_back(ShaderPath::Type::FRAG, "simple.frag");
	Create();
}

void SimpleShader::Bind() const
{
	glUseProgram(programHandle);
}

void SimpleShader::BindMaterial() const
{
	// Nem kell
}

std::string SimpleShader::GetPrettyName() const
{
	return "SimpleShader";
}
