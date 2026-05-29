#include "Engine/Math/ZSphere3D.hpp"

ZSphere3D::ZSphere3D(Vec3 const& center, float radius)
	:m_center(center)
	,m_radius(radius)
{
}

void ZSphere3D::Translate(Vec3 const& translation)
{
	m_center += translation;
}
