#include "gl_object_3d.h"

#include "../camera.h"
#include "gl_gpu_program.h"
#include "render_state.h"

VertexData::VertexData() :
	vertexCount{ 0 },
	cleared{ false }
{
}

void VertexData::ClearAll()
{
	positions.clear();
	normals.clear();
	uvs.clear();
	tangents.clear();
	bitangents.clear();

	cleared = true;
}

Mesh::Mesh() :
	vao{ 0 },
	indicesCount{ 0 }
{
}

void Mesh::Init(uint newVao)
{
	vao = newVao;
}

void Mesh::UploadVertices(std::unordered_map<VertexLayout, int>& attribLocations)
{
	// A GLM_FORCE_DEFAULT_ALIGNED_GENTYPES miatt kell, mert ezzel
	// peldaul a sizeof(glm::vec3) az 16 lesz a megszokott 12 helyett
	auto vec2ComponentCount = (uint)(sizeof(glm::vec2) / sizeof(float));
	auto vec3ComponentCount = (uint)(sizeof(glm::vec3) / sizeof(float));
	auto vec4ComponentCount = (uint)(sizeof(glm::vec4) / sizeof(float));

	glBindBuffer(GL_ARRAY_BUFFER, vertexHandles[VertexLayout::POSITION]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertexData.vertexCount, vertexData.positions.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(attribLocations[VertexLayout::POSITION]);
	glVertexAttribPointer(attribLocations[VertexLayout::POSITION], vec3ComponentCount, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, vertexHandles[VertexLayout::NORMAL]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertexData.vertexCount, vertexData.normals.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(attribLocations[VertexLayout::NORMAL]);
	glVertexAttribPointer(attribLocations[VertexLayout::NORMAL], vec3ComponentCount, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, vertexHandles[VertexLayout::UV]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * vertexData.vertexCount, vertexData.uvs.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(attribLocations[VertexLayout::UV]);
	glVertexAttribPointer(attribLocations[VertexLayout::UV], vec2ComponentCount, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, vertexHandles[VertexLayout::TANGENT]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertexData.vertexCount, vertexData.tangents.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(attribLocations[VertexLayout::TANGENT]);
	glVertexAttribPointer(attribLocations[VertexLayout::TANGENT], vec3ComponentCount, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, vertexHandles[VertexLayout::BITANGENT]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertexData.vertexCount, vertexData.bitangents.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(attribLocations[VertexLayout::BITANGENT]);
	glVertexAttribPointer(attribLocations[VertexLayout::BITANGENT], vec3ComponentCount, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexHandles[VertexLayout::INDEX]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * indicesCount, indices.data(), GL_STATIC_DRAW);
}

void Object3D::Draw(GpuProgram const& gpuProgram, Camera const& camera) const
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	glm::mat4 identity{ 1.0f };
	theRenderState.model = glm::rotate(identity, time * glm::radians(22.5f), glm::vec3(0.0f, 1.0f, 0.0f));
	theRenderState.view = camera.V();
	theRenderState.proj = camera.P();

	gpuProgram.Bind();

	for (auto const& mesh : meshes)
	{
		theRenderState.surfaceTexture = mesh.surfaceTexture.get();
		gpuProgram.BindMaterial();

		glBindVertexArray(mesh.vao);
		glDrawElements(GL_TRIANGLES, mesh.indicesCount, GL_UNSIGNED_INT, nullptr);
	}
}

void Object3D::Create(LoadedModel const& loadedModel)
{
	static std::unordered_map<VertexLayout, int> attribLocations = {
		{ VertexLayout::POSITION, 0 },
		{ VertexLayout::NORMAL, 1 },
		{ VertexLayout::UV, 2 },
		{ VertexLayout::TANGENT, 3 },
		{ VertexLayout::BITANGENT, 4 },
	};

	std::vector<uint> vaos;
	vaos.resize((int)loadedModel.shapes.size());
	glGenVertexArrays((int)loadedModel.shapes.size(), vaos.data());

	for (int i = 0; i < loadedModel.shapes.size(); i++)
	{
		auto& mesh = meshes.emplace_back();
		mesh.Init(vaos[i]);

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

		auto const& shape = loadedModel.shapes[i];
		ConvertToMesh(mesh, shape);

		mesh.UploadVertices(attribLocations);
		mesh.vertexData.ClearAll();

		auto& material = loadedModel.materials[shape.materialId];
		auto image = theImageCache.Load(material.diffuseTexture);

		int diffuseTextureUnit = 0;
		mesh.surfaceTexture = std::make_shared<SurfaceTexture>(SurfaceTexture::Type::DIFFUSE, image->path, image, diffuseTextureUnit);
	}
}

void Object3D::ConvertToMesh(Mesh& mesh, TinyObjShape const& shape)
{
	mesh.vertexData.vertexCount = (int)shape.vertices.size();
	for (auto i = 0; i < shape.vertices.size(); i++) {
		mesh.vertexData.positions.push_back(shape.vertices[i].pos);
		mesh.vertexData.uvs.push_back(shape.vertices[i].texCoord);
	}

	mesh.indicesCount = (int)shape.indices.size();
	mesh.indices = std::move(shape.indices);
}
