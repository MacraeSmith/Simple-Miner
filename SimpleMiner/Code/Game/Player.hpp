#pragma once
#include "Game/Entity.hpp"
#include "Game/BlockIterator.hpp"
#include "Game/PlayerInventory.hpp"
#include "Engine/Math/IntVec3.hpp"
#include <string>
#include <cstdint>

class Game;
class Camera;
class IndexBuffer;
struct Frustum;
struct Block;
class GameCamera;
class VertexBuffer;
class Timer;
class Player : public Entity
{
public:
	explicit Player(Game* owner);
	virtual ~Player();

	virtual void Update() override;
	void UpdateMovementInput();
	void UpdateBlockInput();
	void UpdatePhysicsAndCameraModeInput();
	void UpdateIsSwimming();
	void UpdateDebugMessages();
	virtual void Render() const override;
	void RenderInventory() const;

	Frustum GetViewFrustrum() const;

	void RestartJumpTimer();
	bool IsUnderWater() const {return m_isUnderWater;}

private:
	void CheckWorldRaycast(Vec3 const& startPos, Vec3 const& fwrd);

	void SwitchPhysicsMode();
	std::string GetStringForPhysicsMode() const;


public:
	GameCamera* m_camera = nullptr;

private:
	uint8_t m_placedBlockType = 13;

	VertexBuffer* m_playerVBO = nullptr;

	Vec3 m_raycastStart;
	Vec3 m_raycastForward;
	bool m_lockedRaycast = false;
	float m_raycastLength = 8.f;
	bool m_drawRaycastHit = true;
	BlockIterator m_selectedBlockIterator = BlockIterator(nullptr, -1);
	IntVec3 m_selectedBlockGlobalCoords;
	bool m_selectedBlock = false;
	Vec3 m_selectedNormal;
	Timer* m_jumpTimer = nullptr;
	bool m_isUnderWater = false;
	PlayerInventory m_inventory;
	bool m_showInventory = true;
};

