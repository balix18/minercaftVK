#pragma once

#include "surface_texture.h"

struct RenderState
{
	SurfaceTexture* surfaceTexture;

	glm::mat4 model, view, proj;

	static RenderState& Instance();

	RenderState(RenderState const&) = delete;
	RenderState& operator=(RenderState const&) = delete;
	RenderState(RenderState&&) = delete;
	RenderState& operator=(RenderState&&) = delete;

private:
	RenderState();
};

inline RenderState& theRenderState = RenderState::Instance();
