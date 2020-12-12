#include "simple_scene.h"

#include "gl_object_3d.h"
#include "../runcfg.h"
#include "../image_cache.h"

void SimpleScene::Create(Utils::WindowSize windowSize)
{
	// upside down triangle
	{
		//mesh.vertexData.vertexCount = 3;
		//mesh.vertexData.positions.push_back({ 0.0, -1.0, 0.0 });	// bottom mid
		//mesh.vertexData.positions.push_back({ 0.5, 0.5, 0.0 });	// top right
		//mesh.vertexData.positions.push_back({ -0.5, 0.5, 0.0 });	// top left

		// ccw bejaras
		//mesh.indicesCount = 3;
		//mesh.indices.push_back(0);
		//mesh.indices.push_back(1);
		//mesh.indices.push_back(2);
	}

	// quad
	{
		//mesh.vertexData.vertexCount = 4;
		//mesh.vertexData.positions.push_back({ -0.5, -0.5, 0.0 });	// bottom left
		//mesh.vertexData.positions.push_back({ 0.5, -0.5, 0.0 });	// bottom right
		//mesh.vertexData.positions.push_back({ 0.5, 0.5, 0.0 });		// top right
		//mesh.vertexData.positions.push_back({ -0.5, 0.5, 0.0 });	// top left

		//mesh.vertexData.uvs.push_back({ 0.0, 0.0 });
		//mesh.vertexData.uvs.push_back({ 1.0, 0.0 });
		//mesh.vertexData.uvs.push_back({ 1.0, 1.0 });
		//mesh.vertexData.uvs.push_back({ 0.0, 1.0});

		//// flip uv
		//for (auto& uv : mesh.vertexData.uvs) {
		//	uv.y = 1.0f - uv.y;
		//}

		//mesh.indicesCount = 6;
		//mesh.indices.push_back(0);
		//mesh.indices.push_back(1);
		//mesh.indices.push_back(2);
		//mesh.indices.push_back(0);
		//mesh.indices.push_back(2);
		//mesh.indices.push_back(3);
	}

	// viking room
	{
		Object3D vikingRoom;
		vikingRoom.Create(LoadVikingRoom());
		drawableObjects.push_back(std::move(vikingRoom));
	}

	// erato
	{
		//auto loadedModel = LoadErato();
	}

	// sponza
	{
		//auto loadedModel = LoadSponza();
	}
}

LoadedModel SimpleScene::LoadVikingRoom()
{
	auto baseDir = theRuncfg.texturesDir / "viking_room";
	auto modelFileName = (baseDir / "viking_room.obj").string();
	auto mtlDirectory = baseDir.string();

	ModelLoader modelLoader;
	return modelLoader.Load(modelFileName, mtlDirectory);
}

LoadedModel SimpleScene::LoadErato()
{
	auto baseDir = theRuncfg.texturesDir / "erato";
	auto modelFileName = (baseDir / "erato.obj").string();
	auto mtlDirectory = baseDir.string();

	ModelLoader modelLoader;
	return modelLoader.Load(modelFileName, mtlDirectory);
}

LoadedModel SimpleScene::LoadSponza()
{
	auto baseDir = theRuncfg.texturesDir / "sponza";
	auto modelFileName = (baseDir / "sponza.obj").string();
	auto mtlDirectory = baseDir.string();

	ModelLoader modelLoader;
	return modelLoader.Load(modelFileName, mtlDirectory);
}
