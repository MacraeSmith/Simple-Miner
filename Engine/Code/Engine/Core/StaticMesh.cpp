#include "Engine/Core/StaticMesh.hpp"
#include "Engine/Core/StaticMeshUtils.hpp"
#include "Engine/Core/Material.hpp"
#include "Engine/Core/EngineCommon.hpp"


#ifdef RENDERER_DX12
	#include "Engine/Renderer/RendererDX12.hpp"
	#include "Engine/Renderer/VertexBufferDX12.hpp"
	#include "Engine/Renderer/IndexBufferDX12.hpp"
	#include "Engine/Renderer/ShaderDX12.hpp"
	#include "Engine/Renderer/TextureDX12.hpp"
#else
	#include "Engine/Renderer/RendererDX11.hpp"
	#include "Engine/Renderer/VertexBuffer.hpp"
	#include "Engine/Renderer/IndexBuffer.hpp"
	#include "Engine/Renderer/Shader.hpp"
	#include "Engine/Renderer/Texture.hpp"
#endif 



StaticMesh::StaticMesh(Renderer* renderer, std::string const& meshFolderPath)
	:m_defaultMaterial(new Material(renderer, Stringf("%s_mat", meshFolderPath.c_str()), nullptr))
{
#ifdef RENDERER_DX12
	RendererDX12* rendererDX12 = dynamic_cast<RendererDX12*>(renderer);
	if (rendererDX12)
	{
		m_renderer = rendererDX12;
	}

	else
	{
		ERROR_AND_DIE("Trying to create DX12 Static Mesh with DX11 Renderer");
	}

#else

	RendererDX11* rendererDX11 = dynamic_cast<RendererDX11*>(renderer);
	if (rendererDX11)
	{
		m_renderer = rendererDX11;
	}

	else
	{
		ERROR_AND_DIE("Trying to create DX11 Static Mesh with DX12 Renderer");
	}

#endif // RENDERER_DX12

	if (!LoadOBJMeshFile(this, meshFolderPath))
	{
		Destroy();
	}

}


StaticMesh::StaticMesh(Renderer* renderer, VertTBNs const& verts, IndexList indexes, StaticMeshConfig const& config)
	:m_staticMeshName(config.m_name)
	,m_defaultMaterial(new Material(renderer, Stringf("%s_mat", config.m_name.c_str()), nullptr))
{
#ifdef RENDERER_DX12
	RendererDX12* rendererDX12 = dynamic_cast<RendererDX12*>(renderer);
	if (rendererDX12)
	{
		m_renderer = rendererDX12;
	}

	else
	{
		ERROR_AND_DIE("Trying to create DX12 Static Mesh with DX11 Renderer");
	}

#else

	RendererDX11* rendererDX11 = dynamic_cast<RendererDX11*>(renderer);
	if (rendererDX11)
	{
		m_renderer = rendererDX11;
	}

	else
	{
		ERROR_AND_DIE("Trying to create DX11 Static Mesh with DX12 Renderer");
	}

#endif // RENDERER_DX12

	m_defaultMaterial->SetTextures(config.m_texture);
	m_defaultMaterial->SetShader(config.m_shader);
	m_transform.AppendTranslation3D(config.m_position);
	Mat44 rotationMat = config.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	m_transform.Append(rotationMat);
	m_transform.AppendScaleUniform3D(config.m_uniformScale);
	m_color = config.m_color;

	SetBuffers(verts, indexes);

}

StaticMesh::StaticMesh(Renderer* renderer, Verts const& verts, StaticMeshConfig const& config)
	:m_staticMeshName(config.m_name)
	,m_defaultMaterial(new Material(renderer, Stringf("%s_mat", config.m_name.c_str()), config.m_shader))
{
#ifdef RENDERER_DX12
	RendererDX12* rendererDX12 = dynamic_cast<RendererDX12*>(renderer);
	if (rendererDX12)
	{
		m_renderer = rendererDX12;
	}

	else
	{
		ERROR_AND_DIE("Trying to create DX12 Static Mesh with DX11 Renderer");
	}

#else

	RendererDX11* rendererDX11 = dynamic_cast<RendererDX11*>(renderer);
	if (rendererDX11)
	{
		m_renderer = rendererDX11;
	}

	else
	{
		ERROR_AND_DIE("Trying to create DX11 Static Mesh with DX12 Renderer");
	}

#endif // RENDERER_DX12
	
	m_defaultMaterial->SetTextures(config.m_texture);
	m_transform.AppendTranslation3D(config.m_position);
	Mat44 rotationMat = config.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	m_transform.Append(rotationMat);
	m_transform.AppendScaleUniform3D(config.m_uniformScale);
	m_color = config.m_color;

	SetBuffers(verts);
}


StaticMesh::~StaticMesh()
{
	Destroy();
}

void StaticMesh::Render(Mat44 const& worldTransform, Rgba8 const& color, Material* overrideMaterial) const
{
	if(!m_renderer || !m_vbo)
		return;

	m_renderer->BeginRendererEvent(Stringf("Draw - %s", m_staticMeshName.c_str()).c_str());
	if(overrideMaterial)
	{
		overrideMaterial->BindMaterial();
	}

	else if(m_currentMaterial)
	{
		m_currentMaterial->BindMaterial();
	}

	else
	{
		m_defaultMaterial->BindMaterial();
	}


	Mat44 transformToRender = m_transform;
	transformToRender.Append(worldTransform);

	Rgba8 colorToRender = m_color * color;
 
 	m_renderer->SetModelConstants(transformToRender, color);

	if(m_ibo)
	{
		m_renderer->DrawIndexedVertexBuffer(m_vbo, m_ibo, m_numIndexes);
	}

	else
	{
		m_renderer->DrawVertexBuffer(m_vbo, m_numVertexes);
	}

	m_renderer->EndRendererEvent();
}

void StaticMesh::SetTransform(Vec3 const& position, EulerAngles const& orientation, float uniformScale)
{
	Vec3 fwrd;
	Vec3 left;
	Vec3 up;
	orientation.GetAsVectors_IFwd_JLeft_KUp(fwrd, left, up);
	m_transform = Mat44(fwrd, left, up, position);
	m_transform.AppendScaleUniform3D(uniformScale);
}

void StaticMesh::SetMaterial(Material* material)
{
	m_currentMaterial = material;
}

Material* StaticMesh::GetDefaultMaterial() const
{
	return m_defaultMaterial;
}

void StaticMesh::SetBuffers(VertTBNs const& verts, std::vector<unsigned int> const& indexes)
{
#ifdef RENDERER_DX12
	m_vbo = m_renderer->CreateVertexBuffer(verts, Stringf("%s_VertexBuffer", m_staticMeshName.c_str()));
	m_ibo = m_renderer->CreateIndexBuffer(indexes, Stringf("%s_IndexBuffer", m_staticMeshName.c_str()));
#else
	m_vbo = m_renderer->CreateVertexBuffer(sizeof(Vertex_PCUTBN) * (int)verts.size(), sizeof(Vertex_PCUTBN));
	m_ibo = m_renderer->CreateIndexBuffer(sizeof(unsigned int) * (int)indexes.size());
	m_renderer->CopyCPUToGPU(verts.data(), sizeof(Vertex_PCUTBN) * (int)verts.size(), m_vbo);
	m_renderer->CopyCPUToGPU(indexes.data(), sizeof(unsigned int) * (int)indexes.size(), m_ibo);
#endif
	m_numVertexes = (unsigned int)verts.size();
	m_numIndexes = (int)indexes.size();
}

void StaticMesh::SetBuffers(Verts const& verts)
{
#ifdef RENDERER_DX12
	m_vbo = m_renderer->CreateVertexBuffer(verts, Stringf("%s_VertexBuffer", m_staticMeshName.c_str()));
#else
	m_vbo = m_renderer->CreateVertexBuffer(sizeof(Vertex_PCUTBN) * (int)verts.size(), sizeof(Vertex_PCUTBN));
	m_renderer->CopyCPUToGPU(verts.data(), sizeof(Vertex_PCUTBN) * (int)verts.size(), m_vbo);
#endif
	m_numVertexes = (unsigned int)verts.size();
}

void StaticMesh::SetShader(std::string const& shaderFilePath)
{
	if (shaderFilePath != "INVALID_SHADER")
	{
#ifdef RENDERER_DX12
		m_defaultMaterial->SetShader(m_renderer->CreateOrGetShaderFromFile(shaderFilePath.c_str(), VertexType::VERTEX_PCUTBN));
#else
		m_defaultMaterial->SetShader(m_renderer->CreateOrGetShaderFromFile(shaderFilePath.c_str(), VertexType::VERTEX_PCUTBN));
#endif

	}
}

void StaticMesh::SetTextures(Strings const& textureFilePaths)
{
#ifdef RENDERER_DX12
	std::vector<TextureDX12 const*> textures;
#else

	std::vector<Texture const*> textures;
#endif 

	textures.reserve((int)textureFilePaths.size());

	for (int i = 0; i < (int)textureFilePaths.size(); ++i)
	{
		std::string lowercaseTexture = GetLowercase(textureFilePaths[i]);

#ifdef RENDERER_DX12
		TextureDX12* newTexture = nullptr;
#else

		Texture* newTexture = nullptr;
#endif 

		if (lowercaseTexture == "zero")
		{
			newTexture = m_renderer->CreateOrGetTextureFromFile("Black");
		}

		else if (lowercaseTexture == "one")
		{
			newTexture = m_renderer->CreateOrGetTextureFromFile("White");
		}
		else if (lowercaseTexture != "invalid_texture")
		{
			newTexture = m_renderer->CreateOrGetTextureFromFile(textureFilePaths[i].c_str());
		}

		textures.push_back(newTexture);
	}

	m_defaultMaterial->SetTextures(textures);
}

void StaticMesh::SetStaticMeshNameAndFilePath(std::string const& name, std::string const& filePath)
{
	m_staticMeshName = name;
	m_staticMeshFilePath = filePath;
}

void StaticMesh::Destroy()
{
	delete m_vbo;
	m_vbo = nullptr;
	delete m_ibo;
	m_ibo = nullptr;
	delete m_defaultMaterial;
	m_defaultMaterial = nullptr;
	delete m_currentMaterial;
	m_currentMaterial = nullptr;
}

