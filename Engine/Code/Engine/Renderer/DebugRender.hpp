#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include <string>
class Renderer;
#ifdef RENDERER_DX12
class RendererDX12;
#else
class RendererDX11;
#endif // RENDERER_DX12

class Camera;
struct Vec2;
struct Vec3;
struct Mat44;
struct AABB2;

enum class DebugRenderMode
{
	ALWAYS,
	USE_DEPTH,
	X_RAY
};

struct DebugRenderConfig
{
	Renderer* m_renderer = nullptr;
	std::string m_fontName = "Data/Font/SquirrelFixedFont";
};

//Setup
void DebugRenderSystemStartup(DebugRenderConfig const& config);
void DebugRenderSystemShutdown();

//Control
void DebugRenderSetVisible();
void DebugRenderSetHidden();
void DebugRenderClear();
void DebugRenderStackMessages(bool stack);

//Output
void DebugRenderBeginFrame();
void DebugRenderWorld(Camera const& camera);
void DebugRenderScreen(Camera const& camera);
void DebugRenderEndFrame();

//Geometry
void DebugAddWorldPoint(Vec3 const& pos, float radius, float duration, 
	Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWorldMarker(Vec3 const& pos, float radius, float duration,
	Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWorldLine(Vec3 const& start, Vec3 const& end, float radius, float duration,
	Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWireCylinder(Vec3 const& base, Vec3 const& top, float radius, float duration,
	Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWorldWireSphere(Vec3 const& center, float radius, float duration,
	Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWorldArrow(Vec3 const& start, Vec3 const& end, float radius, float duration,
	Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWorldText(std::string const& text, Mat44 const& transform, float textHeight, Vec2 const& alignment, float duration,
	Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

//#TODO: maybe add billboard type as pass in?
void DebugAddWorldBillboardText(std::string const& text, Mat44 const& transform, float textHeight, Vec2 const& alignment, float duration,
	Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWorldBasis(const Mat44& transform, float duration, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWorldQuad(Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, float duration,
	Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWorldWireFrameQuad(Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, float lineThickness,
	float duration, Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

//Screen
void DebugAddScreenText(std::string const& text, AABB2 const& bounds, float cellHeight, Vec2 const& alignment, float duration,
	Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE);

void DebugAddMessage(std::string const& text, float duration,
	Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE);

bool Command_DebugRenderClear(EventArgs& args);
bool Command_DegbugRenderToggle(EventArgs& args);
bool Command_DebugStackMessages(EventArgs& args);

struct DebugTimeProfiler
{
	DebugTimeProfiler(std::string message = "Duration", float timeOnScreen = 5.f, Rgba8 const& color = Rgba8::WHITE, bool useMilliseconds = true);
	~DebugTimeProfiler();
private:
	std::string m_message;
	float m_timeOnScreen = 5.f;
	Rgba8 m_color;
	bool m_useMilliseconds = true;
	double m_startTime = 0.0;
};


