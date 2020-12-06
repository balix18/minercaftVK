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
