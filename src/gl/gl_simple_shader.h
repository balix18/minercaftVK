#pragma once

#include "gl_gpu_program.h"

struct SimpleShader : GpuProgram
{
	SimpleShader();
	virtual ~SimpleShader() = default;

	void Bind() const override;
	void BindMaterial() const override;
	std::string GetPrettyName() const override;
};
