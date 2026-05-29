#include "Game/Entity.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/World.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Renderer/DebugRender.hpp"

Entity::Entity(Game* owner)
	:m_game(owner)
{
}

Entity::Entity(Game* owner, Vec3 const& position, EulerAngles const& orientation)
	:m_game(owner)
	,m_position(position)
	,m_orientation(orientation)
{
	m_physicsBounds.SetCenter(m_position);
}

Entity::~Entity()
{
	if (m_physicsTimer)
	{
		delete m_physicsTimer;
		m_physicsTimer = nullptr;

	}
}

void Entity::Update()
{

	if (m_fixedUpdate && m_physicsTimer)
	{
		while (m_physicsTimer->DecrementPeriodIfElapsed())
		{
			UpdatePhysics(m_phsyicsTimeStep);
			UpdateCollision(m_phsyicsTimeStep);
		}
	}

	else
	{
		float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();
		UpdatePhysics(deltaSeconds);
		UpdateCollision(deltaSeconds);
	}

	UpdateIsGrounded();
}

void Entity::UpdatePhysics(float deltaSeconds)
{
	bool isGrounded = m_physicsMode != PhysicsMode::NO_CLIP ? IsGrounded() : false;

	Vec3 drag;
	if (m_physicsMode == PhysicsMode::WALKING)
	{
		float dragCoefficient = isGrounded ? m_groundDrag : m_airDrag;
		Vec3 horizontalVelocity = m_velocity;
		horizontalVelocity.z = 0.f;
		drag = -horizontalVelocity * dragCoefficient;
	}

	else if(m_physicsMode == PhysicsMode::SWIMMING)
	{
		drag = -m_velocity * WATER_DRAG;
	}

	else
	{
		drag = -m_velocity * m_airDrag;
	}

	AddForce(drag);
	if (UseGravity())
	{
		float gravity = m_physicsMode == PhysicsMode::SWIMMING ? GRAVITY_ACCELERATION - WATER_BOYANCY_FORCE : GRAVITY_ACCELERATION;
		m_acceleration.z -= gravity;
	}
	
	m_velocity += m_acceleration * deltaSeconds;
	m_acceleration = Vec3::ZERO;
}

void Entity::UpdateCollision(float deltaSeconds)
{
	Vec3 deltaPos = m_velocity * deltaSeconds;

	if (deltaPos.GetLengthSquared() < 0.000001f)
		return;

	if (m_physicsMode != PhysicsMode::NO_CLIP)
	{

		Vec3 direction = deltaPos.GetNormalized();

		float distance = deltaPos.GetLength() + (2.f * PLAYER_COLLISION_RAYCAST_OFFSET);

		Vec3 closestImpactFractions(1.f, 1.f, 1.f);

		AABB3 shrunken = m_physicsBounds;
		Vec3 offset(PLAYER_COLLISION_RAYCAST_OFFSET, PLAYER_COLLISION_RAYCAST_OFFSET, PLAYER_COLLISION_RAYCAST_OFFSET);
		shrunken.m_mins += offset;
		shrunken.m_maxs -= offset;

		float boundsMidHeight = shrunken.GetCenterPos().z;

		//bounds.m_mins
		Vec3 boundsCorners[12] =
		{
			// bottom ring
			shrunken.m_mins,
			Vec3(shrunken.m_maxs.x, shrunken.m_mins.y, shrunken.m_mins.z),
			Vec3(shrunken.m_maxs.x, shrunken.m_maxs.y, shrunken.m_mins.z),
			Vec3(shrunken.m_mins.x, shrunken.m_maxs.y, shrunken.m_mins.z),

			// top ring
			Vec3(shrunken.m_mins.x, shrunken.m_mins.y, shrunken.m_maxs.z),
			Vec3(shrunken.m_maxs.x, shrunken.m_mins.y, shrunken.m_maxs.z),
			shrunken.m_maxs,
			Vec3(shrunken.m_mins.x, shrunken.m_maxs.y, shrunken.m_maxs.z),

			//middle ring
			Vec3(shrunken.m_mins.x, shrunken.m_mins.y, boundsMidHeight),
			Vec3(shrunken.m_maxs.x, shrunken.m_mins.y, boundsMidHeight),
			Vec3(shrunken.m_maxs.x, shrunken.m_maxs.y, boundsMidHeight),
			Vec3(shrunken.m_mins.x, shrunken.m_maxs.y, boundsMidHeight),
		};

		for (int i = 0; i < 12; ++i)
		{
			Vec3 startPos = boundsCorners[i];
			BlockRaycastResult3D result = m_game->m_currentWorld->RaycastVsWorld(startPos, direction, distance);

			if (!result.m_didImpact)
				continue;

			float dotImpactNormal = DotProduct3D(direction, result.m_impactNormal);
			if (dotImpactNormal >= 0.f)
				continue;


			if (result.m_impactNormal.x != 0.f)
			{
				closestImpactFractions.x = GetMin(closestImpactFractions.x, result.m_impactFraction);
			}

			if (result.m_impactNormal.y != 0.f)
			{
				closestImpactFractions.y = GetMin(closestImpactFractions.y, result.m_impactFraction);
			}

			if (result.m_impactNormal.z != 0.f)
			{
				if (i < 4) // bottom points
				{
					closestImpactFractions.z = GetMin(closestImpactFractions.z, result.m_impactFraction);
				}

				else // only zero z velocity out if the impact normal is pointing down
				{
					float dotWorldDown = DotProduct3D(result.m_impactNormal, Vec3::DOWN);
					if (dotWorldDown >= 0.f)
					{
						closestImpactFractions.z = GetMin(closestImpactFractions.z, result.m_impactFraction);
					}
				}
			}

		}


		bool blockedX = closestImpactFractions.x < 1.f;
		bool blockedY = closestImpactFractions.y < 1.f;
		bool blockedZ = closestImpactFractions.z < 1.f;

		if (blockedX)
		{
			deltaPos.x = 0.f;
			m_velocity.x = 0.f;
		}

		if (blockedY)
		{
			deltaPos.y = 0.f;
			m_velocity.y = 0.f;
		}

		if (blockedZ)
		{
			deltaPos.z = 0.f;
			m_velocity.z = 0.f;
		}
	}


	m_position += deltaPos;
	m_position.z = GetClamped(m_position.z, WORLD_MIN_HEIGHT, WORLD_MAX_HEIGHT);
	Vec3 boundsPos = m_position;
	boundsPos.z += m_halfHeight;
	m_physicsBounds.SetCenter(boundsPos);
}

void Entity::UpdateIsGrounded()
{
	Vec3 offset(PLAYER_COLLISION_RAYCAST_OFFSET, PLAYER_COLLISION_RAYCAST_OFFSET, PLAYER_COLLISION_RAYCAST_OFFSET);
	AABB3 shrunken = m_physicsBounds;
	shrunken.m_mins += offset;
	shrunken.m_maxs -= offset;

	Vec3 boundsCorners[4] =
	{
		// bottom ring
		shrunken.m_mins,
		Vec3(shrunken.m_maxs.x, shrunken.m_mins.y, shrunken.m_mins.z),
		Vec3(shrunken.m_maxs.x, shrunken.m_maxs.y, shrunken.m_mins.z),
		Vec3(shrunken.m_mins.x, shrunken.m_maxs.y, shrunken.m_mins.z),
	};

	Vec3 direction = Vec3::DOWN;
	float distance = PLAYER_GROUNDED_RAYCAST_LENGTH + (2.f * PLAYER_COLLISION_RAYCAST_OFFSET);

	for (int i = 0; i < 4; ++i)
	{
		Vec3 startPos = boundsCorners[i];

		BlockRaycastResult3D result = m_game->m_currentWorld->RaycastVsWorld(startPos, direction, distance);

		float dotImpactNormal = DotProduct3D(direction, result.m_impactNormal);
		if (dotImpactNormal >= 0.f)
			continue;

		if (result.m_didImpact)
		{
			m_isGrounded = true;
			m_groundedFraction = result.m_impactFraction;
			return;
		}

	}

	m_groundedFraction = 1.f;
	m_isGrounded = false;
}

void Entity::AddForce(Vec3 const& force)
{
	m_acceleration += force;
}

void Entity::AddImpulse(Vec3 const& impulse)
{
	m_velocity += impulse;
}

bool Entity::IsGrounded() const
{
	return m_isGrounded;
}

void Entity::MoveInDirection(Vec3 const& direction)
{
	float speed = IsFlying() ? m_flySpeed : m_runSpeed;
	float drag = IsFlying() ? m_airDrag : m_groundDrag;
	if (m_isSprinting)
	{
		speed *= m_sprintMultiplier;
	}

	AddForce(speed * direction * drag);
}

void Entity::Teleport(Vec3 const& newPos)
{
	m_position = newPos;
	Vec3 boundsPos = newPos;
	boundsPos.z += m_halfHeight;
	m_physicsBounds.SetCenter(boundsPos);
}

Mat44 Entity::GetModelToWorldTransform() const
{
	Mat44 transform = Mat44::MakeTranslation3D(m_position);
	Mat44 rotationMat = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	transform.Append(rotationMat);
	return transform;
}

Vec3 Entity::GetForwardNormal() const
{
	return m_orientation.Get_IFwd();
}

bool Entity::UseGravity() const
{
	if(IsFlying())
		return false;

	if(IsGrounded())
		return false;

	return g_game->m_currentWorld->DoesChunkExistAtPosition(m_position);
}

bool Entity::IsFlying() const
{
	return m_physicsMode == PhysicsMode::FLYING || m_physicsMode == PhysicsMode::NO_CLIP;
}
