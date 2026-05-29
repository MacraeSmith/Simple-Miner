#pragma once
#include "Engine/Renderer/Camera.hpp"
class Entity;
class Player;
class VertexBuffer;
enum class CameraMode : int
{
	FIRST_PERSON,
	OVER_SHOULDER,
	SPECTATOR,
	SPECTATOR_XY,
	INDEPENDENT,
	COUNT,
};

class GameCamera : public Camera
{
friend class Entity;
friend class Player;

public:
	
	explicit GameCamera(Entity* owningEntity);
	virtual ~GameCamera();

	void Update();
	void UpdateInput();
	void UpdateCompass();

	void RenderCompass() const;
private:
	void SwitchCameraMode();
	std::string GetCameraModeString() const;
	Frustum GetViewFrustrum() const;
	bool IsFreeFly() const;

	void CreateCompass();
	float GetCameraClipDistance(Vec3 const& startPos, Vec3 const& raycastDirection);

public:

private:
	
	Entity* m_owningEntity = nullptr;
	CameraMode m_cameraMode = CameraMode::FIRST_PERSON;

	Vec3 m_compassPos;
	VertexBuffer* m_compassVbo = nullptr;

	Vec3 m_freeFlyVelocity = Vec3(4.f, 4.f, 4.f);
	float m_freeFlySprintSpeed = 20.f;

};

