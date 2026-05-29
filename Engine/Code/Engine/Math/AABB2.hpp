#pragma once
#include "Engine/Math/Vec2.hpp"
#include <vector>
class RandomNumberGenerator;
struct AABB2
{
public:
	Vec2 m_mins;
	Vec2 m_maxs;

	static const AABB2 ZERO;
	static const AABB2 ONE;
	static const AABB2 ZERO_TO_ONE;
	static const AABB2 ONE_TO_ZERO;
public:
	//Constructor/Destruction
	~AABB2(){}
	AABB2() {}
	AABB2(AABB2 const& copyFrom);
	explicit AABB2(float minX, float minY, float maxX, float maxY);
	explicit AABB2(Vec2 const& mins, Vec2 const& maxs);

	//Accessors (const methods)
	bool IsPointOnOrInside(Vec2 const& point) const;
	bool IsPointInsideBounds(Vec2 const& point) const;
	bool IsDiscInside(Vec2 const& discCenter, float discRadius) const;
	Vec2 const GetCenterPos() const;
	Vec2 const GetDimensions() const;
	float GetHeight() const;
	float GetWidth() const;
	float GetAspect() const;
	Vec2 const GetNearestPoint(Vec2 const& referencePosition) const;
	Vec2 const GetUVForPoint(Vec2 const& point) const;
	Vec2 const GetRandomPointInBounds(RandomNumberGenerator* randomNumberGenerator = nullptr);
	Vec2 const GetRandomPointOnEdgeOfBounds(RandomNumberGenerator* randomNumberGenerator = nullptr);

	Vec2 const GetPointAtUV(Vec2 const& uv) const;
	AABB2 const GetBoxAtUVS(Vec2 const& uvMins, Vec2 const& uvMaxs) const;
	AABB2 const GetBoxAtUVS(AABB2 const& uvs) const;

	//Mutators (non-const methods)
	void Translate(Vec2 const& translationToApply);
	void TranslateX(float translationX);
	void TranslateY(float translationY);
	void SetCenter(Vec2 const& newCenter);
	void SetDimensions(Vec2 const& newDimensions);
	void StretchToIncludePoint(Vec2 const& point);
	void AddPadding(float xPadPercent, float yPadPadPercent);
	void AddPadding(float minXPadPercent, float minYPadPercent, float maxXPadPercent, float maxYPadPercent);
	void AddPadding(Vec2 const& minsPadPercent, Vec2 const& maxsPadPercent);

	AABB2 ChopOffTop(float percentOfOriginialToChop); //chop off and return new piece
	AABB2 ChopOffBottom(float percentToChopOff);
	AABB2 ChopOffLeft(float percentToChopOff);
	AABB2 ChopOffRight(float percentToChopOff);
	std::vector<AABB2> GetHorizontalSlicedBoxesTopToBottom(int numBoxes);
	std::vector<AABB2> GetVerticalSlicedBoxesLeftToRight(int numBoxes);

	bool operator==(AABB2 otherBox);
	bool operator!=(AABB2 otherBox);
};

