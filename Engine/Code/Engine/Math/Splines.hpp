#pragma once
#include "Engine/Math/Splines.hpp"
#include "Engine/Math/Vec2.hpp"
#include <vector>

class CubicHermiteCurve2D;
class CubicBezierCurve2D
{
public:
	Vec2 m_start;
	Vec2 m_guidePos1;
	Vec2 m_guidePos2;
	Vec2 m_end;

public:
	CubicBezierCurve2D() {};
	CubicBezierCurve2D(Vec2 const& startPos, Vec2 const& guidePos1, Vec2 const& guidePos2, Vec2 const& endPos);
	explicit CubicBezierCurve2D(CubicHermiteCurve2D const& fromHermite);
	~CubicBezierCurve2D() {}

	Vec2 EvaluateAtParametric(float parametricZeroToOne) const;
	float GetApproximateLength(int numSubdivisions = 64) const;
	Vec2 EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;
};

struct HermitePoint2D
{
	Vec2 m_position;
	Vec2 m_velocity;
};

class CubicHermiteCurve2D
{
public:
	HermitePoint2D m_start;
	HermitePoint2D m_end;
public:
	CubicHermiteCurve2D() {};
	CubicHermiteCurve2D(HermitePoint2D const& start, HermitePoint2D const& end);
	CubicHermiteCurve2D(Vec2 const& startPos, Vec2 const& startVelocity, Vec2 const& endPos, Vec2 const& endVelocity);
	explicit CubicHermiteCurve2D(CubicBezierCurve2D const& fromBezier);

	Vec2 EvaluateAtParametric(float parametricZeroToOne) const;
	float GetApproximateLength(int numSubdivisions = 64) const;
	Vec2 EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;
	void SetStartVelocity(Vec2 const& velocity);
	void SetEndVelocity(Vec2 const& velocity);
	Vec2 GetStartVelocity() const;
	Vec2 GetEndVelocity() const;

};

typedef std::vector<HermitePoint2D>(HermitePoints2D);
class CubicHermiteSpline2D
{
public:
	HermitePoints2D m_points;

public:
	CubicHermiteSpline2D() {};
	CubicHermiteSpline2D(HermitePoints2D const& points);
	CubicHermiteSpline2D(std::vector<Vec2> positions, std::vector<Vec2> velocities);
	CubicHermiteSpline2D(std::vector<Vec2> positions); //Catmull-Rom spline
	~CubicHermiteSpline2D() {}

	int GetNumberOfCurves() const;
	std::vector<CubicHermiteCurve2D> GetAllCurves() const;
	CubicHermiteCurve2D GetCurveAtParametric(float parametricZeroToOne) const;
	Vec2 EvaluateAtParametric(float parametricZeroToOne) const;
	float GetApproximateLength(int numSubdivisionsPerCurve = 64) const;
	Vec2 EvaluateAtApproximateDistance(float distanceAlongSpline, int numSubdivisionsPerCurve = 64) const;
};

