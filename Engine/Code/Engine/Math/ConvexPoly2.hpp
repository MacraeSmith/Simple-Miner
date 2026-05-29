#pragma once
#include "Engine/Math/Vec2.hpp"
#include <vector>
struct Disc2;
struct ConvexHull2;
struct Plane2D;
struct ConvexPoly2
{
public:
	ConvexPoly2() {};
	ConvexPoly2(std::vector<Vec2> vertexPositions);
	~ConvexPoly2() {};

	void AddPoint(Vec2 const& point);
	bool IsPointInside(Vec2 const& point) const;
	Vec2 GetAverageOfPoints() const;
	int GetNumVertexPositions() const;

	const std::vector<Vec2> GetVertexPositions() const;
	void SetVertexPositions(std::vector<Vec2> positions){m_vertexPositions = positions;}
	Vec2 GetVertexPositionAtIndex(int index) const;

	void Translate(Vec2 const& translation);
	void ScaleFromPoint(Vec2 const& point, float scaleAmount);
	void RotateAroundPoint(Vec2 const& point, float theta);
	Disc2 GetMinimumBoundingDisc() const;

	ConvexHull2 GetAsConvexHull() const;

	ConvexPoly2 ClipConvexPolyByPlane(Plane2D const& plane, bool keepFrontSide = true);
	void ClipConvexPolyByPlane(Plane2D const& plane, ConvexPoly2& out_front, ConvexPoly2& out_back) const;

private:
	void ArrangePointsCCW();

private:

	std::vector<Vec2> GetDeterministicShuffledPositions() const;

private:
	std::vector<Vec2> m_vertexPositions;
};

