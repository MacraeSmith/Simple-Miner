#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include "Engine/Core/VertexUtils.hpp"

#include <vector>
#include <string>

class Window;
struct IntVec2;
class BitmapFont;
class Image;
class StaticMesh;
struct StaticMeshConfig;
class Texture;
class TextureDX12;


#define DX_SAFE_RELEASE(dxObject)	\
{									\
	if ((dxObject) != nullptr)		\
		{							\
			(dxObject)->Release();	\
			(dxObject) = nullptr;	\
		}							\
}

enum class RendererType : int
{
	DIRECTX_11,
	DIRECTX_12,
	NUM_RENDERING_TYPES
};
struct RendererConfig 
{
	Window* m_window = nullptr;
	RendererType m_renderingType = RendererType::DIRECTX_11;
	bool m_useImGUI = false;
};


enum class VertexType : int
{
	VERTEX_PCU,
	VERTEX_PCUTBN,
	VERTEX_TERRAIN,
	COUNT
};

#if defined(OPAQUE)
#undef OPAQUE
#endif
enum class BlendMode : int
{
	OPAQUE,
	ALPHA,
	ADDITIVE,
	LIGHTEN,
	COUNT,
	//add to GetNameForBlendMode() for future additions
};

enum class SamplerMode : int
{
	POINT_CLAMP,
	BILINEAR_WRAP,
	ANISOTROPIC_WRAP,
	COUNT
};

enum class RasterizerMode : int
{
	SOLID_CULL_NONE,
	SOLID_CULL_BACK,
	SOLID_CULL_FRONT,
	WIREFRAME_CULL_NONE,
	WIREFRAME_CULL_BACK,
	WIREFRAME_CULL_FRONT,
	COUNT
	//add to GetNameForRasterizerMode() for future additions
};

enum class DepthMode : int
{
	DISABLED,
	READ_ONLY_ALWAYS,
	READ_ONLY_LESS_EQUAL,
	READ_WRITE_LESS_EQUAL,
	COUNT
	//add to GetNameForDepthMode() for future additions
};

constexpr int MAX_NUM_LIGHTS = 64;
struct LightData
{
	LightData() {};
	LightData(Rgba8 const& color, Vec3 const& position, float innerRadius, float outerRadius,
		Vec3 const& spotForward = Vec3::ZERO, float penumbraInnerDotThreshold = -1.f, float penumbraOuterDotThreshold = -2.f); //Default variables if unset will make a point light
	bool IsPointLight() const;

	float m_color[4] = {1.f, 1.f, 1.f, 1.f}; // alpha is intensity [0,1]
	Vec3 m_position;
	float m_innerRadius = 1.f;
	float m_outerRadius = 2.f;
	Vec3 m_spotForward;
	float m_penumbraInnerDotThreshold = -1.f;
	float m_penumbraOuterDotThreshold = -2.f;
	Vec2 Padding;
};

struct PerFrameConstants
{
	float m_time = 0.f;
	int m_debugInt = 0;
	float m_debugFloat = 0.f;
	float Padding = 0.f;
};

static const int s_perFrameConstantsSlot = 1;

struct CameraConstants
{
	Mat44 m_worldToCameraTransform;
	Mat44 m_cameraToRenderTransform;
	Mat44 m_renderToClipTransform;
	Vec3 m_cameraPosition;
	float Padding = 0.f;
};

static const int s_cameraConstantsSlot = 2;

struct ModelConstants
{
	Mat44 m_modelToWorldTransform;
	float m_modelColor[4];
};

static const int s_modelConstantsSlot = 3;

struct LightConstants
{
	LightConstants() {};
	explicit LightConstants(Vec3 const& sunDirection, float sunIntensity, float ambientIntensity, Rgba8 const& sunColorRGB);
	explicit LightConstants(Vec3 const& sunDirection, float sunIntensity, float ambientIntensity, std::vector<LightData> lights, Rgba8 const& sunColorRGB);

	Vec3 m_sunDirection;
	float m_sunIntensity = 1.f;
	float m_sunColorRGB[3] = {1.f, 1.f, 1.f};
	float m_ambientIntensity = 0.5f;
	int m_numLights = 0;
	float Padding[3] = {};
	LightData m_lights[MAX_NUM_LIGHTS];
};

static const int s_lightConstantsSlot = 4;

constexpr int NUM_TEXTURE_SLOTS = 16;

struct ColorAdjustmentConstants
{
	float m_saturation = 1.f;
	float m_brightness = 0.f;
	float m_inversion = 0.f;
	float m_hueStrength = 0.f;
	float m_hueColor[4] = {1.f, 1.f, 1.f, 1.f};

};

static const int s_colorAdjustmentConstantsSlot = 8;

struct RenderTarget
{
#ifdef RENDERER_DX12
	TextureDX12 const* m_color = nullptr;
	TextureDX12 const* m_depth = nullptr;
	TextureDX12 const* m_mask = nullptr;
#else
	Texture const* m_color = nullptr;
	Texture const* m_depth = nullptr;
#endif // RENDERER_DX12

};

constexpr int NUM_COPY_RENDER_TARGETS = 50;

class Renderer
{
public:
	Renderer(RendererConfig const& config);
	~Renderer() {};

	virtual void Startup() = 0;
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual void Shutdown() = 0;

	virtual void ClearScreen(const Rgba8& clearColor) = 0;
	virtual void ClearDepth() = 0;
	virtual void BeginCamera(const Camera& camera) = 0;
	virtual void EndCamera(const Camera& camera) = 0;

	virtual void BeginRendererEvent(char const* eventName) const = 0;
	virtual void EndRendererEvent() const = 0;

	//Creation

	virtual BitmapFont* CreateOrGetBitMapFontFromFile(char const* bitmapFontFilePathWithNoExtension) = 0;

	//Binds
	void SetBlendMode(BlendMode blendMode);
	void SetSamplerMode(SamplerMode samplerMode, int slot = 0);
	void SetRasterizerMode(RasterizerMode rasterizerMode);
	void SetDepthMode(DepthMode depthMode);

	virtual void SetModelConstants(Mat44 const& modelToWorldTransform = Mat44(), Rgba8 const& modelColor = Rgba8::WHITE) = 0;
	virtual void SetLightConstants(Vec3 const& sunDirection, float sunIntensity, float ambientIntensity, Rgba8 const& sunColor = Rgba8::WHITE) = 0;
	virtual void SetLightConstants(LightConstants const& lightConstants) = 0;
	virtual void SetColorAdjustmentConstants(ColorAdjustmentConstants const& colorAdjustmentConstants) = 0;
	virtual void SetPerFrameConstants(PerFrameConstants const& perFrameConstants) = 0;

	static std::string GetNameForBlendMode(BlendMode const& blendMode);
	static std::string GetNameForDepthMode(DepthMode const& depthMode);
	static std::string GetNameForRasterizerMode(RasterizerMode const& rasterizerMode);

	static BlendMode GetBlendModeFromName(std::string const& name);

	StaticMesh*		CreateOrGetStaticMeshFromFile(std::string const& filePath);
	StaticMesh*		CreateOrGetStaticMeshFromVerts(Verts const& verts, StaticMeshConfig const& config);
	StaticMesh*		CreateOrGetStaticMeshFromVertTBNs(VertTBNs const& verts, IndexList const& indexes, StaticMeshConfig const& config);

protected:
	BitmapFont* GetBitMapFontForFileName(char const* bitmapFontFilePathWithNoExtension) const;


private:

protected:
	RendererConfig m_config;
	std::vector<BitmapFont*> m_loadedFonts;

	BlendMode		m_desiredBlendMode = BlendMode::ALPHA;
	SamplerMode		m_desiredSamplerModeBySlot[NUM_TEXTURE_SLOTS] = {};
	RasterizerMode	m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	DepthMode		m_desiredDepthMode = DepthMode::READ_WRITE_LESS_EQUAL;

	RenderTarget m_renderTargetCopies[NUM_COPY_RENDER_TARGETS] = {};
	int m_currentRenderTargetCopyIndex = 0;

	RenderTarget m_renderTarget = {};
	bool m_activeRenderTarget = true;

	std::vector<StaticMesh*> m_loadedStaticMeshes;


#if defined(ENGINE_DEBUG_RENDERER)
	void* m_dxgiDebug = nullptr;
	void* m_dxgiDebugModule = nullptr;
#endif

};