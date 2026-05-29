#include "Engine/Math/Splines.hpp"
#include "Engine/Math/MathUtils.hpp"


//Cubic Bezier Curve 2D
//-------------------------------------------------------------------------------------------------------------------
CubicBezierCurve2D::CubicBezierCurve2D(Vec2 const& startPos, Vec2 const& guidePos1, Vec2 const& guidePos2, Vec2 const& endPos)
	:m_start(startPos)
	, m_guidePos1(guidePos1)
	, m_guidePos2(guidePos2)
	, m_end(endPos)
{
}
CubicBezierCurve2D::CubicBezierCurve2D(CubicHermiteCurve2D const& fromHermite)
	:m_start(fromHermite.m_start.m_position)
	,m_guidePos1(fromHermite.m_start.m_position + (fromHermite.m_start.m_velocity * 0.33333333f))
	,m_guidePos2(fromHermite.m_end.m_position - (fromHermite.m_end.m_velocity * 0.33333333f))
	,m_end(fromHermite.m_end.m_position)
{
}

Vec2 CubicBezierCurve2D::EvaluateAtParametric(float parametricZeroToOne) const
{
	Vec2 pos;
	pos.x = ComputeCubicBezier1D(m_start.x, m_guidePos1.x, m_guidePos2.x, m_end.x, parametricZeroToOne);
	pos.y = ComputeCubicBezier1D(m_start.y, m_guidePos1.y, m_guidePos2.y, m_end.y, parametricZeroToOne);
	return pos;
}

float CubicBezierCurve2D::GetApproximateLength(int numSubdivisions) const
{

	float length = 0;
	float tStep = 1.f / numSubdivisions;
	for (int subDivNum = 0; subDivNum < numSubdivisions; ++subDivNum)
	{
		float t = GetClamped(subDivNum * tStep, 0.f, 1.f);
		float nextT = GetClamped((subDivNum + 1.f) * tStep, 0.f, 1.f);
		Vec2 start = EvaluateAtParametric(t);
		Vec2 end = EvaluateAtParametric(nextT);
		length += (end - start).GetLength();
	}
	return length;
}

Vec2 CubicBezierCurve2D::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions) const
{
	float length = 0;
	float tStep = 1.f / numSubdivisions;
	for (int subDivNum = 0; subDivNum < numSubdivisions; ++subDivNum)
	{
		float t = GetClamped(subDivNum * tStep, 0.f, 1.f);
		float nextT = GetClamped((subDivNum + 1.f) * tStep, 0.f, 1.f);

		Vec2 start = EvaluateAtParametric(t);
		Vec2 end = EvaluateAtParametric(nextT);
		Vec2 displacement = end - start;
		float subDivLength = displacement.GetLength();
		float prevLength = length;
		length += subDivLength;

		if (distanceAlongCurve <= length)
		{
			float fractionAlongSubDiv = GetFractionWithinRange(distanceAlongCurve, prevLength, length);
			return Lerp(start, end, fractionAlongSubDiv);
		}

	}

	//If distanceAlongCurve is greater than approximate length of curve
	return m_end;
}

//Cubic Hermite Curve 2D
//-------------------------------------------------------------------------------------------------------------------
CubicHermiteCurve2D::CubicHermiteCurve2D(HermitePoint2D const& start, HermitePoint2D const& end)
	:m_start(start)
	,m_end(end)
{
}

CubicHermiteCurve2D::CubicHermiteCurve2D(Vec2 const& startPos, Vec2 const& startVelocity, Vec2 const& endPos, Vec2 const& endVelocity)
	:m_start({startPos, startVelocity})
	,m_end({endPos, endVelocity})
{
}

CubicHermiteCurve2D::CubicHermiteCurve2D(CubicBezierCurve2D const& fromBezier)
	:m_start({fromBezier.m_start, (3.f * (fromBezier.m_guidePos1 - fromBezier.m_start))})
	,m_end({fromBezier.m_end, (3.f * (fromBezier.m_end - fromBezier.m_guidePos2))})
{
}

Vec2 CubicHermiteCurve2D::EvaluateAtParametric(float parametricZeroToOne) const
{
	CubicBezierCurve2D bezierCurve(*this);
	return bezierCurve.EvaluateAtParametric(parametricZeroToOne);
}

float CubicHermiteCurve2D::GetApproximateLength(int numSubdivisions) const
{
	CubicBezierCurve2D bezierCurve(*this);
	return bezierCurve.GetApproximateLength(numSubdivisions);
}

Vec2 CubicHermiteCurve2D::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions) const
{
	CubicBezierCurve2D bezierCurve(*this);
	return bezierCurve.EvaluateAtApproximateDistance(distanceAlongCurve, numSubdivisions);
}

void CubicHermiteCurve2D::SetStartVelocity(Vec2 const& velocity)
{
	m_start.m_velocity = velocity;
}

void CubicHermiteCurve2D::SetEndVelocity(Vec2 const& velocity)
{
	m_end.m_velocity = velocity;
}

Vec2 CubicHermiteCurve2D::GetStartVelocity() const
{
	return m_start.m_velocity;
}

Vec2 CubicHermiteCurve2D::GetEndVelocity() const
{
	return m_end.m_velocity;
}

//Cubic Hermite Spline 2D
//-------------------------------------------------------------------------------------------------------------------

CubicHermiteSpline2D::CubicHermiteSpline2D(HermitePoints2D const& points)
	:m_points(points)
{
}

CubicHermiteSpline2D::CubicHermiteSpline2D(std::vector<Vec2> positions, std::vector<Vec2> velocities)
{
	HermitePoints2D hermitePoints;
	hermitePoints.reserve(positions.size());

	for (int posNum = 0; posNum < (int)positions.size(); ++posNum)
	{
		hermitePoints.push_back({ positions[posNum], velocities[posNum] });
	}
}

CubicHermiteSpline2D::CubicHermiteSpline2D(std::vector<Vec2> positions)
{
	m_points.reserve(positions.size());
	m_points.push_back({ positions[0], Vec2::ZERO });

	for (int posNum = 1; posNum < (int)positions.size() - 1; ++posNum)
	{
		Vec2 prevPos = positions[posNum - 1];
		Vec2 pos = positions[posNum];
		Vec2 nextPos = positions[posNum + 1];

		//#TODO check this Catmull-Rom equation
// 		Vec2 dispAB = pos - prevPos;
// 		Vec2 dispBC = nextPos - pos;
// 		Vec2 velocity = 0.5f * (dispAB + dispBC);

		Vec2 velocity = 0.5f * (nextPos - prevPos);
		m_points.push_back({ pos, velocity });
	}

	m_points.push_back({ positions[positions.size() - 1], Vec2::ZERO });
}

int CubicHermiteSpline2D::GetNumberOfCurves() const
{
	return (int)m_points.size() - 1;
}

std::vector<CubicHermiteCurve2D> CubicHermiteSpline2D::GetAllCurves() const
{
	std::vector<CubicHermiteCurve2D> curves;
	const int NUM_CURVES = GetNumberOfCurves();
	curves.reserve(NUM_CURVES);
	for (int pointNum = 0; pointNum < NUM_CURVES; ++pointNum)
	{
		HermitePoint2D start = m_points[pointNum];
		HermitePoint2D end = m_points[pointNum + 1];
		curves.push_back(CubicHermiteCurve2D(start, end));
	}
	return curves;
}

CubicHermiteCurve2D CubicHermiteSpline2D::GetCurveAtParametric(float parametricZeroToOne) const
{
	float t = Lerp(0.f, (float)m_points.size(), parametricZeroToOne);
	CubicHermiteCurve2D hermiteCurve;
	hermiteCurve.m_start = m_points[(int)floorf(t)];
	hermiteCurve.m_end = m_points[(int)ceilf(t)];
	return hermiteCurve;
}

Vec2 CubicHermiteSpline2D::EvaluateAtParametric(float parametricZeroToOne) const
{
	float t = GetClamped(Lerp(0.f, (float)m_points.size() - 1, parametricZeroToOne), 0.f, (float)m_points.size() - 1);
	CubicHermiteCurve2D hermiteCurve;
	int curveIndex = (int)floor(t);
	hermiteCurve.m_start = m_points[curveIndex];
	hermiteCurve.m_end = m_points[curveIndex + 1];
	return hermiteCurve.EvaluateAtParametric(t - (float)curveIndex);
}

float CubicHermiteSpline2D::GetApproximateLength(int numSubdivisionsPerCurve) const
{
	std::vector<CubicHermiteCurve2D> curves = GetAllCurves();
	float length = 0;
	for (int curveNum = 0; curveNum < (int)curves.size(); ++curveNum)
	{
		length += curves[curveNum].GetApproximateLength(numSubdivisionsPerCurve);
	}

	return length;
}

Vec2 CubicHermiteSpline2D::EvaluateAtApproximateDistance(float distanceAlongSpline, int numSubdivisionsPerCurve) const
{
	float length = 0;
	std::vector<CubicHermiteCurve2D> curves = GetAllCurves();
	for (int curveNum = 0; curveNum < (int)curves.size(); ++curveNum)
	{
		CubicHermiteCurve2D curve = curves[curveNum];
		float tStep = 1.f / numSubdivisionsPerCurve;
		for (int subDivNum = 0; subDivNum < numSubdivisionsPerCurve; ++subDivNum)
		{
			float t = GetClamped(subDivNum * tStep, 0.f, 1.f);
			float nextT = GetClamped((subDivNum + 1.f) * tStep, 0.f, 1.f);

			Vec2 start = curve.EvaluateAtParametric(t);
			Vec2 end = curve.EvaluateAtParametric(nextT);
			Vec2 displacement = end - start;
			float subDivLength = displacement.GetLength();
			float prevLength = length;
			length += subDivLength;
			if (distanceAlongSpline <= length)
			{
				float fractionAlongSubDiv = GetClampedFractionWithinRange(distanceAlongSpline, prevLength, length);
				return Lerp(start, end, fractionAlongSubDiv);
			}
		}
	}
	return m_points[(int)m_points.size() - 1].m_position;
}

