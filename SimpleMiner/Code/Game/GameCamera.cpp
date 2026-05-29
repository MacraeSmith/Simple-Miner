#include "Game/GameCamera.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/Entity.hpp"
#include "Game/World.hpp"

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/RendererDX11.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"

GameCamera::GameCamera(Entity* owningEntity)
	:m_owningEntity(owningEntity)
{
	CreateCompass();
	SetCameraToRenderTransform(Mat44::IFWRD_JLEFT_KUP_TO_DX11RENDER);
	Vec3 pos = m_owningEntity->m_position;
	pos.z += PLAYER_EYE_HEIGHT;
	SetPositionAndOrientation(pos, m_owningEntity->m_orientation);
}

GameCamera::~GameCamera()
{
	delete(m_compassVbo);
	m_compassVbo = nullptr;
}

void GameCamera::Update()
{
	if (!IsFreeFly() && m_cameraMode != CameraMode::INDEPENDENT)
	{
		Vec3 position = m_owningEntity->m_position;
		position.z += PLAYER_EYE_HEIGHT;
		EulerAngles orientation = m_owningEntity->m_orientation;
		if (m_cameraMode == CameraMode::OVER_SHOULDER)
		{
			Vec3 eyePosition = position;
			Vec3 playerFwrd = m_owningEntity->GetForwardNormal();
			float camDistance = GetCameraClipDistance(eyePosition, -playerFwrd);

			position = eyePosition + (-playerFwrd * camDistance);

		}

		SetPositionAndOrientation(position, orientation);
	}

	UpdateCompass();

	//Set perspective mode
	IntVec2 windowDimensions = Window::s_mainWindow->GetClientDimensions();
	float aspect = (float)windowDimensions.x / (float)windowDimensions.y;
	SetPerspectiveView(aspect, 60.f, 0.01f, 1000.f);
}

void GameCamera::UpdateInput()
{
	if(!IsFreeFly())
		return;

	float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();

	//Rotation
	int xDirection = g_invertMouseX ? -1 : 1;
	int yDirection = g_invertMouseY ? 1 : -1;
	Vec2 mouseDelta = g_inputSystem->GetCursorClientDelta();
	EulerAngles orientation = m_orientation;

	orientation.m_yawDegrees += mouseDelta.x * g_mouseSensitivity * yDirection;
	orientation.m_pitchDegrees = GetClamped(orientation.m_pitchDegrees + (mouseDelta.y * g_mouseSensitivity * xDirection), -85.f, 85.f);

	//Movement
	Vec3 translation;
	Vec3 fwrd;
	Vec3 left;
	Vec3 up;
	orientation.GetAsVectors_IFwd_JLeft_KUp(fwrd, left, up);

	if (m_cameraMode == CameraMode::SPECTATOR_XY)
	{
		fwrd.z = 0.f;
		fwrd.Normalize();
		left.z = 0.f; left.Normalize();
		up = Vec3::UP;
	}


	if (g_inputSystem->IsKeyDown('W'))
	{
		translation += fwrd * (m_freeFlyVelocity * deltaSeconds);
	}

	if ((g_inputSystem->IsKeyDown('S')))
	{
		translation -= fwrd * (m_freeFlyVelocity * deltaSeconds);
	}

	if ((g_inputSystem->IsKeyDown('A')))
	{
		translation += left * (m_freeFlyVelocity * deltaSeconds);
	}

	if ((g_inputSystem->IsKeyDown('D')))
	{
		translation -= left * (m_freeFlyVelocity * deltaSeconds);
	}

	if ((g_inputSystem->IsKeyDown('E')))
	{
		translation += up * (m_freeFlyVelocity * deltaSeconds);
	}

	if ((g_inputSystem->IsKeyDown('Q')))
	{
		translation -= up * (m_freeFlyVelocity * deltaSeconds);
	}

	if (g_inputSystem->IsKeyDown(KEYCODE_SHIFT))
	{
		if (g_inputSystem->GetWheelDelta() > 0.f)
		{
			m_freeFlySprintSpeed += 100.f * deltaSeconds;
		}

		else if (g_inputSystem->GetWheelDelta() < 0.f)
		{
			m_freeFlySprintSpeed -= 100.f * deltaSeconds;
		}
		m_freeFlySprintSpeed = GetClamped(m_freeFlySprintSpeed, 10.f, 100.f);
		DebugAddMessage(Stringf("Sprint Speed: %.2f", m_freeFlySprintSpeed), 0.f, Rgba8::GREEN);
		translation *= m_freeFlySprintSpeed;
	}

	
	m_position += translation;
	m_orientation = orientation;
}

void GameCamera::UpdateCompass()
{
	Vec3 camForward = m_orientation.Get_IFwd();
	m_compassPos = m_position + (camForward * 0.25f);
}

void GameCamera::RenderCompass() const
{
	//if(m_cameraMode == CameraMode::FIRST_PERSON || m_cameraMode == CameraMode::OVER_SHOULDER)
		//return;

	g_renderer->BeginRendererEvent("Draw - Compass");
	g_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_renderer->SetBlendMode(BlendMode::OPAQUE);
	g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_renderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
	Mat44 compassTransform = Mat44::MakeTranslation3D(m_compassPos);
	g_renderer->SetModelConstants(compassTransform);
	g_renderer->BindTexture(nullptr);
	g_renderer->BindShader(nullptr);
	g_renderer->DrawVertexBuffer(m_compassVbo, m_compassVbo->GetCount());
	g_renderer->EndRendererEvent();
}

void GameCamera::SwitchCameraMode()
{
	int mode = (int)m_cameraMode;
	mode++;
	if (mode >= (int)CameraMode::COUNT)
	{
		mode = 0;
	}
	m_cameraMode = (CameraMode)mode;
	DebugAddMessage(Stringf("Camera Mode [c]: %s", GetCameraModeString().c_str()), 5.f, Rgba8::GREEN, Rgba8::GREEN);

	if(m_cameraMode == CameraMode::FIRST_PERSON)
		m_owningEntity->Teleport(m_position);
}

std::string GameCamera::GetCameraModeString() const
{
	switch (m_cameraMode)
	{
	case CameraMode::FIRST_PERSON: return "First Person";
	case CameraMode::OVER_SHOULDER: return "Over Shoulder";
	case CameraMode::SPECTATOR: return "Spectator";
	case CameraMode::SPECTATOR_XY: return "Spectator World Aligned";
	case CameraMode::INDEPENDENT: return "Independent";
	default: return "Invalid";
	}
}

Frustum GameCamera::GetViewFrustrum() const
{
	return GetPerspectiveFrustum();
}

bool GameCamera::IsFreeFly() const
{
	return m_cameraMode == CameraMode::SPECTATOR || m_cameraMode == CameraMode::SPECTATOR_XY;
}

void GameCamera::CreateCompass()
{
	Verts verts;
	AddVertsForArrow3D(verts, Vec3::ZERO, Vec3::FORWARD, .01f, 0.00075f, 8, Rgba8::RED);
	AddVertsForArrow3D(verts, Vec3::ZERO, Vec3::LEFT, .01f, 0.00075f, 8, Rgba8::GREEN);
	AddVertsForArrow3D(verts, Vec3::ZERO, Vec3::UP, .01f, 0.00075f, 8, Rgba8::BLUE);
	m_compassVbo = g_renderer->CreateVertexBuffer(sizeof(Vertex_PCU) * (int)verts.size(), sizeof(Vertex_PCU));
	g_renderer->CopyCPUToGPU(verts.data(), sizeof(Vertex_PCU) * (int)verts.size(), m_compassVbo);
}

float GameCamera::GetCameraClipDistance(Vec3 const& startPos, Vec3 const& raycastDirection)
{
	BlockRaycastResult3D result = g_game->m_currentWorld->RaycastVsWorld(startPos, raycastDirection, OVER_SHOULDER_CAM_DISTANCE);
	return result.m_impactDistance;
}
