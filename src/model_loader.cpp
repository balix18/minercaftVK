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

LoadedModel ModelLoader::Load(std::string const& fileName, std::string const& mtlDirectory, bool flipWinding)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	theLogger.LogInfo("Loading {}", fileName);

	auto timerStart = std::chrono::high_resolution_clock::now();

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fileName.c_str(), mtlDirectory.c_str())) {
		throw std::runtime_error(warn + err);
	}

	auto timerStop = std::chrono::high_resolution_clock::now();
	auto loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(timerStop - timerStart).count();
	theLogger.LogInfo("Loading {} finished in {} ms", fileName, loadTime);

	if (!warn.empty()) {
		theLogger.LogWarning("LoadObj: {}\n", warn);
	}

	if (!err.empty()) {
		theLogger.LogError("LoadObj: {}\n", err);
		throw std::runtime_error(err);
	}

	CheckMaterialIds(shapes);

	LoadedModel loadedModel;

	for (auto const& material : materials)
	{
		fs::path mtlDirectoryPath = mtlDirectory;
		auto fullDiffuse = HandleDefaultTexure(mtlDirectoryPath / material.diffuse_texname);

		TinyObjMaterial newMaterial(fullDiffuse);
		loadedModel.materials.push_back(std::move(newMaterial));
	}

	for (auto const& shape : shapes)
	{
		TinyObjShape newShape;
		newShape.materialId = shape.mesh.material_ids[0];

		std::unordered_map<Vertex, uint32_t, Vertex::Hasher> uniqueVertices;
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
				uniqueVertices[vertex] = static_cast<uint32_t>(newShape.vertices.size());
				newShape.vertices.push_back(vertex);
			}

			newShape.indices.push_back(uniqueVertices[vertex]);
		}

		if (flipWinding) {
			if (newShape.indices.size() % 3 != 0) throw std::runtime_error("index count not matching");

			for (int i = 0; i < newShape.indices.size(); i += 3) {
				std::swap(newShape.indices[i], newShape.indices[i + 2]);
			}
		}

		loadedModel.shapes.push_back(std::move(newShape));
	}

	LoadMaterialTextures(loadedModel.materials);

	return loadedModel;
}

void ModelLoader::CheckMaterialIds(std::vector<tinyobj::shape_t> const& shapes)
{
	for (auto const& shape : shapes)
	{
		auto firstMaterialId = shape.mesh.material_ids[0];
		for (int materialIdIdx = 1; materialIdIdx < shape.mesh.material_ids.size(); materialIdIdx++) {
			auto materialId = shape.mesh.material_ids[materialIdIdx];
			if (materialId != firstMaterialId) {
				throw std::runtime_error("materialIds are mixed up");
			}
		}
	}
}

void ModelLoader::LoadMaterialTextures(std::vector<TinyObjMaterial> const& materials)
{
	for (auto const& material : materials) {
		theImageCache.Load(material.diffuseTexture);
	}
}

std::string ModelLoader::HandleDefaultTexure(fs::path const& path)
{
	if (fs::is_regular_file(path)) {
		return path.string();
	}

	return (theRuncfg.texturesDir / "helper" / "default_diffuse.tga").string();
}

TinyObjMaterial::TinyObjMaterial(std::string const& diffuseTexture)
	: diffuseTexture{ diffuseTexture }
{
}
