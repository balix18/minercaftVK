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
		ConvertToMesh(mesh, loadedModel);

		mesh.UploadVertices(attribLocations);
		mesh.vertexData.ClearAll();

		auto fileName = loadedModel.materials[0].diffuseTexture;
		auto image = theImageCache.Load(fileName);

		int textureUnit = 0;
		mesh.surfaceTexture = std::make_shared<SurfaceTexture>(SurfaceTexture::Type::AMBIENT, fileName, image, 0);

		drawableObjects.push_back(std::move(triangle));
	}

	// erato
	{
		//auto loadedModel = LoadErato();
	}

	// sponza
	{
		auto loadedModel = LoadSponza();
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

void SimpleScene::ConvertToMesh(Mesh& mesh, LoadedModel const& loadedModel)
{
	// TODO only works with shape 0
	auto const& shape = loadedModel.shapes[0];

	mesh.vertexData.vertexCount = (int)shape.vertices.size();
	for (auto i = 0; i < shape.vertices.size(); i++) {
		mesh.vertexData.positions.push_back(shape.vertices[i].pos);
		mesh.vertexData.uvs.push_back(shape.vertices[i].texCoord);
	}

	mesh.indicesCount = (int)shape.indices.size();
	mesh.indices = std::move(shape.indices);
}

