#pragma once

#include "../model_loader.h"
#include "../utils.h"
#include "gl_object_3d.h"
#include "surface_texture.h"

struct SimpleScene
{
	SimpleScene() = default;
	~SimpleScene() = default;

	std::vector<Object3D> drawableObjects;

	void Create(Utils::WindowSize windowSize);
	void Animate(float currentTime, float deltaTime);

private:
	LoadedModel LoadVikingRoom();
	LoadedModel LoadErato();
	LoadedModel LoadSponza();
};