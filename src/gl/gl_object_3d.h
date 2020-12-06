#pragma once

struct Camera;
struct GpuProgram;

#include "surface_texture.h"

enum struct VertexLayout
{
	POSITION,
	NORMAL,
	UV,
	TANGENT,
	BITANGENT,
	INDEX
};

struct VertexData
{
	int vertexCount;
	bool cleared;

	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> tangents;
	std::vector<glm::vec3> bitangents;

	VertexData();
	virtual ~VertexData() = default;

	void ClearAll();
};

struct Mesh
{
	uint vao;

	std::unordered_map<VertexLayout, int> vertexHandles;
	VertexData vertexData;
	std::shared_ptr<SurfaceTexture> surfaceTexture;

	int indicesCount;
	std::vector<uint> indices;

	Mesh();
	virtual ~Mesh() = default;

	void Init(uint newVao);
	void UploadVertices(std::unordered_map<VertexLayout, int>& attribLocations);
};

struct Object3D
{
	std::vector<Mesh> meshes;

	Object3D() = default;
	virtual ~Object3D() = default;

	void Draw(GpuProgram const& gpuProgram, Camera const& camera) const;
};

