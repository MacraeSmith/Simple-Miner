#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"

void Camera::SetOrthoView(Vec2 const& bottomLeft, Vec2 const& topRight, float nearZ, float farZ)
{
	m_mode = eMode_Orthograhic;
	m_orthoBottomLeft = bottomLeft;
	m_orthoTopRight = topRight;
	m_orthoNear = nearZ;
	m_orthoFar = farZ;
}

void Camera::SetOrthoView(AABB3 const& cameraBounds)
{
	m_mode = eMode_Orthograhic;
	m_orthoBottomLeft = Vec2(cameraBounds.m_mins.x, cameraBounds.m_mins.y);
	m_orthoTopRight = Vec2(cameraBounds.m_maxs.x, cameraBounds.m_maxs.y);
	m_orthoNear = cameraBounds.m_mins.z;
	m_orthoFar = cameraBounds.m_maxs.z;
}

AABB2 Camera::GetOrthoBounds() const
{
	return AABB2(m_orthoBottomLeft, m_orthoTopRight);
}

void Camera::SetPerspectiveView(float aspect, float fov, float nearZ, float farZ)
{
	m_mode = eMode_Perspective;
	m_perspectiveAspect = aspect;
	m_perspectiveFOV = fov;
	m_perspectiveNear = nearZ;
	m_perspectiveFar = farZ;
}

void Camera::SetPositionAndOrientation(Vec3 const& pos, EulerAngles const& orientation)
{
	m_position = pos;
	m_orientation = orientation;
}

void Camera::SetPosition(Vec3 const& position)
{
	m_position = position;
}

Vec3 Camera::GetPosition() const
{
	return m_position;
}

void Camera::SetOrientation(EulerAngles const& orientation)
{
	m_orientation = orientation;
}


EulerAngles Camera::GetOrientation() const
{
	return m_orientation;
}


Vec2 Camera::GetOrthoBottomLeft() const
{
	return m_orthoBottomLeft;
}

Vec2 Camera::GetOrthoTopRight() const
{
	return m_orthoTopRight;
}

Mat44 Camera::GetOrthoMatrix() const
{
	return Mat44::MakeOrthoProjection(m_orthoBottomLeft.x, m_orthoTopRight.x, m_orthoBottomLeft.y, m_orthoTopRight.y, m_orthoNear, m_orthoFar);
}

Mat44 Camera::GetPerspectiveMatrix() const
{
	return Mat44::MakePerspectiveProjection(m_perspectiveFOV, m_perspectiveAspect, m_perspectiveNear, m_perspectiveFar);
}

Mat44 Camera::GetProjectionMatrix() const
{
	switch (m_mode)
	{
	case Camera::eMode_Orthograhic:
		return GetOrthoMatrix();
	case Camera::eMode_Perspective:
		return GetPerspectiveMatrix();
	}
	return GetOrthoMatrix();
}


Mat44 Camera::GetWorldToClipTransform() const
{
	Mat44 worldToClipTransform = GetRenderToClipTransform();
	worldToClipTransform.Append(GetCameraToRenderTransform());
	worldToClipTransform.Append(GetWorldToCameraTransform());
	return worldToClipTransform;
}

Mat44 Camera::GetClipToWorldTransform() const
{
	Mat44 worldToClipTransform = GetWorldToClipTransform();
	Mat44 const clipToWorldTransform = worldToClipTransform.GetInverse();
	return clipToWorldTransform;
}

void Camera::SetViewportBounds(AABB2 const& bounds)
{
	m_viewportBounds = bounds;
}

AABB2 Camera::GetViewportBounds() const
{
	return m_viewportBounds;
}

Vec2 Camera::GetViewportDimensions() const
{
	return Vec2(m_viewportBounds.m_maxs.x - m_viewportBounds.m_mins.x, m_viewportBounds.m_maxs.y - m_viewportBounds.m_mins.y);
}

Mat44 Camera::GetCameraToWorldTransform() const
{
	Mat44 transform = Mat44::MakeTranslation3D(m_position);
	Mat44 rotationMat = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	transform.Append(rotationMat);
	return transform;
}

Mat44 Camera::GetWorldToCameraTransform() const
{
	Mat44 cameraTransform = GetCameraToWorldTransform();
	return cameraTransform.GetOrthonormalInverse();
}

void Camera::SetCameraToRenderTransform(Mat44 const& matrix)
{
	m_cameraToRenderTransform = matrix;
}

Mat44 Camera::GetCameraToRenderTransform() const
{
	return m_cameraToRenderTransform;
}

Mat44 Camera::GetRenderToClipTransform() const
{
	return GetProjectionMatrix();
}

Frustum Camera::GetPerspectiveFrustum() const
{
	float halfTanPerspective = TanDegrees(m_perspectiveFOV * 0.5f);

	float nearHalfHeight = halfTanPerspective * m_perspectiveNear;
	float nearHalfWidth = nearHalfHeight * m_perspectiveAspect;
	float farHalfHeight = halfTanPerspective * m_perspectiveFar;
	float farHalfWidth = farHalfHeight * m_perspectiveAspect;

	Vec3 nBL(m_perspectiveNear, nearHalfWidth, -nearHalfHeight);
	Vec3 nBR(m_perspectiveNear, -nearHalfWidth, -nearHalfHeight);
	Vec3 nTR(m_perspectiveNear, -nearHalfWidth, nearHalfHeight);
	Vec3 nTL(m_perspectiveNear, nearHalfWidth, nearHalfHeight);

	Vec3 fBL(m_perspectiveFar, farHalfWidth, -farHalfHeight);
	Vec3 fBR(m_perspectiveFar, -farHalfWidth, -farHalfHeight);
	Vec3 fTR(m_perspectiveFar, -farHalfWidth, farHalfHeight);
	Vec3 fTL(m_perspectiveFar, farHalfWidth, farHalfHeight);

	Mat44 transform = Mat44::MakeTranslation3D(m_position);
	transform.Append(m_orientation.GetAsMatrix_IFwd_JLeft_KUp());

	nBL = transform.TransformPosition3D(nBL);
	nBR = transform.TransformPosition3D(nBR);
	nTR = transform.TransformPosition3D(nTR);
	nTL = transform.TransformPosition3D(nTL);

	fBL = transform.TransformPosition3D(fBL);
	fBR = transform.TransformPosition3D(fBR);
	fTR = transform.TransformPosition3D(fTR);
	fTL = transform.TransformPosition3D(fTL);

	Frustum frustum;
	frustum.m_nearPlanePoints[0] = nBL;
	frustum.m_nearPlanePoints[1] = nBR;
	frustum.m_nearPlanePoints[2] = nTR;
	frustum.m_nearPlanePoints[3] = nTL;

	frustum.m_farPlanePoints[0] = fBL;
	frustum.m_farPlanePoints[1] = fBR;
	frustum.m_farPlanePoints[2] = fTR;
	frustum.m_farPlanePoints[3] = fTL;

	frustum.m_planes[0] = Plane3D(nBL, nBR, nTL); //near
	frustum.m_planes[1] = Plane3D(fBR, fBL, fTR); //far 
	frustum.m_planes[2] = Plane3D(nBL, fBL, nBR); //bottom
	frustum.m_planes[3] = Plane3D(nTL, nTR, fTL); //top
	frustum.m_planes[4] = Plane3D(fBL, nBL, fTL); //left
	frustum.m_planes[5] = Plane3D(nBR, fBR, nTR); //right

	return frustum;
}


Frustum Camera::GetPerspectiveFrustum(Vec3 const& pos, EulerAngles const& orientation, float fov, float aspect, float nearDistance, float farDistance) const
{
	float halfTanPerspective = TanDegrees(fov * 0.5f);

	float nearHalfHeight = halfTanPerspective * nearDistance;
	float nearHalfWidth = nearHalfHeight * aspect;
	float farHalfHeight = halfTanPerspective * farDistance;
	float farHalfWidth = farHalfHeight * aspect;

	Vec3 nBL(nearDistance, nearHalfWidth, -nearHalfHeight);
	Vec3 nBR(nearDistance, -nearHalfWidth, -nearHalfHeight);
	Vec3 nTR(nearDistance, -nearHalfWidth, nearHalfHeight);
	Vec3 nTL(nearDistance, nearHalfWidth, nearHalfHeight);

	Vec3 fBL(farDistance, farHalfWidth, -farHalfHeight);
	Vec3 fBR(farDistance, -farHalfWidth, -farHalfHeight);
	Vec3 fTR(farDistance, -farHalfWidth, farHalfHeight);
	Vec3 fTL(farDistance, farHalfWidth, farHalfHeight);

	Mat44 transform = Mat44::MakeTranslation3D(pos);
	transform.Append(orientation.GetAsMatrix_IFwd_JLeft_KUp());

	nBL = transform.TransformPosition3D(nBL);
	nBR = transform.TransformPosition3D(nBR);
	nTR = transform.TransformPosition3D(nTR);
	nTL = transform.TransformPosition3D(nTL);

	fBL = transform.TransformPosition3D(fBL);
	fBR = transform.TransformPosition3D(fBR);
	fTR = transform.TransformPosition3D(fTR);
	fTL = transform.TransformPosition3D(fTL);

	Frustum frustum;
	frustum.m_nearPlanePoints[0] = nBL;
	frustum.m_nearPlanePoints[1] = nBR;
	frustum.m_nearPlanePoints[2] = nTR;
	frustum.m_nearPlanePoints[3] = nTL;

	frustum.m_farPlanePoints[0] = fBL;
	frustum.m_farPlanePoints[1] = fBR;
	frustum.m_farPlanePoints[2] = fTR;
	frustum.m_farPlanePoints[3] = fTL;

	frustum.m_planes[0] = Plane3D(nBL, nBR, nTL); //near
	frustum.m_planes[1] = Plane3D(fBR, fBL, fTR); //far 
	frustum.m_planes[2] = Plane3D(nBL, fBL, nBR); //bottom
	frustum.m_planes[3] = Plane3D(nTL, nTR, fTL); //top
	frustum.m_planes[4] = Plane3D(fBL, nBL, fTL); //left
	frustum.m_planes[5] = Plane3D(nBR, fBR, nTR); //right

	return frustum;
}
