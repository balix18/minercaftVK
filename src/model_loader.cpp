#include "model_loader.h"

#include "runcfg.h"

bool Vertex::operator==(const Vertex& other) const
{
	return pos == other.pos
		&& color == other.color
		&& texCoord == other.texCoord;
}

std::size_t Vertex::Hasher::operator()(Vertex const& vertex) const noexcept
{
	std::size_t seed = 0;
	boost::hash_combine(seed, std::hash<glm::vec3>()(vertex.pos));
	boost::hash_combine(seed, std::hash<glm::vec3>()(vertex.color));
	boost::hash_combine(seed, std::hash<glm::vec2>()(vertex.texCoord));
	return seed;
}

LoadedModel ModelLoader::Load(std::string const& fileName, bool flipWinding)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fileName.c_str())) {
		throw std::runtime_error(warn + err);
	}

	LoadedModel loadedModel;

	std::unordered_map<Vertex, uint32_t, Vertex::Hasher> uniqueVertices;

	for (auto const& shape : shapes) {
		for (auto const& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.find(vertex) == uniqueVertices.end()) {
				uniqueVertices[vertex] = static_cast<uint32_t>(loadedModel.vertices.size());
				loadedModel.vertices.push_back(vertex);
			}

			loadedModel.indices.push_back(uniqueVertices[vertex]);
		}
	}

	if (flipWinding) {
		if (loadedModel.indices.size() % 3 != 0) throw std::runtime_error("index count not matching");

		for (int i = 0; i < loadedModel.indices.size(); i += 3) {
			std::swap(loadedModel.indices[i], loadedModel.indices[i + 2]);
		}
	}

	return loadedModel;
}
