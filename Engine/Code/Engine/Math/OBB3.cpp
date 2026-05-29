#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"

OBB3::OBB3(Vec3 const& center, Vec3 const& halfDimensions, Vec3 const& iBasis, Vec3 const& jBasis, Vec3 const& kBasis)
	:m_center(center)
	,m_halfDimensionsIJK(halfDimensions)
	,m_iBasis(iBasis)
	,m_jBasis(jBasis)
	,m_kBasis(kBasis)
{
}

OBB3::OBB3(Vec3 const& center, Vec3 const& halfDimensions, EulerAngles const& orientation)
	:m_center(center)
	,m_halfDimensionsIJK(halfDimensions)
{
	orientation.GetAsVectors_IFwd_JLeft_KUp(m_iBasis, m_jBasis, m_kBasis);
}

bool OBB3::IsPointInside(Vec3 const& point) const
{
	Vec3 displacement = point - m_center;
	float iDispPercent = DotProduct3D(displacement, m_iBasis);

	float halfDepth = m_halfDimensionsIJK.x;
	float halfWidth = m_halfDimensionsIJK.y;
	float halfHeight = m_halfDimensionsIJK.z; 

	if (iDispPercent >= halfDepth || iDispPercent <= -halfDepth)
		return false;

	float jDispPercent = DotProduct3D(displacement, m_jBasis);
	if (jDispPercent >= halfWidth || jDispPercent <= -halfWidth)
		return false;

	float kDispPercent = DotProduct3D(displacement, m_kBasis);
	if (kDispPercent >= halfHeight || kDispPercent <= -halfHeight)
		return false;

	return true;
}

Vec3 const OBB3::GetCenterPos() const
{
	return m_center;
}

Vec3 const OBB3::GetNearestPoint(Vec3 const& referencePos) const
{
	Vec3 displacement = referencePos - m_center;
	float iDispPercent = DotProduct3D(displacement, m_iBasis);

	float jDispPercent = DotProduct3D(displacement, m_jBasis);

	float kDispPercent = DotProduct3D(displacement, m_kBasis);

	if (iDispPercent < m_halfDimensionsIJK.x && iDispPercent > -m_halfDimensionsIJK.x
		&& jDispPercent < m_halfDimensionsIJK.y && jDispPercent > -m_halfDimensionsIJK.y
		&& kDispPercent < m_halfDimensionsIJK.z && jDispPercent > -m_halfDimensionsIJK.z)
	{
		return referencePos; // point is inside
	}

	float nearestPointI = GetClamped(iDispPercent, -m_halfDimensionsIJK.x, m_halfDimensionsIJK.x);
	float nearestPointJ = GetClamped(jDispPercent, -m_halfDimensionsIJK.y, m_halfDimensionsIJK.y);
	float nearestPointZ = GetClamped(kDispPercent, -m_halfDimensionsIJK.z, m_halfDimensionsIJK.z);

	Vec3 nearestPoint = m_center + (nearestPointI * m_iBasis) + (nearestPointJ * m_kBasis) + (nearestPointZ * m_kBasis);
	return nearestPoint;
}

void OBB3::Translate(Vec3 const& translation)
{
	m_center += translation;
}

void OBB3::SetHalfDimensions(Vec3 const& newHalfDimensions)
{
	m_halfDimensionsIJK = newHalfDimensions;
}
