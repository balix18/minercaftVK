#pragma once
#include "../pch.h"

#include "../image_cache.h"
#include "gl_gpu_program.h"

struct SurfaceTexture
{
	struct { GLuint handle, unit; } texture;

	enum struct Type { AMBIENT, DIFFUSE, SPECULAR, NORMAL, DEPTH, SKYBOX } type;

	std::string path;

	SurfaceTexture(Type const& type, std::string const& path, Image* image, GLuint textureUnit);
	~SurfaceTexture() = default;

	GLuint GetHandle() const;
	void SetUniform(std::string const& uniformName, GpuProgram const& gpuProgram) const;
};
