#include "Game/Player.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/World.hpp"
#include "Game/BlockDefinition.hpp"
#include "Game/WorldDefinition.hpp"
#include "Game/GameCamera.hpp"

#include "Engine/Math/SmoothNoise.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Input/XboxController.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/RendererDX11.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Core/Timer.hpp"


Player::Player(Game* owner)
	:Entity(owner, Vec3(-25.f, -50.f, 128.f), EulerAngles(153.f, 10.f, 0.f))
	,m_camera(new GameCamera(this))
	,m_jumpTimer(new Timer(0.25, &Clock::GetSystemClock()))
{

	m_groundDrag = PLAYER_XY_GROUND_DRAG;
	m_airDrag = PLAYER_XY_AIR_DRAG;
	m_runSpeed = PLAYER_XY_GROUND_SPEED;
	m_flySpeed = PLAYER_XY_AIR_SPEED;
	m_halfHeight = PLAYER_HALF_HEIGHT;
	m_fixedUpdate = true;
	m_phsyicsTimeStep = 0.005f;
	m_physicsTimer = new Timer(m_phsyicsTimeStep, &Clock::GetSystemClock());;


	m_physicsBounds = AABB3(-PLAYER_HALF_WIDTH, -PLAYER_HALF_WIDTH, 0.f, PLAYER_HALF_WIDTH, PLAYER_HALF_WIDTH, PLAYER_HEIGHT);

	Verts playerBoundsVerts;
	AddVertsForWireFrameAABB3D(playerBoundsVerts, m_physicsBounds, 0.01f, Rgba8::CYAN);
	m_playerVBO = g_renderer->CreateVertexBuffer(sizeof(Vertex_PCU) * (int)playerBoundsVerts.size(), sizeof(Vertex_PCU));
	g_renderer->CopyCPUToGPU(playerBoundsVerts.data(), sizeof(Vertex_PCU) * (int)playerBoundsVerts.size(), m_playerVBO);

	Vec3 boundsCenter = m_position;
	boundsCenter.z += PLAYER_HALF_HEIGHT;
	m_physicsBounds.SetCenter(boundsCenter);

}


Player::~Player()
{
	delete m_playerVBO;
	m_playerVBO = nullptr;

	delete m_jumpTimer;
	m_jumpTimer = nullptr;

	delete m_camera;
	m_camera = nullptr;
}

void Player::Update()
{
	UpdateIsSwimming();

	UpdateMovementInput();
	Entity::Update();
	m_camera->Update();

	if (!m_lockedRaycast )
	{
		m_raycastStart = m_position;
		m_raycastStart.z += PLAYER_EYE_HEIGHT;
		Vec3 playerFwrd = m_orientation.Get_IFwd();
		m_raycastForward = playerFwrd;
	}

	CheckWorldRaycast(m_raycastStart, m_raycastForward);

	UpdateBlockInput();
	UpdatePhysicsAndCameraModeInput();

	if (g_inputSystem->WasKeyJustPressed(KEYCODE_F7))
	{
		m_showInventory = !m_showInventory;
	}

}

void Player::UpdateMovementInput()
{

	if (m_camera->IsFreeFly())
	{
		m_camera->UpdateInput();
		return;
	}

	//Rotation
	int xDirection = g_invertMouseX ? -1 : 1;
	int yDirection = g_invertMouseY ? 1 : -1;
	Vec2 mouseDelta = g_inputSystem->GetCursorClientDelta();
	EulerAngles orientation = m_orientation;

	orientation.m_yawDegrees += mouseDelta.x * g_mouseSensitivity * yDirection;
	orientation.m_pitchDegrees = GetClamped(orientation.m_pitchDegrees + (mouseDelta.y * g_mouseSensitivity * xDirection), -85.f, 85.f);

	//Movement
	Vec3 moveDirection;
	Vec3 fwrd;
	Vec3 left;
	Vec3 up;
	orientation.GetAsVectors_IFwd_JLeft_KUp(fwrd, left, up);

	if (m_physicsMode == PhysicsMode::WALKING || m_physicsMode == PhysicsMode::SWIMMING)
	{
		fwrd.z = 0.f;
		left.z = 0.f;
		fwrd.Normalize();
		left.Normalize();

		up = Vec3::UP;

	}

	if (g_inputSystem->IsKeyDown('W'))
	{
		moveDirection += fwrd;
	}

	if ((g_inputSystem->IsKeyDown('S')))
	{
		moveDirection -= fwrd;
	}

	if ((g_inputSystem->IsKeyDown('A')))
	{
		moveDirection += left;
	}

	if ((g_inputSystem->IsKeyDown('D')))
	{
		moveDirection -= left;
	}

	if (m_physicsMode != PhysicsMode::WALKING && m_physicsMode != PhysicsMode::SWIMMING)
	{
		if ((g_inputSystem->IsKeyDown('E')))
		{
			moveDirection += up;
		}

		if ((g_inputSystem->IsKeyDown('Q')))
		{
			moveDirection -= up;
		}
	}

	if (g_inputSystem->IsKeyDown(' '))
	{
		if (m_physicsMode == PhysicsMode::WALKING)
		{
			if (IsGrounded() && m_jumpTimer->Tick())
			{
				AddImpulse(PLAYER_JUMP_IMPULSE * up);
				m_jumpTimer->Restart();
			}
		}

		else
		{
			moveDirection += up;
		}
	}

	m_isSprinting = false;

	if (g_inputSystem->IsKeyDown(KEYCODE_SHIFT))
	{
		m_isSprinting = true;
		float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();
		if (g_inputSystem->GetWheelDelta() > 0.f)
		{
			m_sprintMultiplier += 100.f * deltaSeconds;
		}

		else if (g_inputSystem->GetWheelDelta() < 0.f)
		{
			m_sprintMultiplier -= 100.f * deltaSeconds;
		}
		m_sprintMultiplier = GetClamped(m_sprintMultiplier, 1.f, 10.f);
	}


	MoveInDirection(moveDirection.GetNormalized());
	m_orientation = orientation;
}

void Player::UpdateBlockInput()
{

	{
		//Placing blocks
		if (g_inputSystem->WasKeyJustPressed('1'))
		{
			m_inventory.ChangeSelectedBlock(0);
			m_placedBlockType = 13; //glowstone
		}

		if (g_inputSystem->WasKeyJustPressed('2'))
		{
			m_inventory.ChangeSelectedBlock(1);
			m_placedBlockType = 14; //cobblestone
		}

		if (g_inputSystem->WasKeyJustPressed('3'))
		{
			m_inventory.ChangeSelectedBlock(2);
			m_placedBlockType = 15; //chiseled brick
		}

		if (g_inputSystem->WasKeyJustPressed('4'))
		{
			m_inventory.ChangeSelectedBlock(3);
			m_placedBlockType = 1; //water
		}

		if (g_inputSystem->WasKeyJustPressed('5'))
		{
			m_inventory.ChangeSelectedBlock(4);
			m_placedBlockType = 12; //lava
		}

		if (g_inputSystem->WasKeyJustPressed('6'))
		{
			m_inventory.ChangeSelectedBlock(5);
		}

		if (g_inputSystem->WasKeyJustPressed('7'))
		{
			m_inventory.ChangeSelectedBlock(6);
		}

		if (g_inputSystem->WasKeyJustPressed('8'))
		{
			m_inventory.ChangeSelectedBlock(7);
		}

		if (g_inputSystem->WasKeyJustPressed('9'))
		{
			m_inventory.ChangeSelectedBlock(8);
		}
	}

	if (g_inputSystem->WasKeyJustPressed(KEYCODE_LEFT_MOUSE_BUTTON) && m_selectedBlock)
	{
		uint8_t TNT_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Tnt");
		Block* block = m_selectedBlockIterator.GetBlock();
		if (block && block->m_blockType == TNT_TYPE)
		{
			Vec3 pos = Vec3(m_selectedBlockGlobalCoords) + Vec3(0.5f, 0.5f, 0.f);
			g_game->SpawnTNT(pos);
		}

		g_game->m_currentWorld->RemoveBlock(m_selectedBlockIterator);
	}

	if (g_inputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE_BUTTON) && m_selectedBlock)
	{
		BlockDirection blockDir = BlockDirection::UP;
		if (m_selectedNormal == Vec3::UP)
			blockDir = BlockDirection::UP;
		else if (m_selectedNormal == Vec3::DOWN)
			blockDir = BlockDirection::DOWN;
		else if (m_selectedNormal == Vec3::NORTH)
			blockDir = BlockDirection::NORTH;
		else if (m_selectedNormal == Vec3::EAST)
			blockDir = BlockDirection::EAST;
		else if (m_selectedNormal == Vec3::SOUTH)
			blockDir = BlockDirection::SOUTH;
		else if (m_selectedNormal == Vec3::WEST)
			blockDir = BlockDirection::WEST;

		BlockIterator blockIterator = m_selectedBlockIterator.GetIterator(blockDir);

		BlockIterator playerFeetIter = g_game->m_currentWorld->GetBlockIteratorAtPosition(m_position);
		Vec3 headPos = m_position;
		headPos.z += PLAYER_EYE_HEIGHT;
		BlockIterator playerHeadIter = g_game->m_currentWorld->GetBlockIteratorAtPosition(headPos);
		if (blockIterator.GetBlock() != playerFeetIter.GetBlock() && blockIterator.GetBlock() != playerHeadIter.GetBlock())
		{
			g_game->m_currentWorld->AddBlock(blockIterator, m_inventory.GetSelectedBlock());
		}
	}

	if (g_inputSystem->WasKeyJustPressed('R'))
	{
		m_lockedRaycast = !m_lockedRaycast;
	}
}

void Player::UpdatePhysicsAndCameraModeInput()
{
	if (g_inputSystem->WasKeyJustPressed('C'))
	{
		m_camera->SwitchCameraMode();
	}

	if (g_inputSystem->WasKeyJustPressed('V'))
	{
		SwitchPhysicsMode();
	}
}

void Player::UpdateIsSwimming()
{
	bool inWater = m_game->m_currentWorld->IsPositionWater(m_position);

	Vec3 eyePosition = m_position;
	eyePosition.z += PLAYER_EYE_HEIGHT;
	if(m_game->m_currentWorld->IsPositionWater(eyePosition))
		m_isUnderWater = true;
	else
		m_isUnderWater = false;

	if(inWater && m_physicsMode == PhysicsMode::WALKING)
		m_physicsMode = PhysicsMode::SWIMMING;

	else if (!inWater && m_physicsMode == PhysicsMode::SWIMMING)
	{
		AddImpulse(Vec3(0.f, 0.f, 1.f));
		m_physicsMode = PhysicsMode::WALKING;
	}


	
}

void Player::UpdateDebugMessages()
{
	AABB2 screenBounds = g_game->GetScreenBounds();
	AABB2 bounds = screenBounds;
	bounds = screenBounds;
	bounds.AddPadding(-.3f, -.9f, -.3f, -.0001f);
	std::string text = "Camera Mode [C]: " + m_camera->GetCameraModeString() + "\n" + "Physics Mode [V]: " + GetStringForPhysicsMode() + "\nDig [LMB], Place Block [RMB]" + "\nSelect Block [1-9]: " + BlockDefinition::GetBlockDefinitionFromIndex(m_placedBlockType)->m_name;
	DebugAddScreenText(text, bounds, 0.1f * g_SCREEN_SIZE_Y, Vec2(.5f, .95f), 0.f);
}


void Player::Render() const
{
	//Compass
	m_camera->RenderCompass();

	if (m_camera->m_cameraMode != CameraMode::FIRST_PERSON)
	{
		g_renderer->BeginRendererEvent("Draw - Player Bounds");
		g_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_renderer->SetBlendMode(BlendMode::OPAQUE);
		g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_renderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
		g_renderer->BindTexture(nullptr);
		g_renderer->BindShader(nullptr);

		Mat44 transform = Mat44::MakeTranslation3D(m_position);
		g_renderer->SetModelConstants(transform);
		g_renderer->DrawVertexBuffer(m_playerVBO, m_playerVBO->GetCount());
		g_renderer->EndRendererEvent();
	}
}

void Player::RenderInventory() const
{
	if (m_showInventory)
	{
		m_inventory.Render();
	}
}

Frustum Player::GetViewFrustrum() const
{
	return m_camera->GetViewFrustrum();
}

void Player::RestartJumpTimer()
{
	m_jumpTimer->Restart();
}


void Player::CheckWorldRaycast(Vec3 const& startPos, Vec3 const& fwrd)
{
	Vec3 hitPos = startPos + (fwrd * m_raycastLength);
	Rgba8 raycastColor = Rgba8::RED;
	BlockRaycastResult3D raycastResult = g_game->m_currentWorld->RaycastVsWorld(startPos, fwrd, m_raycastLength);
	if (raycastResult.m_didImpact)
	{
		hitPos = raycastResult.m_impactPos;
		raycastColor = Rgba8::CYAN;
		m_selectedBlockIterator = raycastResult.m_blockIterator;
		m_selectedBlock = true;
		m_selectedNormal = raycastResult.m_impactNormal;
		m_selectedBlockGlobalCoords = raycastResult.m_blockGlobalCoords;

		if (m_drawRaycastHit)
		{
			Vec3 blockPos = Vec3(raycastResult.m_blockGlobalCoords);
			Vec3 bl;
			Vec3 br;
			Vec3 tr;
			Vec3 tl;
			if (raycastResult.m_impactNormal == Vec3::UP)
			{
				bl = blockPos;
				bl.z += 1.f;

				br = bl;
				br.x += 1.f;

				tr = br;
				tr.y += 1.f;

				tl = bl;
				tl.y += 1.f;
			}

			else if (raycastResult.m_impactNormal == Vec3::DOWN)
			{
				bl = blockPos;
				bl.y += 1.f;

				br = bl;
				br.x += 1.f;

				tr = br;
				tr.y -= 1.f;

				tl = blockPos;
			}

			else if (raycastResult.m_impactNormal == Vec3::NORTH)
			{
				bl = blockPos + Vec3(1.f, 1.f, 0.f);
				br = bl;
				br.x -= 1.f;
				tr = br;
				tr.z += 1.f;
				tl = tr;
				tl.x += 1.f;
			}

			else if (raycastResult.m_impactNormal == Vec3::SOUTH)
			{
				bl = blockPos;
				br = bl;
				br.x += 1.f;
				tr = br;
				tr.z += 1.f;
				tl = blockPos;
				tl.z += 1.f;
			}

			else if (raycastResult.m_impactNormal == Vec3::EAST)
			{
				bl = blockPos;
				bl.x += 1.f;
				br = bl;
				br.y += 1.f;
				tr = br;
				tr.z += 1.f;
				tl = bl;
				tl.z += 1.f;
			}

			else if (raycastResult.m_impactNormal == Vec3::WEST)
			{
				bl = blockPos;
				bl.y += 1.f;
				br = blockPos;
				tr = br;
				tr.z += 1.f;
				tl = bl;
				tl.z += 1.f;
			}


			DebugAddWorldWireFrameQuad(bl, br, tr, tl, 0.025f, 0.00001f, Rgba8::ORANGE, Rgba8::ORANGE);

		}
	}

	else
	{
		m_selectedBlock = false;
	}

	if (m_lockedRaycast || m_camera->m_cameraMode == CameraMode::INDEPENDENT)
	{
		Vec3 raycastDrawStartPos = startPos + (fwrd * 1.f);
		DebugAddWorldLine(raycastDrawStartPos, hitPos, 0.025f, 0.00001f, raycastColor, raycastColor, DebugRenderMode::USE_DEPTH);
		if (raycastResult.m_didImpact)
		{
			DebugAddWorldPoint(raycastResult.m_impactPos, 0.05f, 0.00001f, Rgba8::BLUE, Rgba8::BLUE);
			DebugAddWorldArrow(raycastResult.m_impactPos, raycastResult.m_impactPos + (raycastResult.m_impactNormal * 0.2f), 0.025f, 0.00001f, Rgba8::WHITE, Rgba8::WHITE);
		}
	}


}

void Player::SwitchPhysicsMode()
{
	int mode = (int)m_physicsMode;
	mode = GetClampedInt(mode, 1, (int)PhysicsMode::COUNT - 1);
	mode++;

	if (mode >= (int)PhysicsMode::COUNT)
	{
		mode = 1;
	}

	m_physicsMode = (PhysicsMode)mode;
	DebugAddMessage(Stringf("Physics Mode [v]: %s", GetStringForPhysicsMode().c_str()), 5.f, Rgba8::CYAN, Rgba8::CYAN);
}

std::string Player::GetStringForPhysicsMode() const
{
	switch (m_physicsMode)
	{
	case PhysicsMode::WALKING: return "Walking";
	case PhysicsMode::SWIMMING: return "Swimming";
	case PhysicsMode::FLYING: return "Flying";
	case PhysicsMode::NO_CLIP: return "No Clip";
	default: return "Invalid";
	}
}
