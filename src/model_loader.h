#pragma once
#include "pch.h"

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	bool operator==(const Vertex& other) const;

	struct Hasher
	{
		std::size_t operator()(Vertex const& vertex) const noexcept;
	};
};

struct TinyObjShape
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	int materialId;
};

struct TinyObjMaterial
{
	std::string diffuseTexture;

	TinyObjMaterial(std::string const& diffuseTexture);
};

struct LoadedModel
{
	std::vector<TinyObjShape> shapes;
	std::vector<TinyObjMaterial> materials;
};

struct ModelLoader
{
	LoadedModel Load(std::string const& fileName, std::string const& mtlDirectory, bool flipWinding = false);
	void CheckMaterialIds(std::vector<tinyobj::shape_t> const& shapes);
};
