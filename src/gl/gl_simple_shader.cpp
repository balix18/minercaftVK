#include "gl_simple_shader.h"

#include "../runcfg.h"
#include "gl_wrapper.h"
#include "render_state.h"

SimpleShader::SimpleShader()
{
	shaderPathList.emplace_back(ShaderPath::Type::VERT, "simple.vert");
	shaderPathList.emplace_back(ShaderPath::Type::FRAG, "simple.frag");
	Create();
}

void SimpleShader::Bind() const
{
	glUseProgram(programHandle);

	GlWrapper::SetUniform(theRenderState.model, "model", *this);
	GlWrapper::SetUniform(theRenderState.view, "view", *this);
	GlWrapper::SetUniform(theRenderState.proj, "proj", *this);
}

void SimpleShader::BindMaterial() const
{
	theRenderState.surfaceTexture->SetUniform("texSampler", *this);
}

std::string SimpleShader::GetPrettyName() const
{
	return "SimpleShader";
}
