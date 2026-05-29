#include "OBB2.hpp"

OBB2::OBB2(Vec2 const& center, Vec2 const& iBasisNormal, Vec2 const& halfDimensionsIJ)
	:m_center(center)
	,m_iBasisNormal(iBasisNormal.GetNormalized())
	,m_halfDimensionsIJ(halfDimensionsIJ)
{
}

void const OBB2::GetCornerPoints(Vec2* out_forCornerWorldPositions) const
{
	Vec2 jBasisNormal = m_iBasisNormal.GetRotated90Degrees();
	float halfWidth = m_halfDimensionsIJ.x;
	float halfHeight = m_halfDimensionsIJ.y;

	out_forCornerWorldPositions[0] = m_center + Vec2((m_iBasisNormal * -halfWidth) + (jBasisNormal * -halfHeight)); //bottom left
	out_forCornerWorldPositions[1] = m_center + Vec2((m_iBasisNormal * halfWidth) + (jBasisNormal * -halfHeight)); // bottom right
	out_forCornerWorldPositions[2] = m_center + Vec2((m_iBasisNormal * halfWidth) + (jBasisNormal * halfHeight)); //top right
	out_forCornerWorldPositions[3] = m_center + Vec2((m_iBasisNormal * -halfWidth) + (jBasisNormal * halfHeight)); // top left
}

Vec2 const OBB2::GetWorldPosForLocalPos(Vec2 const& localPos) const
{
	Vec2 jBasisNormal = m_iBasisNormal.GetRotated90Degrees();
	return Vec2((m_iBasisNormal * localPos.x) + (jBasisNormal * localPos.y));
}


void OBB2::RotateAboutCenter(float rotationDeltaDegrees)
{
	m_iBasisNormal.RotateDegrees(rotationDeltaDegrees); 
}

void OBB2::Translate(Vec2 const& translation)
{
	m_center += translation;
}

