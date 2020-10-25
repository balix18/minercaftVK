#include "simple_scene.h"

#include "gl_object_3d.h"

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
	triangle.meshes.push_back(Mesh{});

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

	mesh.vertexData.vertexCount = 3;
	mesh.vertexData.positions.push_back({ 0.0, -0.9, 0.0 });
	mesh.vertexData.positions.push_back({ 0.5, 0.5, 0.0 });
	mesh.vertexData.positions.push_back({ -0.5, 0.5, 0.0 });

	mesh.indicesCount = 3;
	mesh.indices.push_back(0);
	mesh.indices.push_back(1);
	mesh.indices.push_back(2);

	mesh.UploadVertices(attribLocations);
	mesh.vertexData.ClearAll();

	drawableObjects.push_back(std::move(triangle));
}
