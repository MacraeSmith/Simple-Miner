#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Time.hpp"
#include <vector>
#include <queue>
#include <mutex>


#ifdef RENDERER_DX12
#include "Engine/Renderer/RendererDX12.hpp"
#else
#include "Engine/Renderer/RendererDX11.hpp"
#endif 


DebugRenderConfig s_debugRenderConfig;

struct DebugObject
{
	Verts m_verts;
	Rgba8 m_startColor;
	Rgba8 m_endColor;
	DebugRenderMode m_renderMode = DebugRenderMode::USE_DEPTH;
	RasterizerMode m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	float m_duration = 0.f;
	Timer m_timer;
	Mat44 m_transform;
	bool m_billboardToCamera = false;
	BillboardType m_billboardType = BillboardType::WORLD_UP_OPPOSING;
	bool m_worldText = false;
};

struct DebugScreenObject
{
	std::string m_text;
	Verts m_verts;
	Rgba8 m_startColor;
	Rgba8 m_endColor;
	float m_duration;
	Timer m_timer;
	int m_numInstances = 0;
};

std::vector<DebugObject> s_debugObjects;

std::vector<DebugScreenObject> s_debugScreenObjects;
std::vector<DebugScreenObject> s_messageObjects;
bool s_isVisible = true;
bool s_stackMessages = true;
static std::mutex s_debugRenderMutex;

void DebugRenderSystemStartup(DebugRenderConfig const& config)
{
	s_debugRenderConfig = config;

	g_eventSystem->SubscribeEventCallbackFunction("DebugRenderClear", Command_DebugRenderClear);
	g_eventSystem->SubscribeEventCallbackFunction("DebugRenderToggle", Command_DegbugRenderToggle);

	Strings arguments;
	arguments.push_back("True");
	arguments.push_back("False");
	g_eventSystem->SubscribeEventCallbackFunction("DebugStackMessages", arguments, Command_DebugStackMessages);
}

void DebugRenderSystemShutdown()
{
	s_debugObjects.clear();
	s_debugScreenObjects.clear();
	s_messageObjects.clear();
}

void DebugRenderSetVisible()
{
	s_isVisible = true;
}

void DebugRenderSetHidden()
{
	s_isVisible = false;
}


void DebugRenderClear()
{
	std::scoped_lock lock(s_debugRenderMutex);
	s_debugObjects.clear();
	s_debugScreenObjects.clear();
	s_messageObjects.clear();
}

void DebugRenderStackMessages(bool stack)
{
	std::scoped_lock lock(s_debugRenderMutex);
	s_stackMessages = stack;
}

void DebugRenderBeginFrame()
{
	{
		std::scoped_lock lock(s_debugRenderMutex);
		for (int objectNum = 0; objectNum < (int)s_debugObjects.size(); ++objectNum)
		{
			DebugObject& object = s_debugObjects[objectNum];

			if (object.m_timer.Tick() || object.m_duration == 0.f)
			{
				s_debugObjects.erase(s_debugObjects.begin() + objectNum);
				objectNum--;
			}
		}
	}

	{
		std::scoped_lock lock(s_debugRenderMutex);
		for (int screenObjectNum = 0; screenObjectNum < (int)s_debugScreenObjects.size(); ++screenObjectNum)
		{
			DebugScreenObject& screenObject = s_debugScreenObjects[screenObjectNum];

			if (screenObject.m_timer.Tick() || screenObject.m_duration == 0.f)
			{
				s_debugScreenObjects.erase(s_debugScreenObjects.begin() + screenObjectNum);
				screenObjectNum--;
			}
		}
	}

	
	{
		std::scoped_lock lock(s_debugRenderMutex);
		for (int messageObjectNum = 0; messageObjectNum < (int)s_messageObjects.size(); ++messageObjectNum)
		{
			DebugScreenObject& messageObject = s_messageObjects[messageObjectNum];
			if (messageObject.m_timer.Tick() || messageObject.m_duration == 0.f)
			{
				s_messageObjects.erase(s_messageObjects.begin() + messageObjectNum);
				messageObjectNum--;
			}
		}
	}
	

}

void DebugRenderEndFrame()
{
}


void DebugRenderWorld(Camera const& camera)
{
	std::scoped_lock lock(s_debugRenderMutex);
	if (!s_isVisible)
		return;

#ifdef RENDERER_DX12
	RendererDX12* renderer = dynamic_cast<RendererDX12*>(s_debugRenderConfig.m_renderer);
	GUARANTEE_OR_DIE(renderer, "trying to render with incorrect renderer");
	renderer->BindRootSignature(nullptr);
#else
	RendererDX11* renderer = dynamic_cast<RendererDX11*>(s_debugRenderConfig.m_renderer);
	GUARANTEE_OR_DIE(renderer, "trying to render with incorrect renderer");
#endif

	renderer->BeginCamera(camera);
	renderer->BeginRendererEvent("DRAW - Debug World Objects");
	

	for (int objectNum = 0; objectNum < (int)s_debugObjects.size(); ++objectNum)
	{
		DebugObject& object = s_debugObjects[objectNum];
		Rgba8 color = Rgba8::ColorLerp(object.m_startColor, object.m_endColor, object.m_timer.GetElapsedFraction());
		Rgba8 transparentColor(color.r, color.g, color.b, 100);
		Mat44 transform = object.m_transform;

		switch (object.m_renderMode)
		{
		case DebugRenderMode::ALWAYS:

			renderer->SetDepthMode(DepthMode::DISABLED);
			renderer->SetBlendMode(BlendMode::ALPHA);
			break;
		case DebugRenderMode::USE_DEPTH:
			renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
			renderer->SetBlendMode(BlendMode::ALPHA);
			break;

		case DebugRenderMode::X_RAY:
			//Transparent pass
			if (object.m_billboardToCamera)
			{
				Mat44 cameraTransform;
				cameraTransform.MakeTranslation3D(camera.GetPosition());
				Mat44 rotation = camera.GetOrientation().GetAsMatrix_IFwd_JLeft_KUp();
				cameraTransform.Append(rotation);
				Vec3 objectPos = object.m_transform.GetTranslation3D();
				transform = GetBillboardTransform(object.m_billboardType, cameraTransform, objectPos);
			}

			renderer->SetDepthMode(DepthMode::READ_ONLY_ALWAYS);
			renderer->SetBlendMode(BlendMode::ALPHA);
			renderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
			renderer->SetRasterizerMode(object.m_rasterizerMode);
			renderer->SetModelConstants(transform, transparentColor);
			renderer->BindShader(nullptr);
			renderer->BindTexture(nullptr);
			renderer->DrawVertexArray(object.m_verts);

			renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
			renderer->SetBlendMode(BlendMode::OPAQUE);
			break;
		default:
			break;
		}

		if (object.m_billboardToCamera)
		{
			Mat44 cameraTransform = Mat44::MakeTranslation3D(camera.GetPosition());
			Mat44 rotation = camera.GetOrientation().GetAsMatrix_IFwd_JLeft_KUp();
			cameraTransform.Append(rotation);
			Vec3 objectPos = object.m_transform.GetTranslation3D();
			transform = GetBillboardTransform(object.m_billboardType, cameraTransform, objectPos);
		}
		renderer->SetRasterizerMode(object.m_rasterizerMode);
		renderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
		renderer->SetModelConstants(transform, color);
		renderer->BindTexture(nullptr);
		if (object.m_worldText)
		{
			BitmapFont* font = renderer->CreateOrGetBitMapFontFromFile(s_debugRenderConfig.m_fontName.c_str());
			renderer->BindTexture(&font->GetTexture());
			renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		}
		renderer->BindShader(nullptr);
		renderer->DrawVertexArray(object.m_verts);
	}

	renderer->EndRendererEvent();
	renderer->EndCamera(camera);
}

void DebugRenderScreen(Camera const& camera)
{
	std::scoped_lock lock(s_debugRenderMutex);
	if (!s_isVisible)
		return;

#ifdef RENDERER_DX12
	RendererDX12* renderer = dynamic_cast<RendererDX12*>(s_debugRenderConfig.m_renderer);
	GUARANTEE_OR_DIE(renderer, "trying to render with incorrect renderer");
	renderer->BindRootSignature(nullptr);
#else
	RendererDX11* renderer = dynamic_cast<RendererDX11*>(s_debugRenderConfig.m_renderer);
	GUARANTEE_OR_DIE(renderer, "trying to render with incorrect renderer");
#endif

	renderer->BeginCamera(camera);
	renderer->BeginRendererEvent("DRAW - Debug Screen Objects");
	BitmapFont* font = renderer->CreateOrGetBitMapFontFromFile(s_debugRenderConfig.m_fontName.c_str());

	for (int textNum = 0; textNum < (int)s_debugScreenObjects.size(); ++textNum)
	{
		DebugScreenObject& textObject = s_debugScreenObjects[textNum];
		Rgba8 color = textObject.m_startColor;
		if (textObject.m_duration > 0.f)
		{
			color = Rgba8::ColorLerp(textObject.m_startColor, textObject.m_endColor, textObject.m_timer.GetElapsedFraction());
		}

		renderer->SetBlendMode(BlendMode::ALPHA);
		renderer->SetDepthMode(DepthMode::DISABLED);
		renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		renderer->SetModelConstants(Mat44::IDENTITY, color);
		renderer->BindShader(nullptr);
		renderer->BindTexture(&font->GetTexture());
		renderer->DrawVertexArray(textObject.m_verts);
	}


	AABB2 cameraBounds = AABB2(camera.GetOrthoBottomLeft(), camera.GetOrthoTopRight());
	float lineHeight = 0.02f * (cameraBounds.m_maxs.y - cameraBounds.m_mins.y);
	float lineWidth = 0.2f * (cameraBounds.m_maxs.x - cameraBounds.m_mins.x);
	bool activeDevConsole = g_devConsole && g_devConsole->GetMode() != HIDDEN;
	float bottomOfLines = cameraBounds.m_maxs.y;

	Verts messageVerts;
	for (int messageNum = 0; messageNum < (int)s_messageObjects.size(); ++messageNum)
	{
		//work backwards to first add infinite messages and then most recent message added from top to bottom
		DebugScreenObject& messageObject = s_messageObjects[((int)s_messageObjects.size() - 1) - messageNum];

		Rgba8 color = messageObject.m_startColor;
		if (messageObject.m_duration > 0.f)
		{
			color = Rgba8::ColorLerp(messageObject.m_startColor, messageObject.m_endColor, messageObject.m_timer.GetElapsedFraction());
		}

		std::string text;
		if (messageObject.m_numInstances > 1 && s_stackMessages)
		{
			text = Stringf("%s (%i)", messageObject.m_text.c_str(), messageObject.m_numInstances);
		}

		else
		{
			text = messageObject.m_text;
		}

		AABB2 lineBounds = AABB2(cameraBounds.m_mins.x, cameraBounds.m_maxs.y - ((messageNum + 1) * lineHeight),
			cameraBounds.m_mins.x + lineWidth, cameraBounds.m_maxs.y - (messageNum * lineHeight));
		font->AddVertsForTextInBox2D(messageVerts, text, lineBounds, lineHeight, color, 1.f, Vec2(0.f, 0.5f), SHRINK_TO_FIT);

		if (activeDevConsole)
		{
			bottomOfLines = lineBounds.m_mins.y;
		}
	}

	renderer->SetBlendMode(BlendMode::ALPHA);
	renderer->SetDepthMode(DepthMode::DISABLED);
	renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	renderer->SetModelConstants(Mat44::IDENTITY, Rgba8::WHITE);
	renderer->BindShader(nullptr);

	//Draw background for messages if dev console is active
	if (activeDevConsole && s_messageObjects.size() > 0)
	{
		Verts backGroundVerts;
		AABB2 backGroundBounds(cameraBounds.m_mins.x, bottomOfLines, cameraBounds.m_mins.x + lineWidth, cameraBounds.m_maxs.y);
		AddVertsForAABB2D(backGroundVerts, backGroundBounds, Rgba8::GREY);

		renderer->BindTexture(nullptr);
		renderer->DrawVertexArray(backGroundVerts);
	}

	renderer->BindTexture(&font->GetTexture());
	renderer->DrawVertexArray(messageVerts);

	renderer->EndRendererEvent();
	renderer->EndCamera(camera);

}

void DebugAddWorldPoint(Vec3 const& pos, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
	std::scoped_lock lock(s_debugRenderMutex);
	Verts verts;
	AddVertsForUVSphereZ3D(verts, pos, radius);

	DebugObject object;
	object.m_verts = verts;
	object.m_duration = duration;
	object.m_startColor = startColor;
	object.m_endColor = endColor;
	object.m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;

	bool startTimer = duration <= 0.f ? false : true;
	object.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);
	object.m_renderMode = mode;

	s_debugObjects.push_back(object);
}

void DebugAddWorldMarker(Vec3 const& pos, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
	std::scoped_lock lock(s_debugRenderMutex);
	Verts verts;
	AddVertsForUVSphereZ3D(verts, pos, radius, 3, 2);

	DebugObject object;
	object.m_verts = verts;
	object.m_duration = duration;
	object.m_startColor = startColor;
	object.m_endColor = endColor;
	object.m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;

	bool startTimer = duration <= 0.f ? false : true;
	object.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);
	object.m_renderMode = mode;

	s_debugObjects.push_back(object);
}

void DebugAddWorldLine(Vec3 const& start, Vec3 const& end, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
	std::scoped_lock lock(s_debugRenderMutex);
	Verts verts;
	AddVertsForCylinder3D(verts, start, end, radius);

	DebugObject object;
	object.m_verts = verts;
	object.m_duration = duration;
	object.m_startColor = startColor;
	object.m_endColor = endColor;

	bool startTimer = duration <= 0.f ? false : true;
	object.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);
	object.m_renderMode = mode;

	s_debugObjects.push_back(object);
}

void DebugAddWireCylinder(Vec3 const& base, Vec3 const& top, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
	std::scoped_lock lock(s_debugRenderMutex);
	Verts verts;
	AddVertsForCylinder3D(verts, base, top, radius);

	DebugObject object;
	object.m_verts = verts;
	object.m_duration = duration;
	object.m_startColor = startColor;
	object.m_endColor = endColor;
	object.m_rasterizerMode = RasterizerMode::WIREFRAME_CULL_BACK;

	bool startTimer = duration <= 0.f ? false : true;
	object.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);
	object.m_renderMode = mode;

	s_debugObjects.push_back(object);
}

void DebugAddWorldWireSphere(Vec3 const& center, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
	std::scoped_lock lock(s_debugRenderMutex);
	Verts verts;
	AddVertsForUVSphereZ3D(verts, center, radius);

	DebugObject object;
	object.m_verts = verts;
	object.m_duration = duration;
	object.m_startColor = startColor;
	object.m_endColor = endColor;
	object.m_rasterizerMode = RasterizerMode::WIREFRAME_CULL_BACK;

	bool startTimer = duration <= 0.f ? false : true;
	object.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);
	object.m_renderMode = mode;

	s_debugObjects.push_back(object);
}

void DebugAddWorldArrow(Vec3 const& start, Vec3 const& end, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
	std::scoped_lock lock(s_debugRenderMutex);
	Vec3 direction = end - start;
	float length = direction.GetLength();
	Verts verts;
	AddVertsForArrow3D(verts, start, direction, length, radius);

	DebugObject object;
	object.m_verts = verts;
	object.m_duration = duration;
	object.m_startColor = startColor;
	object.m_endColor = endColor;

	bool startTimer = duration <= 0.f ? false : true;
	object.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);
	object.m_renderMode = mode;

	s_debugObjects.push_back(object);
}

void DebugAddWorldText(std::string const& text, Mat44 const& transform, float textHeight, Vec2 const& alignment, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
	std::scoped_lock lock(s_debugRenderMutex);
	Verts verts;
	BitmapFont* font = s_debugRenderConfig.m_renderer->CreateOrGetBitMapFontFromFile(s_debugRenderConfig.m_fontName.c_str());
	font->AddVertsForText3DAtOriginXForward(verts, textHeight, text, Rgba8::WHITE, 1.f, alignment);

	DebugObject object;
	object.m_verts = verts;
	object.m_duration = duration;
	object.m_worldText = true;
	object.m_startColor = startColor;
	object.m_endColor = endColor;
	object.m_renderMode = mode;
	object.m_transform = transform;
	object.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;

	bool startTimer = duration <= 0.f ? false : true;
	object.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);

	s_debugObjects.push_back(object);

}

void DebugAddWorldBillboardText(std::string const& text, Mat44 const& transform, float textHeight, Vec2 const& alignment, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
	std::scoped_lock lock(s_debugRenderMutex);
	Verts verts;
	BitmapFont* font = s_debugRenderConfig.m_renderer->CreateOrGetBitMapFontFromFile(s_debugRenderConfig.m_fontName.c_str());
	font->AddVertsForText3DAtOriginXForward(verts, textHeight, text, Rgba8::WHITE, 1.f, alignment);

	DebugObject object;
	object.m_verts = verts;
	object.m_duration = duration;
	object.m_billboardToCamera = true;
	object.m_worldText = true;
	object.m_startColor = startColor;
	object.m_endColor = endColor;
	object.m_renderMode = mode;
	object.m_billboardType = BillboardType::WORLD_UP_OPPOSING;
	object.m_transform = transform;

	bool startTimer = duration <= 0.f ? false : true;
	object.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);

	s_debugObjects.push_back(object);
}

void DebugAddWorldBasis(const Mat44& transform, float duration, DebugRenderMode mode)
{
	std::scoped_lock lock(s_debugRenderMutex);
	Vec3 start = transform.GetTranslation3D();
	Vec3 fwrd = transform.GetIBasis3D();
	Vec3 left = transform.GetJBasis3D();
	Vec3 up = transform.GetKBasis3D();

	Verts verts;
	AddVertsForArrow3D(verts, start, fwrd, 1.f, 0.075f, 16, Rgba8::RED);
	AddVertsForArrow3D(verts, start, left, 1.f, 0.075f, 16, Rgba8::GREEN);
	AddVertsForArrow3D(verts, start, up, 1.f, 0.075f, 16, Rgba8::BLUE);

	DebugObject object;
	object.m_verts = verts;
	object.m_duration = duration;

	bool startTimer = duration <= 0.f ? false : true;
	object.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);
	object.m_renderMode = mode;

	s_debugObjects.push_back(object);
}

void DebugAddWorldQuad(Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
	std::scoped_lock lock(s_debugRenderMutex);
	Verts verts;
	AddVertsForQuad3D(verts, bottomLeft, bottomRight, topRight, topLeft);

	DebugObject object;
	object.m_verts = verts;
	object.m_duration = duration;
	object.m_startColor = startColor;
	object.m_endColor = endColor;
	object.m_renderMode = mode;
	object.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;

	bool startTimer = duration <= 0.f ? false : true;
	object.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);

	s_debugObjects.push_back(object);
}

void DebugAddWorldWireFrameQuad(Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, float lineThickness, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
	std::scoped_lock lock(s_debugRenderMutex);
	Verts verts;
	AddVertsForWireFrameQuad3D(verts, bottomLeft, bottomRight, topRight, topLeft, lineThickness);

	DebugObject object;
	object.m_verts = verts;
	object.m_duration = duration;
	object.m_startColor = startColor;
	object.m_endColor = endColor;
	object.m_renderMode = mode;
	object.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;

	bool startTimer = duration <= 0.f ? false : true;
	object.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);

	s_debugObjects.push_back(object);
}

void DebugAddScreenText(std::string const& text, AABB2 const& bounds, float cellHeight, Vec2 const& alignment, float duration, Rgba8 const& startColor, Rgba8 const& endColor)
{
	std::scoped_lock lock(s_debugRenderMutex);
	Verts verts;
	BitmapFont* font = s_debugRenderConfig.m_renderer->CreateOrGetBitMapFontFromFile(s_debugRenderConfig.m_fontName.c_str());
	font->AddVertsForTextInBox2D(verts, text, bounds, cellHeight, Rgba8::WHITE, 1.f, alignment, SHRINK_TO_FIT);

	DebugScreenObject screenObject;
	screenObject.m_duration = duration;
	screenObject.m_text = text;
	screenObject.m_verts = verts;
	screenObject.m_startColor = startColor;
	screenObject.m_endColor = endColor;

	bool startTimer = duration <= 0.f ? false : true;
	screenObject.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);

	s_debugScreenObjects.push_back(screenObject);
}

void DebugAddMessage(std::string const& text, float duration, Rgba8 const& startColor, Rgba8 const& endColor)
{
	std::scoped_lock lock(s_debugRenderMutex);
	if (s_stackMessages)
	{
		for (DebugScreenObject& messageObject : s_messageObjects)
		{
			if (messageObject.m_text == text)
			{
				messageObject.m_duration = duration;
				messageObject.m_numInstances++;
				messageObject.m_startColor = startColor;
				messageObject.m_endColor = endColor;
				//#TODO: figure out why frame order of timer freezes game when I try to get elapsed fraction in render screen
				//messageObject.m_timer.SetPeriod(duration);
				messageObject.m_timer.Restart();
				return;
			}
		}
	}

	DebugScreenObject screenObject;
	screenObject.m_duration = duration;
	screenObject.m_text = text;
	screenObject.m_startColor = startColor;
	screenObject.m_endColor = endColor;
	screenObject.m_numInstances = 1;

	bool startTimer = duration <= 0.f ? false : true;
	screenObject.m_timer = Timer(duration, &Clock::GetSystemClock(), startTimer);


	if (duration < 0.f)
	{
		s_messageObjects.push_back(screenObject);
		return;
	}

	//insert message right before infinite messages
	int insertionIndex = 0;
	for (int i = 0; i < s_messageObjects.size(); ++i)
	{
		if (s_messageObjects[i].m_duration < 0.f)
			break;

		insertionIndex++;
	}

	s_messageObjects.insert(s_messageObjects.begin() + insertionIndex, screenObject);
	
}

bool Command_DebugRenderClear(EventArgs& args)
{
	UNUSED(args);
	DebugRenderClear();
	return true;
}

bool Command_DegbugRenderToggle(EventArgs& args)
{
	std::scoped_lock lock(s_debugRenderMutex);
	UNUSED(args);
	s_isVisible = !s_isVisible;
	if (s_isVisible)
	{
		DebugRenderSetVisible();
		return true;
	}

	DebugRenderSetHidden();
	return true;
}

bool Command_DebugStackMessages(EventArgs& args)
{
	if (args.HasKey("True"))
	{
		DebugRenderStackMessages(true);
		return true;
	}

	if (args.HasKey("False"))
	{
		DebugRenderStackMessages(false);
		return true;
	}

	DebugRenderStackMessages(true);
	return true;
}

DebugTimeProfiler::DebugTimeProfiler(std::string message, float timeOnScreen, Rgba8 const& color, bool useMilliseconds)
	:m_message(message)
	,m_timeOnScreen(timeOnScreen)
	,m_color(color)
	,m_useMilliseconds(useMilliseconds)
{
	m_startTime = GetCurrentTimeSeconds();
}

DebugTimeProfiler::~DebugTimeProfiler()
{
	double endTime = GetCurrentTimeSeconds();
	double totalTime = endTime - m_startTime;
	std::string appendSymbol = "sec";
	if (m_useMilliseconds)
	{
		totalTime *= 1000.0;
		appendSymbol = "ms";
	}

	DebugAddMessage(Stringf("%s: %.2f %s", m_message.c_str(), totalTime, appendSymbol.c_str()), m_timeOnScreen, m_color, m_color);

}
