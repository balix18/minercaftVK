#include "render_state.h"

RenderState& RenderState::Instance()
{
	static RenderState renderState;
	return renderState;
}

RenderState::RenderState()
{
}
