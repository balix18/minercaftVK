#include "surface_texture.h"

#include "gl_wrapper.h"

SurfaceTexture::SurfaceTexture(Type const& type, std::string const& path, Image* image, GLuint textureUnit) :
	texture{ 0 , textureUnit },
	type{ type },
	path{ path }
{
	glGenTextures(1, &texture.handle);
	glBindTexture(GL_TEXTURE_2D, texture.handle);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->imageSize.x, image->imageSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data.get());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// When magnifying the image (no bigger mipmap available), use LINEAR filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// When minifying the image, use a LINEAR blend of two mipmaps, each filtered LINEARLY too
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glGenerateMipmap(GL_TEXTURE_2D);
}

GLuint SurfaceTexture::GetHandle() const
{
	return texture.handle;
}

void SurfaceTexture::SetUniform(std::string const& uniformName, GpuProgram const& gpuProgram) const
{
	GlWrapper::SetUniform(static_cast<int>(texture.unit), uniformName, gpuProgram);

	glActiveTexture(GL_TEXTURE0 + texture.unit);
	glBindTexture(GL_TEXTURE_2D, texture.handle);
}