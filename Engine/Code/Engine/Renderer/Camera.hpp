#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Plane3D.hpp"
#pragma once
struct AABB3;
struct Frustum
{
	Plane3D m_planes[6] = {};
	Vec3 m_nearPlanePoints[4] = {};
	Vec3 m_farPlanePoints[4] = {};
};

class Camera
{
public:
	enum Mode : int 
	{
		eMode_Orthograhic,
		eMode_Perspective,

		eMode_Count
	};
	
public:

	void SetOrthoView(Vec2 const& bottomLeft, Vec2 const& topRight, float nearZ = 0.f, float farZ = 1.f );
	void SetOrthoView(AABB3 const& cameraBounds);
	AABB2 GetOrthoBounds() const;
	void SetPerspectiveView(float aspect, float fov, float nearZ, float farZ);

	void SetPositionAndOrientation(Vec3 const& pos, EulerAngles const& orientation);
	void SetPosition(Vec3 const& position);
	Vec3 GetPosition() const;
	void SetOrientation(EulerAngles const& orientation);
	EulerAngles GetOrientation() const;

	Mat44 GetCameraToWorldTransform() const;
	Mat44 GetWorldToCameraTransform() const;

	void SetCameraToRenderTransform(Mat44 const& matrix);
	Mat44 GetCameraToRenderTransform() const;

	Mat44 GetRenderToClipTransform() const;

	Vec2 GetOrthoBottomLeft() const;
	Vec2 GetOrthoTopRight() const;

	Mat44 GetOrthoMatrix() const;
	Mat44 GetPerspectiveMatrix() const;
	Mat44 GetProjectionMatrix() const;

	Mat44 GetClipToWorldTransform() const;
	Mat44 GetWorldToClipTransform() const;

	void SetViewportBounds(AABB2 const& bounds);
	AABB2 GetViewportBounds() const;
	Vec2 GetViewportDimensions() const;

	float GetFOV() const {return m_perspectiveFOV;}
	float GetAspect() const {return m_perspectiveAspect;}
	float GetPerspectiveNearDistance() const {return m_perspectiveNear;}
	float GetPerspectiveFarDistance() const {return m_perspectiveFar;}
	Frustum GetPerspectiveFrustum() const;
	Frustum GetPerspectiveFrustum(Vec3 const& pos, EulerAngles const& orientation, float fov, float aspect, float nearDistance, float farDistance) const;

protected:
	Mode m_mode = eMode_Orthograhic;

	Vec3 m_position;
	EulerAngles m_orientation;

	Vec2 m_orthoBottomLeft;
	Vec2 m_orthoTopRight;
	float m_orthoNear;
	float m_orthoFar;

	float m_perspectiveAspect;
	float m_perspectiveFOV;
	float m_perspectiveNear;
	float m_perspectiveFar;

	Mat44 m_cameraToRenderTransform;
	AABB2 m_viewportBounds = AABB2::ZERO_TO_ONE;

};

