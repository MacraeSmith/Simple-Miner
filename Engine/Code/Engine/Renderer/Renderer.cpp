#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Renderer/DefaultShader.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StaticMesh.hpp"

#include<vector>
#include "ThirdParty/stb/stb_image.h"


#define WIN32_LEAN_AND_MEAN	
#include <windows.h>
#if defined(ENGINE_DEBUG_RENDERER)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif


#if defined(OPAQUE)
#undef OPAQUE
#endif
extern Window* g_window;

Renderer::Renderer(RendererConfig const& config)
	:m_config(config)
{
}


//Shaders
//----------------------------------------------------------------------------------------------------------------------


LightData::LightData(Rgba8 const& color, Vec3 const& position, float innerRadius, float outerRadius, Vec3 const& spotForward, float penumbraInnerDotThreshold, float penumbraOuterDotThreshold)
	:m_position(position)
	,m_innerRadius(innerRadius)
	,m_outerRadius(outerRadius)
	,m_spotForward(spotForward)
	,m_penumbraInnerDotThreshold(penumbraInnerDotThreshold)
	,m_penumbraOuterDotThreshold(penumbraOuterDotThreshold)
{
	color.GetAsFloats(m_color);
}

bool LightData::IsPointLight() const
{
	return m_spotForward == Vec3::ZERO;
}

LightConstants::LightConstants(Vec3 const& sunDirection, float sunIntensity, float ambientIntensity, Rgba8 const& sunColorRGB)
	:m_sunDirection(sunDirection)
	,m_sunIntensity(sunIntensity)
	,m_ambientIntensity(ambientIntensity)
	,m_numLights(0)
{
	m_sunColorRGB[0] = NormalizeByte(sunColorRGB.r);
	m_sunColorRGB[1] = NormalizeByte(sunColorRGB.g);
	m_sunColorRGB[2] = NormalizeByte(sunColorRGB.b);
}

LightConstants::LightConstants(Vec3 const& sunDirection, float sunIntensity, float ambientIntensity, std::vector<LightData> lights, Rgba8 const& sunColorRGB)
	:m_sunDirection(sunDirection)
	,m_sunIntensity(sunIntensity)
	,m_ambientIntensity(ambientIntensity)
	,m_numLights((int)lights.size())
{
	m_sunColorRGB[0] = NormalizeByte(sunColorRGB.r);
	m_sunColorRGB[1] = NormalizeByte(sunColorRGB.g);
	m_sunColorRGB[2] = NormalizeByte(sunColorRGB.b);

	if ((int)lights.size() > MAX_NUM_LIGHTS)
	{
		m_numLights = MAX_NUM_LIGHTS;
	}

	for (int i = 0; i < m_numLights; ++i)
	{
		m_lights[i] = lights[i];
	}
}

std::string Renderer::GetNameForBlendMode(BlendMode const& blendMode)
{
	switch (blendMode)
	{
	case BlendMode::OPAQUE: return "OPAQUE";
	case BlendMode::ALPHA: return "ALPHA";
	case BlendMode::ADDITIVE: return "ADDITIVE";
	case BlendMode::LIGHTEN: return "LIGHTEN";
	default: return "UNAMED_BLEND_MODE";
	}
}

std::string Renderer::GetNameForDepthMode(DepthMode const& depthMode)
{
	switch (depthMode)
	{
	case DepthMode::DISABLED: return "DISABLED";
	case DepthMode::READ_ONLY_ALWAYS: return "READ_ONLY_ALWAYS";
	case DepthMode::READ_ONLY_LESS_EQUAL: return "READ_ONLY_LESS_EQUAL";
	case DepthMode::READ_WRITE_LESS_EQUAL: return "READ_WRITE_LESS_EQUAL";
	default: return "UNAMED_DEPTH_MODE";
	}
}

std::string Renderer::GetNameForRasterizerMode(RasterizerMode const& rasterizerMode)
{
	switch (rasterizerMode)
	{
	case RasterizerMode::SOLID_CULL_NONE: return "SOLID_CULL_NONE";
	case RasterizerMode::SOLID_CULL_BACK: return "SOLID_CULL_BACK";
	case RasterizerMode::SOLID_CULL_FRONT : return "SOLID_CULL_FRONT";
	case RasterizerMode::WIREFRAME_CULL_NONE: return "WIREFRAME_CULL_NONE";
	case RasterizerMode::WIREFRAME_CULL_BACK: return "WIREFRAME_CULL_BACK";
	case RasterizerMode::WIREFRAME_CULL_FRONT: return "WIREFRAME_CULL_FRONT";
	default: return "UNAMED_RASTERIZER_MODE";
	}
}

BlendMode Renderer::GetBlendModeFromName(std::string const& name)
{
	std::string nameLowerCase = GetLowercase(name);
	if(name == "opaque")
		return BlendMode::OPAQUE;

	else if (name == "alpha")
		return BlendMode::ALPHA;

	else if (name == "additive")
		return BlendMode::ADDITIVE;

	else if (name == "lighten")
		return BlendMode::LIGHTEN;

	return BlendMode::OPAQUE;
}

StaticMesh* Renderer::CreateOrGetStaticMeshFromFile(std::string const& filePath)
{
	for (int i = 0; i < (int)m_loadedStaticMeshes.size(); ++i)
	{
		if (m_loadedStaticMeshes[i] && m_loadedStaticMeshes[i]->m_staticMeshFilePath == filePath)
		{
			return m_loadedStaticMeshes[i];
		}
	}

	StaticMesh* staticMesh = new StaticMesh(this, filePath);
	m_loadedStaticMeshes.push_back(staticMesh);
	return staticMesh;
}

StaticMesh* Renderer::CreateOrGetStaticMeshFromVerts(Verts const& verts, StaticMeshConfig const& config)
{
	for (int i = 0; i < (int)m_loadedStaticMeshes.size(); ++i)
	{
		if (m_loadedStaticMeshes[i] && m_loadedStaticMeshes[i]->m_staticMeshName == config.m_name)
		{
			return m_loadedStaticMeshes[i];
		}
	}

	StaticMesh* newMesh = new StaticMesh(this, verts, config);
	m_loadedStaticMeshes.push_back(newMesh);
	return newMesh;
}

StaticMesh* Renderer::CreateOrGetStaticMeshFromVertTBNs(VertTBNs const& verts, IndexList const& indexes, StaticMeshConfig const& config)
{
	for (int i = 0; i < (int)m_loadedStaticMeshes.size(); ++i)
	{
		if (m_loadedStaticMeshes[i] && m_loadedStaticMeshes[i]->m_staticMeshName == config.m_name)
		{
			return m_loadedStaticMeshes[i];
		}
	}

	StaticMesh* newMesh = new StaticMesh(this, verts, indexes, config);
	m_loadedStaticMeshes.push_back(newMesh);
	return newMesh;
}


BitmapFont* Renderer::GetBitMapFontForFileName(char const* bitmapFontFilePathWithNoExtension) const
{
	for (int fontIndex = 0; fontIndex < (int)(m_loadedFonts.size()); ++fontIndex)
	{
		if (m_loadedFonts[fontIndex]->m_fontFilePathNameWithNoExtension == static_cast<std::string>(bitmapFontFilePathWithNoExtension))
		{
			return m_loadedFonts[fontIndex];
		}
	}

	return nullptr;
}


void Renderer::SetBlendMode(BlendMode blendMode)
{
	m_desiredBlendMode = blendMode;
}

void Renderer::SetSamplerMode(SamplerMode samplerMode, int slot)
{
	m_desiredSamplerModeBySlot[slot] = samplerMode;
}

void Renderer::SetRasterizerMode(RasterizerMode rasterizerMode)
{
	m_desiredRasterizerMode = rasterizerMode;
}

void Renderer::SetDepthMode(DepthMode depthMode)
{
	m_desiredDepthMode = depthMode;
}
