#pragma once
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include <string>
#include <vector>

#include "Game/EngineBuildPreferences.hpp"
#ifdef RENDERER_DX12
class RendererDX12;
class VertexBufferDX12;
class IndexBufferDX12;
class ShaderDX12;
class TextureDX12;
class PipelineStateObjectDX12;
#else
class RendererDX11;
class VertexBuffer;
class IndexBuffer;
class Texture;
class Shader;
#endif // RENDERER_DX12

class Renderer;
class Material;
typedef std::vector<std::string> Strings;
enum class BlendMode : int;

struct StaticMeshConfig
{
	std::string m_name;

#ifdef RENDERER_DX12
	TextureDX12 const* m_texture = nullptr;
	ShaderDX12 const* m_shader = nullptr;
#else
	Texture const* m_texture = nullptr;
	Shader const* m_shader = nullptr;
#endif

	std::string m_filePath;
	Vec3 m_position;
	EulerAngles m_orientation;
	float m_uniformScale = 1.f;
	Rgba8 m_color = Rgba8::WHITE;

};

class StaticMesh
{
friend class Renderer;
protected:
	StaticMesh(Renderer* renderer, std::string const& meshFolderPath);
	StaticMesh(Renderer* renderer, VertTBNs const& verts, IndexList indexes, StaticMeshConfig const& config);
	StaticMesh(Renderer* renderer, Verts const& verts, StaticMeshConfig const& config);

public:
	~StaticMesh();
	void Render(Mat44 const& worldTransform, Rgba8 const& color = Rgba8::WHITE, Material* overrideMaterial = nullptr) const;
	void SetTransform(Vec3 const& position, EulerAngles const& orientation = EulerAngles::ZERO, float uniformScale = 1.f);
	void SetMaterial(Material* material);
	Material* GetDefaultMaterial() const;

	void SetBuffers(VertTBNs const& verts, std::vector<unsigned int> const& indexes);
	void SetBuffers(Verts const& verts);
	void SetShader(std::string const& shaderFilePath);
	void SetTextures(Strings const& textureFilePaths);
	void SetStaticMeshNameAndFilePath(std::string const& name, std::string const& filePath);

#ifdef RENDERER_DX12
	VertexBufferDX12* GetVertexBuffer() const { return m_vbo; }
	IndexBufferDX12* GetIndexBuffer() const { return m_ibo; }
#else
	VertexBuffer* GetVertexBuffer() const {return m_vbo;}
	IndexBuffer* GetIndexBuffer() const {return m_ibo;}
	
#endif // RendererDX12

private:
	void Destroy();

private:
#ifdef RENDERER_DX12
	VertexBufferDX12* m_vbo = nullptr;
	IndexBufferDX12* m_ibo = nullptr;
	RendererDX12* m_renderer = nullptr;

#else
	VertexBuffer* m_vbo = nullptr;
	IndexBuffer* m_ibo = nullptr;
	RendererDX11* m_renderer = nullptr;
#endif

	std::string m_staticMeshName;
	std::string m_staticMeshFilePath;
	Material* m_defaultMaterial = nullptr;
	Material* m_currentMaterial = nullptr;
	Mat44 m_transform = Mat44::IDENTITY;
	unsigned int m_numIndexes = 0;
	unsigned int m_numVertexes = 0;
	Rgba8 m_color = Rgba8::WHITE;
};

