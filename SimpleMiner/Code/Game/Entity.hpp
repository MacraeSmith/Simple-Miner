#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB3.hpp"
class Game;
struct Mat44;
class Timer;

enum class PhysicsMode : int
{
	SWIMMING,
	WALKING,
	FLYING,
	NO_CLIP, //Flying + no collisions
	COUNT
};
class Entity
{
public:
	Entity(Game* owner);
	explicit Entity(Game* owner, Vec3 const& position, EulerAngles const& orientation = EulerAngles::ZERO);
	virtual ~Entity();

	virtual void Update();
	void UpdatePhysics(float deltaSeconds);
	void UpdateCollision(float deltaSeconds);
	void UpdateIsGrounded();
	virtual void Render() const = 0;
	void AddForce(Vec3 const& force);
	void AddImpulse(Vec3 const& impulse);
	bool IsGrounded() const;
	void MoveInDirection(Vec3 const& direction);
	void Teleport(Vec3 const& newPos);

	virtual Mat44 GetModelToWorldTransform() const;
	Vec3 GetForwardNormal() const;

private:
	bool UseGravity() const;
	bool IsFlying() const;

public:
	Game* m_game = nullptr;
	Vec3 m_position;
	Vec3 m_velocity;
	Vec3 m_acceleration;
	AABB3 m_physicsBounds;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity;
	Rgba8 m_color = Rgba8::WHITE;
	float m_halfHeight = 0.5f;

	float m_groundDrag = 1.f;
	float m_airDrag = 1.f;
	float m_runSpeed = 1.f;
	float m_flySpeed = 50.f;
	bool m_isSprinting = false;
	float m_sprintMultiplier = 1.5f;

	PhysicsMode m_physicsMode = PhysicsMode::WALKING;

	bool m_isGrounded = false;
	float m_groundedFraction = 0.f;

	bool m_isGarbage = false;
	bool m_fixedUpdate = false;
	Timer* m_physicsTimer = nullptr;
	float m_phsyicsTimeStep = 0.005f;
};

