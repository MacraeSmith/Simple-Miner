#include "Engine/Math/ConvexPoly2.hpp"
#include "Engine/Math/ConvexHull2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Core/EngineCommon.hpp"

ConvexPoly2::ConvexPoly2(std::vector<Vec2> vertexPositions)
	:m_vertexPositions(vertexPositions)
{
}


void ConvexPoly2::AddPoint(Vec2 const& point)
{
	int const numVertexes = (int)m_vertexPositions.size();

	if (numVertexes == 0)
	{
		m_vertexPositions.push_back(point);
		return;
	}

	if (numVertexes == 1)
	{
		m_vertexPositions.push_back(point);
		return;
	}

	if (numVertexes == 2)
	{
		// Make a valid winding (clockwise) if the three points are not colinear.
		Vec2 const& a = m_vertexPositions[0];
		Vec2 const& b = m_vertexPositions[1];
		Vec2 const& c = point;

		float cross = CrossProduct2D((b - a), (c - a));
		float eps = 1e-6f;

		if ((cross > -eps) && (cross < eps))
		{
			// Colinear, just add and let later points fix it (or handle extremes if you want)
			m_vertexPositions.push_back(point);
			return;
		}

		// If cross is positive, a->b->c is CCW, swap to make CW
		if (cross > 0.0f)
		{
			m_vertexPositions[1] = c;
			m_vertexPositions.push_back(b);
		}
		else
		{
			m_vertexPositions.push_back(c);
		}
		return;
	}

	// 1) Mark visible ("bad") edges.
	std::vector<bool> bad(numVertexes, false);
	int badCount = 0;

	for (int i = 0; i < numVertexes; ++i)
	{
		int const j = (i + 1 < numVertexes) ? (i + 1) : 0;

		Vec2 const& a = m_vertexPositions[i];
		Vec2 const& b = m_vertexPositions[j];

		Vec2 const ab = (b - a);
		Vec2 const ap = (point - a);

		float const cross = CrossProduct2D(ab, ap);

		if (cross < 0.0f)
		{
			bad[i] = true;
			badCount += 1;
		}
	}

	// If nothing is visible, treat as inside/on-edge.
	if (badCount == 0)
	{
		return;
	}

	// 2) Find the start and end edge of the visible chain.
	int startEdge = -1;
	int endEdge = -1;

	for (int i = 0; i < numVertexes; ++i)
	{
		int const prev = (i - 1 >= 0) ? (i - 1) : (numVertexes - 1);
		int const next = (i + 1 < numVertexes) ? (i + 1) : 0;

		if (bad[i] && !bad[prev])
		{
			startEdge = i;
		}

		if (bad[i] && !bad[next])
		{
			endEdge = i;
		}
	}

	// If we did not find a clean chain, bail.
	if (startEdge == -1 || endEdge == -1)
	{
		return;
	}

	// 3) Keep vertices from (endEdge + 1) up to (startEdge + 1), wrapping.
	std::vector<Vec2> newVerts;
	newVerts.reserve(numVertexes + 1);
	int i = (endEdge + 1 < numVertexes) ? (endEdge + 1) : 0;
	int stop = (startEdge + 1 < numVertexes) ? (startEdge + 1) : 0;

	// Copy at least one vertex. This matters when startEdge == endEdge.
	int cur = i;
	do
	{
		newVerts.push_back(m_vertexPositions[cur]);
		cur = (cur + 1 < numVertexes) ? (cur + 1) : 0;
	} while (cur != stop);

	// 4) Insert the new point to replace the removed visible chain.
	newVerts.push_back(point);

	m_vertexPositions = newVerts;
}


bool ConvexPoly2::IsPointInside(Vec2 const& point) const
{	
	const int numPoints = (int)m_vertexPositions.size();

	for (int i = 0; i < numPoints; ++i)
	{
		Vec2 a = m_vertexPositions[i];
		int j = i + 1 < numPoints ? i + 1 : 0;
		Vec2 b = m_vertexPositions[j];

		Vec2 ab = (b - a);
		Vec2 ap = (point - a);

		float cross = CrossProduct2D(ab, ap);

		if (cross < 0.f)
		{
			return false;
		}
	}

	return true;
}

Vec2 ConvexPoly2::GetAverageOfPoints() const
{
	Vec2 pointTotal;
	for (int i = 0; i < (int)m_vertexPositions.size(); ++i)
	{
		pointTotal += m_vertexPositions[i];
	}

	return pointTotal / (float)m_vertexPositions.size();
}

int ConvexPoly2::GetNumVertexPositions() const
{
	return (int)m_vertexPositions.size();
}

const std::vector<Vec2> ConvexPoly2::GetVertexPositions() const
{
	return m_vertexPositions;
}

Vec2 ConvexPoly2::GetVertexPositionAtIndex(int index) const
{
	return m_vertexPositions[index];
}

void ConvexPoly2::Translate(Vec2 const& translation)
{
	for (int i = 0; i < (int)m_vertexPositions.size(); ++i)
	{
		m_vertexPositions[i] += translation;
	}
}

void ConvexPoly2::ScaleFromPoint(Vec2 const& point, float scaleAmount)
{
	for (int i = 0; i < (int)m_vertexPositions.size(); ++i)
	{
		Vec2 displacement = m_vertexPositions[i] - point;
		displacement *= scaleAmount;
		m_vertexPositions[i] = point + displacement;
	}
}

void ConvexPoly2::RotateAroundPoint(Vec2 const& point, float theta)
{
	float const c = cosf(theta);
	float const s = sinf(theta);

	for (int i = 0; i < (int)m_vertexPositions.size(); ++i)
	{
		Vec2 displacement = m_vertexPositions[i] - point;
		Vec2 rotated;
		rotated.x = ((displacement.x * c) - (displacement.y * s));
		rotated.y = ((displacement.x * s) + (displacement.y * c));

		m_vertexPositions[i] = point + rotated;
	}
	
}

Disc2 ConvexPoly2::GetMinimumBoundingDisc() const
{
	std::vector<Vec2> shuffledPoints = GetDeterministicShuffledPositions();
	Disc2 disc;
	disc.m_radius = -1;

	for (int i = 0; i < (int)shuffledPoints.size(); ++i)
	{
		if(disc.m_radius >= 0.f && disc.IsPointInside(shuffledPoints[i]))
			continue;

		disc.m_center = shuffledPoints[i];
		disc.m_radius = 0.f;

		for (int j = 0; j < i; ++j)
		{
			if(disc.IsPointInside(shuffledPoints[j]))
				continue;

			disc = Disc2::GetDiscFromTwoPoints(shuffledPoints[i], shuffledPoints[j]);

			for (int k = 0; k < j; ++k)
			{
				if(disc.IsPointInside(shuffledPoints[k]))
					continue;

				disc = Disc2::GetDiscFromThreePoints(shuffledPoints[i], shuffledPoints[j], shuffledPoints[k]);
			}
		}
	}

	return disc;
}

ConvexHull2 ConvexPoly2::GetAsConvexHull() const
{
	const int numPlanes = GetNumVertexPositions();

	std::vector<Plane2D> planes;
	planes.reserve(numPlanes);

	for (int i = 0; i < numPlanes; ++i)
	{
		Vec2 a = m_vertexPositions[i];
		int j = i + 1 < numPlanes ? i + 1 : 0;
		Vec2 b = m_vertexPositions[j];

		Vec2 ab = (b - a);
		Vec2 abNormalized = ab.GetNormalized();

		Vec2 planeNormal = abNormalized.GetRotatedMinus90Degrees();
		float planeDistance = DotProduct2D(a, planeNormal);

		Plane2D plane(planeNormal, planeDistance);
		planes.push_back(plane);
	}

	return ConvexHull2(planes);
}

ConvexPoly2 ConvexPoly2::ClipConvexPolyByPlane(Plane2D const& plane, bool keepFrontSide)
{
	ConvexPoly2 out;

	int const count = (int)m_vertexPositions.size();
	if (count < 3)
	{
		return out;
	}

	out.m_vertexPositions.clear();
	out.m_vertexPositions.reserve(count + 1);

	for (int i = 0; i < count; ++i)
	{
		int const j = (i + 1 < count) ? (i + 1) : 0;

		Vec2 const& a = m_vertexPositions[i];
		Vec2 const& b = m_vertexPositions[j];

		float const aAlt = plane.GetAltitudeFromPoint(a);
		float const bAlt = plane.GetAltitudeFromPoint(b);

		bool const aIn = keepFrontSide ? (aAlt >= 0.0f) : (aAlt <= 0.0f);
		bool const bIn = keepFrontSide ? (bAlt >= 0.0f) : (bAlt <= 0.0f);

		if (aIn && bIn)
		{
			// both inside -> keep b
			out.m_vertexPositions.push_back(b);
		}
		else if (aIn && !bIn)
		{
			// leaving -> add intersection
			float const denom = (aAlt - bAlt);
			if (denom != 0.0f)
			{
				float const t = (aAlt / denom);
				out.m_vertexPositions.push_back(Lerp(a, b, t));
			}
		}
		else if (!aIn && bIn)
		{
			// entering -> add intersection + b
			float const denom = (aAlt - bAlt);
			if (denom != 0.0f)
			{
				float const t = (aAlt / denom);
				out.m_vertexPositions.push_back(Lerp(a, b, t));
			}
			out.m_vertexPositions.push_back(b);
		}
		else
		{
			// both outside -> add nothing
		}
	}

	// Remove exact duplicate consecutive points (no eps)
	if ((int)out.m_vertexPositions.size() >= 2)
	{
		std::vector<Vec2> cleaned;
		cleaned.reserve(out.m_vertexPositions.size());

		for (int k = 0; k < (int)out.m_vertexPositions.size(); ++k)
		{
			Vec2 const& p = out.m_vertexPositions[k];
			if (cleaned.empty() || !(cleaned.back() == p))
			{
				cleaned.push_back(p);
			}
		}

		if (!cleaned.empty() && cleaned.front() == cleaned.back())
		{
			cleaned.pop_back();
		}

		out.m_vertexPositions = cleaned;
	}

	if ((int)out.m_vertexPositions.size() >= 3)
	{
		out.ArrangePointsCCW();
	}

	return out;
}

void ConvexPoly2::ClipConvexPolyByPlane(Plane2D const& plane, ConvexPoly2& out_front, ConvexPoly2& out_back) const
{
	out_front.SetVertexPositions({});
	out_back.SetVertexPositions({});

	const std::vector<Vec2> verts = GetVertexPositions();
	int const count = (int)verts.size();
	if (count < 3)
	{
		return;
	}

	std::vector<Vec2> frontVerts;
	std::vector<Vec2> backVerts;

	frontVerts.reserve(count + 1);
	backVerts.reserve(count + 1);

	for (int i = 0; i < count; ++i)
	{
		int const j = (i + 1 < count) ? (i + 1) : 0;

		Vec2 const& a = verts[i];
		Vec2 const& b = verts[j];

		float const aAlt = plane.GetAltitudeFromPoint(a);
		float const bAlt = plane.GetAltitudeFromPoint(b);

		bool const aFront = (aAlt >= 0.0f);
		bool const bFront = (bAlt >= 0.0f);
		bool const aBack = (aAlt <= 0.0f);
		bool const bBack = (bAlt <= 0.0f);

		// ---- FRONT OR ON ----
		if (aFront && bFront)
		{
			frontVerts.push_back(b);
		}
		else if (aFront && !bFront)
		{
			float const denom = (aAlt - bAlt);
			if (denom != 0.0f)
			{
				float const t = (aAlt / denom);
				frontVerts.push_back(Lerp(a, b, t));
			}
		}
		else if (!aFront && bFront)
		{
			float const denom = (aAlt - bAlt);
			if (denom != 0.0f)
			{
				float const t = (aAlt / denom);
				frontVerts.push_back(Lerp(a, b, t));
			}
			frontVerts.push_back(b);
		}

		// ---- BACK OR ON ----
		if (aBack && bBack)
		{
			backVerts.push_back(b);
		}
		else if (aBack && !bBack)
		{
			float const denom = (aAlt - bAlt);
			if (denom != 0.0f)
			{
				float const t = (aAlt / denom);
				backVerts.push_back(Lerp(a, b, t));
			}
		}
		else if (!aBack && bBack)
		{
			float const denom = (aAlt - bAlt);
			if (denom != 0.0f)
			{
				float const t = (aAlt / denom);
				backVerts.push_back(Lerp(a, b, t));
			}
			backVerts.push_back(b);
		}
	}

	auto CleanAndFinalize = [](std::vector<Vec2>& v)
		{
			if ((int)v.size() < 2)
			{
				return;
			}

			std::vector<Vec2> cleaned;
			cleaned.reserve(v.size());

			for (int i = 0; i < (int)v.size(); ++i)
			{
				if (cleaned.empty() || !(cleaned.back() == v[i]))
				{
					cleaned.push_back(v[i]);
				}
			}

			if (!cleaned.empty() && cleaned.front() == cleaned.back())
			{
				cleaned.pop_back();
			}

			v = cleaned;
		};

	CleanAndFinalize(frontVerts);
	CleanAndFinalize(backVerts);

	if ((int)frontVerts.size() >= 3)
	{
		out_front.SetVertexPositions(frontVerts);
		out_front.ArrangePointsCCW();
	}

	if ((int)backVerts.size() >= 3)
	{
		out_back.SetVertexPositions(backVerts);
		out_back.ArrangePointsCCW();
	}
}


std::vector<Vec2> ConvexPoly2::GetDeterministicShuffledPositions() const
{
	const int numPoints = (int)m_vertexPositions.size();

	int step = (numPoints / 2) + 1;
	while (GetGreatestCommonDivisor(step, numPoints) != 1)
	{
		step += 1;
		if(step >= 1)
			step = 1;
		if(step == 1)
			break;
	}

	std::vector<Vec2> shuffledPoints;
	shuffledPoints.reserve(numPoints);

	int idx = 0;
	for (int i = 0; i < numPoints; ++i)
	{
		shuffledPoints.push_back(m_vertexPositions[idx]);
		idx = (idx + step) % numPoints;
	}

	return shuffledPoints;
}

void ConvexPoly2::ArrangePointsCCW()
{
	int const n = (int)m_vertexPositions.size();
	if (n < 3)
	{
		return;
	}

	// 1) Compute centroid
	Vec2 center = Vec2::ZERO;
	for (int i = 0; i < n; ++i)
	{
		center += m_vertexPositions[i];
	}
	center *= (1.0f / (float)n);

	// 2) Selection sort by polar angle about centroid (ascending = CCW)
	for (int i = 0; i < (n - 1); ++i)
	{
		int bestIndex = i;
		float bestAngle = Vec2::GetPolarAngleAboutPoint(m_vertexPositions[i], center);

		for (int j = (i + 1); j < n; ++j)
		{
			float angle = Vec2::GetPolarAngleAboutPoint(m_vertexPositions[j], center);

			if (angle < bestAngle)
			{
				bestAngle = angle;
				bestIndex = j;
			}
			else if (angle == bestAngle)
			{
				// Tie-break: keep the farther one later in the order
				// so near-duplicates do not shuffle unpredictably.
				Vec2 const da = (m_vertexPositions[bestIndex] - center);
				Vec2 const db = (m_vertexPositions[j] - center);

				float const distASq = ((da.x * da.x) + (da.y * da.y));
				float const distBSq = ((db.x * db.x) + (db.y * db.y));

				// If angles equal, prefer the closer point first.
				if (distBSq < distASq)
				{
					bestIndex = j;
				}
			}
		}

		if (bestIndex != i)
		{
			Vec2 temp = m_vertexPositions[i];
			m_vertexPositions[i] = m_vertexPositions[bestIndex];
			m_vertexPositions[bestIndex] = temp;
		}
	}

	// 3) Ensure CCW winding (optional but recommended)
	// Signed area > 0 means CCW (typical 2D convention).
	float area2 = 0.0f;
	for (int i = 0; i < n; ++i)
	{
		int const j = (i + 1 < n) ? (i + 1) : 0;
		Vec2 const& a = m_vertexPositions[i];
		Vec2 const& b = m_vertexPositions[j];
		area2 += ((a.x * b.y) - (a.y * b.x));
	}

	if (area2 < 0.0f)
	{
		// Reverse in place
		for (int i = 0; i < (n / 2); ++i)
		{
			int const j = (n - 1 - i);
			Vec2 temp = m_vertexPositions[i];
			m_vertexPositions[i] = m_vertexPositions[j];
			m_vertexPositions[j] = temp;
		}
	}
}