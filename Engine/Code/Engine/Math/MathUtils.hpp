#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"

struct Vec4;
struct AABB2;
struct AABB3;
struct Disc2;
struct OBB2;
struct Capsule2;
struct LineSegment2;
struct Triangle2;
class TileHeatMap;
struct Mat44;
struct ZCylinder3D;
struct OBB3;
struct Plane3D;
struct Plane2D;
struct ConvexHull2;
class Camera;
struct Frustum;

struct Ray2
{
	Ray2() {}
	Ray2(Vec2 const& startPos, Vec2 const& endPos);
	Ray2(Vec2 const& startPos, Vec2 const& fwrdNormal, float maxLength);

	Vec2 m_startPos;
	Vec2 m_fwrdNormal;
	float m_maxLength = 1.f;

};

struct RaycastResult2D
{
	bool m_didImpact = false;
	float m_impactDistance = 0.f;
	Vec2 m_impactPos;
	Vec2 m_impactNormal;
};

struct Ray3
{
	Ray3() {}
	Ray3(Vec3 const& startPos, Vec3 const& endPos);
	Ray3(Vec3 const& startPos, Vec3 const& fwrdNormal, float maxLength);

	Vec3 m_startPos;
	Vec3 m_fwrdNormal;
	float m_maxLength = 1.f;
};

struct RaycastResult3D
{
	bool m_didImpact = false;
	float m_impactDistance = 0.f;
	Vec3 m_impactPos;
	Vec3 m_impactNormal;

	bool operator==(RaycastResult3D const& resultToCompareTo) const;
};

enum class BillboardType
{
	NONE = -1,
	WORLD_UP_FACING,
	WORLD_UP_OPPOSING,
	FULL_FACING,
	FULL_OPPOSING,
	COUNT
};

//--------------------------------------------------------------------
constexpr float pi = 3.14159265358979323846f;

//--------------------------------------------------------------------
//Clamp and lerp
float		GetClamped(float value, float minValue, float maxValue);
int			GetClampedInt(int value, int minValue, int maxValue);
float		GetClampedZeroToOne(float value);
template<typename T>
T Lerp(T start, T end, float t)
{
	return start + (t * (end - start));
}
float		Lerp(float start, float end, float fractionTowardEnd);
Vec2		Lerp(Vec2 const& start, Vec2 const& end, float fractionTowardEnd, bool normalize = false);
Vec3		Lerp(Vec3 const& start, Vec3 const& end, float fractionTowardEnd, bool normalize = false);
float		LerpAngleDegrees(float a, float b, float t);
float		GetFractionWithinRange(float value, float rangeStart, float rangeEnd);
template<typename T>
float GetFractionWithinRange(T value, T rangeStart, T rangeEnd)
{
	return (value - rangeStart) / (rangeEnd - rangeStart);
}
float		GetClampedFractionWithinRange(float value, float rangeStart, float rangeEnd);
float		RangeMapFrom01toNeg11(float value);
float		RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float		RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd);
Vec2		RangeMap(Vec2 const& inValue, Vec2 const& inStart, Vec2 const& inEnd, Vec2 const& outStart, Vec2 const& outEnd);
int			RoundToNearestInt(float value);
int			RoundDownToInt(float value);
template<typename T>
T GetMax(T a, T b)
{
	return a > b ? a : b;
}

float		GetMin(float a, float b);
int			GetMin(int a, int b);
unsigned int GetMin(unsigned int a, unsigned int b);
int			GetGreatestCommonDivisor(int a, int b);

//--------------------------------------------------------------------
//Angle utilities
float ConvertDegreesToRadians(float degrees);
float ConvertRadiansToDegrees(float radians);
float CosDegrees(float degrees);
float SinDegrees(float degrees);
float TanDegrees(float degrees);
float Atan2Degrees(float y, float x);
float AsinDegrees(float x);
float AcosDegrees(float x);
float GetShortestAngularDispDegrees(float startDegrees, float endDegrees);
float GetTurnedTowardDegrees(float currentDegrees, float goalDegrees, float maxDeltaDegrees);
float GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b);
float GetAngleDegreesBetweenVectors3D(Vec3 const& a, Vec3 const& b);

//--------------------------------------------------------------------
//Dot and Cross
//
float DotProduct2D(Vec2 const& a, Vec2 const& b);
float DotProduct3D(Vec3 const& a, Vec3 const& b);
float DotProduct4D(Vec4 const& a, Vec4 const& b);
float CrossProduct2D(Vec2 const& a, Vec2 const& b);
Vec3 CrossProduct3D(Vec3 const& a, Vec3 const& b);

//--------------------------------------------------------------------
//Distance and projections
//
float GetDistance2D(Vec2 const& positionA, Vec2 const& positionB);
float GetDistanceSquared2D(Vec2 const& positionA, Vec2 const& positionB);
float GetDistance3D(Vec3 const& positionA, Vec3 const& positionB);
float GetDistanceSquared3D(Vec3 const& positionA, Vec3 const& positionB);
float GetDistanceXY3D(Vec3 const& positionA, Vec3 const& positionB);
float GetDistanceXYSquared3D(Vec3 const& positionA, Vec3 const& positionB);
int GetTaxicabDistance2D(IntVec2 const& pointA, IntVec2 const& pointB);
float GetProjectedLength2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto); //works if Vecs not normalized
Vec2 const GetProjectedOnto2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto); //works if Vecs not normalized

float GetProjectedLength3D(Vec3 const& vectorToProject, Vec3 const VectorToProjectOnto);
Vec3 const GetProjectedOnto3D(Vec3 const& vectorToProject, Vec3 const& vectorToProjectOnto);

//--------------------------------------------------------------------
//Geometric query utilities
//

//Is point inside ...
bool IsPointInsideDisc2D(Vec2 const& point, Vec2 const& discCenter, float discRadius);
bool IsPointInsideDisc2D(Vec2 const& point, Disc2 const& disc);
bool IsPointInsideAABB2D(Vec2 const& point, AABB2 const& axisAlignedBox);
bool IsPointInsideOBB2D(Vec2 const& point, OBB2 const& orientedBox);
bool IsPointInsideCapsule2D(Vec2 const& point, Vec2 const& boneStart, Vec2 const& boneEnd, float radius);
bool IsPointInsideCapsule2D(Vec2 const& point, Capsule2 const& capsule);
bool IsPointInsideTriangle2D(Vec2 const& point, Vec2 const& triPointACounterClockwise, Vec2 const& triPointBCounterClockwise, Vec2 const& triPointCCounterClockwise); // counter clockwise triangle points
bool IsPointInsideTriangle2D(Vec2 const& point, Triangle2 const& triangle);
bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideDirectedSector2D(Vec2 const& point, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius);

bool IsPointInsideSphere3D(Vec3 const& point, Vec3 const& sphereCenter, float sphereRadius);
bool IsPointInsideZCylinder3D(Vec3 const& point, ZCylinder3D const& cylinder);
bool IsPointInsideAABB3D(Vec3 const& point, AABB3 const& box);
bool IsPointInsideOBB3D(Vec3 const& point, OBB3 const& orientedBox);

//Get Nearest point...
Vec2 GetNearestPointOnDisc2D(Vec2 const& referencePos, Vec2 const& discCenter, float discRadius);
Vec2 GetNearestPointOnDisc2D(Vec2 const& referencePos, Disc2 const& disc);
Vec2 GetNearestPointOnAABB2D(Vec2 const& referencePos, AABB2 const& axisAlignedBox);
Vec2 GetNearestPointOnOBB2D(Vec2 const& referencePos, OBB2 const& orientedBox);
Vec2 GetNearestPointOnInfiniteLine2D(Vec2 const& referencePos, Vec2 const& pointOnLine, Vec2 const& anotherPointOnLine);
Vec2 GetNearestPointOnInfiniteLine2D(Vec2 const& referencePos, LineSegment2 const& infiniteLine);
Vec2 GetNearestPointOnLineSegment(Vec2 const& referencePos, Vec2 const& start, Vec2 const& end);
Vec2 GetNearestPointOnLineSegment(Vec2 const& referencePos, LineSegment2 const& line);
Vec2 GetNearestPointOnCapsule2D(Vec2 const& referencePos, Vec2 const& boneStart, Vec2 const& boneEnd, float const& radius);
Vec2 GetNearestPointOnCapsule2D(Vec2 const& referencePos, Capsule2 const& capsule);
Vec2 GetNearestPointOnTriangle2D(Vec2 const& referencePos, Vec2 const& triPointACounterClockwise, Vec2 const& triPointBCounterClockwise, Vec2 const& triPointCCounterClockwise);
Vec2 GetNearestPointOnTriangle2D(Vec2 const& referencePos, Triangle2 const& triangle);

Vec3 GetNearestPointOnSphere3D(Vec3 const& referencePos, Vec3 const& sphereCenter, float sphereRadius);
Vec3 GetNearestPointOnAABB3D(Vec3 const& referencePos, AABB3 const& box);
Vec3 GetNearestPointOnZCylinder3D(Vec3 const& referencePos, ZCylinder3D const& cylinder);
Vec3 GetNearestPointOnOBB3D(Vec3 const& referencePos, OBB3 const& orientedBox);
Vec3 GetNearestPointOnPlane3D(Vec3 const& referencePos, Plane3D const& plane);

//Overlap functions
bool DoDiscsOverlap(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB);
bool DoAABB2sOverlap(AABB2 const& boxA, AABB2 const& boxB);
bool DoDiscAndAABB2Overlap(Vec2 const& center, float radius, AABB2 const& box);

bool DoSpheresOverlap(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB);
bool DoAABB3sOverlap3D(AABB3 const& boxA, AABB3 const& boxB);
bool DoZCylindersOverlap3D(ZCylinder3D const& cylinderA, ZCylinder3D const& cylinderB);
bool DoSphereAndAABB3Overlap3D(Vec3 const& sphereCenter, float sphereRadius, AABB3 const& box);
bool DoSphereAndZCylinderOverlap3D(Vec3 const& sphereCenter, float sphereRadius, ZCylinder3D const& cylinder);
bool DoZCylinderAndAABB3Overlap3D(ZCylinder3D const& cylinder, AABB3 const& box);
bool DoSphereAndOBB3Overlap3D(Vec3 const& sphereCenter, float sphereRadius, OBB3 const& orientedBox);
bool DoSphereAndPlane3Overlap3D(Vec3 const& sphereCenter, float sphereRadius, Plane3D const& plane);
bool DoAABB3AndPlane3Overlap3D(AABB3 const& box, Plane3D const& plane);
bool DoOBB3AndPlane3Overlap3D(OBB3 const& orientedBox, Plane3D const& plane);

bool DoPlanes2DIntersect(Vec2& out_intersectionPoint, Plane2D const& a, Plane2D const& b);

//Frustrum Functions
bool IsSphereInViewFrustum(Vec3 const& sphereCenter, float sphereRadius, Frustum const& frustum);
bool IsAABB3inViewFrustum(AABB3 const& bounds, Frustum const& frustum);

bool PushDiscOutOfFixedPoint2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedPoint);
bool PushPointOutOfFixedDisc2D(Vec2& point, Vec2 const& fixedDiscCenter, float fixedDiscRadius);
bool PushDiscOutOfFixedDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius);
bool PushDiscsOutOfEachOther2D(Vec2& discACenter, float discARadius, Vec2& discBCenter, float discBRadius);
bool PushDiscOutOfFixedAABB2D(Vec2& mobileDiscCenter, float discRadius, AABB2 const& fixedBox);

bool BounceDiscOutOfFixedPoint2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2& mobileDiscVelocity, Vec2 const& fixedPointCenter, float combinedElasticity);
bool BounceDiscsOutOfEachOther2D(Vec2& discACenter, float discARadius, Vec2& discAVelocity, Vec2& discBCenter, float discBRadius, Vec2& discBVelocity, float combinedElasticity);
bool BounceDiscOutOfFixedDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2& mobileDiscVelocity, Vec2 const& fixedDiscCenter, float fixedDiscRadius, float combinedElasticity);
bool BounceDiscOutOfFixedOBB2D(Vec2& discCenter, float discRadius, Vec2& discVelocity, OBB2 const& orientedBox, float combinedElasticity);
bool BounceDiscOutOfFixedCapsule2D(Vec2& discCenter, float discRadius, Vec2& discVelocity, Capsule2 const& capsule, float combinedElasticity);


//--------------------------------------------------------------------
//Transform utilities
//
void TransformPosition2D(Vec2& posToTransform, float uniformScale, float rotationDegrees, Vec2 const& translation);
void TransformPosition2D(Vec2& posToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);
void TransformPositionXY3D(Vec3& positionToTransform, float scaleXY, float zRotationDegrees, Vec2 const& translationXY);
void TransformPositionXY3D(Vec3& positionToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translationXY);
void TransformPosition3D(Vec3& positionToTransform, Vec3 const& iBasis, Vec3 const& jBasis, Vec3 const& kBasis, Vec3 const& translationIJK);
void TransformPosition3D(Vec3& positionToTransform, Mat44 const& transform);
Mat44 GetBillboardTransform(BillboardType billboardType, Mat44 const& targetTransform, Vec3 const& billboardPosition, Vec2 const& billboardScale = Vec2::ONE);
Mat44 GetLookAtTransform(Vec3 const& pos, Vec3 const& targetPos);
//--------------------------------------------------------------------
//Raycast utilities
//
RaycastResult2D RaycastVsDisc2D(Vec2 const& startPos, Vec2 const& fwrdNormal, float maxDist, Vec2 const& discCenter, float discRadius);
RaycastResult2D RaycastVsDisc2D(Vec2 const& startPos, Vec2 const& fwrdNormal, float maxDist, Disc2 const& disc);
RaycastResult2D RaycastVsTileHeatMap(Vec2 const& startPos, Vec2 const& fwrdNormal, float maxDist, TileHeatMap const& solidMap, float tileSolidValue);
RaycastResult2D RaycastVsTileHeatMap(Ray2 const& ray, TileHeatMap const& solidMap, float tileSolidValue);
RaycastResult2D RaycastVsLineSegment2D(Ray2 const& ray, LineSegment2 const& lineSegment);
RaycastResult2D RaycastVsAABB2D(Ray2 const& ray, AABB2 const& alignedBox);
RaycastResult2D RaycastVsOBB2D(Ray2 const& ray, OBB2 const& orientedBox);
RaycastResult2D RaycastVsPlane2D(Ray2 const& ray, Plane2D const& plane);
RaycastResult2D RaycastVsConvexHull2(Ray2 const& ray, ConvexHull2 const& convexHull);

RaycastResult3D RaycastVsSphere3D(Ray3 const& ray, Vec3 const& sphereCenter, float sphereRadius);
RaycastResult3D RaycastVsAABB3D(Ray3 const& ray, AABB3 const& box);
RaycastResult3D RaycastVsAABB3D(Vec3 const& startPos, Vec3 const& fwrdNormal, float maxDist, AABB3 const& box);
RaycastResult3D RaycastVsAABB3D(Vec3 const& startPos, Vec3 const& endPosition, AABB3 const& box);
RaycastResult3D RaycastVsOBB3D(Ray3 const& ray, OBB3 const& orientedBox);
RaycastResult3D RaycastVsZCylinder3D(Ray3 const& ray, ZCylinder3D const& cylinder);
RaycastResult3D RaycastVsPlane3D(Ray3 const& ray, Plane3D const& plane);

Ray3 GetRayFromMousePosition(Vec2 const& mouseUV, Camera const* camera, float maxLength = -1.f);

//--------------------------------------------------------------------
//Curves
//
float ComputeCubicBezier1D(float A, float B, float C, float D, float t);
float ComputeQuinticBezier1D(float A, float B, float C, float D, float E, float F, float t);

//--------------------------------------------------------------------
//Easing
//
float SmoothStart2(float t);
float SmoothStart3(float t);
float SmoothStart4(float t);
float SmoothStart5(float t);
float SmoothStart6(float t);

float SmoothStop2(float t);
float SmoothStop3(float t);
float SmoothStop4(float t);
float SmoothStop5(float t);
float SmoothStop6(float t);

float SmoothStep3(float t);
float SmoothStep5(float t);

float SmoothStep3Range(float value, float start, float end);

float Hesitate3(float t);
float Hesitate5(float t);

float SmoothMin(float a, float b, float t);
float SmoothMax(float a, float b, float t);

float SmoothPulse3(float value, float start, float riseEnd, float fallStart, float end);


//--------------------------------------------------------------------
//Byte utilities
//
float NormalizeByte(unsigned char byte);
unsigned char DenormalizeByte(float byteFloatValue);