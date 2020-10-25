#pragma once

#include "../utils.h"
#include "gl_object_3d.h"

struct SimpleScene
{
	SimpleScene() = default;
	~SimpleScene() = default;

	std::vector<Object3D> drawableObjects;

	void Create(Utils::WindowSize windowSize);
};