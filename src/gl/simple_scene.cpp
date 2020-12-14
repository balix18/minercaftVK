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
		//Object3D erato;
		//erato.Create(LoadErato());
		//drawableObjects.push_back(std::move(erato));
	}

	// sponza
	{
		//Object3D sponza;
		//sponza.originalTransformation.scale = glm::vec3(0.01f);
		//sponza.Create(LoadSponza());
		//drawableObjects.push_back(std::move(sponza));
	}

	// dragon
	{
		// Object3D dragon;
		// dragon.Create(LoadDragon());
		// drawableObjects.push_back(std::move(dragon));
	}
}

void SimpleScene::Animate(float currentTime, float deltaTime)
{
	for (auto& drawableObject : drawableObjects) {
		drawableObject.animatedTransformation.translate = drawableObject.originalTransformation.translate;
		drawableObject.animatedTransformation.scale = drawableObject.originalTransformation.scale;
		drawableObject.animatedTransformation.rotate = currentTime * glm::radians(22.5f);
		drawableObject.animatedTransformation.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
	}
}

LoadedModel SimpleScene::LoadVikingRoom()
{
	auto baseDir = theRuncfg.texturesDir / "viking_room";
	auto modelFileName = (baseDir / "viking_room.obj").string();
	auto mtlDirectory = baseDir.string();

	ModelLoader modelLoader;
	return modelLoader.Load(modelFileName, mtlDirectory, ModelLoader::LoadSettings{});
}

LoadedModel SimpleScene::LoadErato()
{
	auto baseDir = theRuncfg.texturesDir / "erato";
	auto modelFileName = (baseDir / "erato.obj").string();
	auto mtlDirectory = baseDir.string();

	ModelLoader modelLoader;
	return modelLoader.Load(modelFileName, mtlDirectory, ModelLoader::LoadSettings{});
}

LoadedModel SimpleScene::LoadSponza()
{
	auto baseDir = theRuncfg.texturesDir / "sponza";
	auto modelFileName = (baseDir / "sponza.obj").string();
	auto mtlDirectory = baseDir.string();

	ModelLoader modelLoader;
	return modelLoader.Load(modelFileName, mtlDirectory, ModelLoader::LoadSettings{});
}

LoadedModel SimpleScene::LoadDragon()
{
	auto baseDir = theRuncfg.texturesDir / "dragon";
	auto modelFileName = (baseDir / "dragon.obj").string();
	auto mtlDirectory = baseDir.string();

	ModelLoader::LoadSettings loadSettings;
	loadSettings.ignoreMissingUVs = true;

	ModelLoader modelLoader;
	return modelLoader.Load(modelFileName, mtlDirectory, loadSettings);
}
