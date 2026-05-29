#pragma once
#include "Engine/Math/Vec3.hpp"
struct EulerAngles;
struct OBB3
{
public:
	Vec3 m_center;
	Vec3 m_halfDimensionsIJK;
	Vec3 m_iBasis;
	Vec3 m_jBasis;
	Vec3 m_kBasis;

	OBB3() {};
	~OBB3() {};
	explicit OBB3(Vec3 const& center, Vec3 const& halfDimensions, Vec3 const& iBasis, Vec3 const& jBasis, Vec3 const& kBasis);
	explicit OBB3(Vec3 const& center, Vec3 const& halfDimensions, EulerAngles const& orientation);

	bool IsPointInside(Vec3 const& point) const;
	Vec3 const GetCenterPos()const;
	Vec3 const GetNearestPoint(Vec3 const& referencePos) const;

	void Translate(Vec3 const& translation);
	void SetHalfDimensions(Vec3 const& newHalfDimensions);

};

