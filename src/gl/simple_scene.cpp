#include "simple_scene.h"

#include "gl_object_3d.h"
#include "../runcfg.h"
#include "../image_cache.h"

void SimpleScene::Create(Utils::WindowSize windowSize)
{
	std::unordered_map<VertexLayout, int> attribLocations = {
			{ VertexLayout::POSITION, 0 },
			{ VertexLayout::NORMAL, 1 },
			{ VertexLayout::UV, 2 },
			{ VertexLayout::TANGENT, 3 },
			{ VertexLayout::BITANGENT, 4 },
	};

	Object3D triangle;
	triangle.meshes.emplace_back();

	uint vao;
	glGenVertexArrays(1, &vao);

	auto& mesh = triangle.meshes[0];
	mesh.Init(vao);

	glBindVertexArray(mesh.vao);

	std::vector<uint> bufferList;
	bufferList.resize(attribLocations.size() + 1);

	glGenBuffers((int)bufferList.size(), bufferList.data());
	mesh.vertexHandles[VertexLayout::POSITION] = bufferList[0];
	mesh.vertexHandles[VertexLayout::NORMAL] = bufferList[1];
	mesh.vertexHandles[VertexLayout::UV] = bufferList[2];
	mesh.vertexHandles[VertexLayout::TANGENT] = bufferList[3];
	mesh.vertexHandles[VertexLayout::BITANGENT] = bufferList[4];
	mesh.vertexHandles[VertexLayout::INDEX] = bufferList[5];

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
		auto loadedModel = LoadVikingRoom();

		mesh.vertexData.vertexCount = (int)loadedModel.vertices.size();
		for (auto i = 0; i < loadedModel.vertices.size(); i++) {
			mesh.vertexData.positions.push_back(loadedModel.vertices[i].pos);
			mesh.vertexData.uvs.push_back(loadedModel.vertices[i].texCoord);
		}

		mesh.indicesCount = (int)loadedModel.indices.size();
		mesh.indices = std::move(loadedModel.indices);
	}

	mesh.UploadVertices(attribLocations);
	mesh.vertexData.ClearAll();

	auto fileName = (theRuncfg.texturesDir / "viking_room.png").string();
	auto image = theImageCache.Load(fileName);

	int textureUnit = 0;
	mesh.surfaceTexture = std::make_shared<SurfaceTexture>(SurfaceTexture::Type::AMBIENT, fileName, image, 0);

	drawableObjects.push_back(std::move(triangle));
}

LoadedModel SimpleScene::LoadVikingRoom()
{
	auto modelFileName = (theRuncfg.texturesDir / "viking_room.obj").string();

	ModelLoader modelLoader;
	return modelLoader.Load(modelFileName);
}

