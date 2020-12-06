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

struct LoadedModel
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

struct ModelLoader
{
	LoadedModel Load(std::string const& fileName, bool flipWinding = false);
};
