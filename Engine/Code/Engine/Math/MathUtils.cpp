#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Core/TileHeatMap.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/ZCylinder3D.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/Plane3D.hpp"
#include "Engine/Math/Plane2D.hpp"
#include "Engine/Math/ConvexHull2.hpp"
#include "Engine/Renderer/Camera.hpp"
#include <math.h>



//--------------------------------------------------------------------
//Ray constructors
Ray2::Ray2(Vec2 const& startPos, Vec2 const& endPos)
    :m_startPos(startPos)
    ,m_fwrdNormal((endPos - startPos).GetNormalized())
    ,m_maxLength((endPos - startPos).GetLength())
{
}

Ray2::Ray2(Vec2 const& startPos, Vec2 const& fwrdNormal, float maxLength)
    :m_startPos(startPos)
    ,m_fwrdNormal(fwrdNormal)
    ,m_maxLength(maxLength)
{
}

Ray3::Ray3(Vec3 const& startPos, Vec3 const& endPos)
	:m_startPos(startPos)
	,m_fwrdNormal((endPos - startPos).GetNormalized())
	,m_maxLength((endPos - startPos).GetLength())
{
}

Ray3::Ray3(Vec3 const& startPos, Vec3 const& fwrdNormal, float maxLength)
	:m_startPos(startPos)
	,m_fwrdNormal(fwrdNormal)
	,m_maxLength(maxLength)
{
}

bool RaycastResult3D::operator==(RaycastResult3D const& resultToCompareTo) const
{
	return m_didImpact == resultToCompareTo.m_didImpact && m_impactPos == resultToCompareTo.m_impactPos
		&& m_impactDistance == resultToCompareTo.m_impactDistance && m_impactNormal == resultToCompareTo.m_impactNormal;
}


//--------------------------------------------------------------------
//Clamp and lerp
float GetClamped(float value, float minValue, float maxValue)
{
    if (value > maxValue)
    {
        return maxValue;
    }

    else if (value < minValue)
    {
        return minValue;
    }

	return value;
}

int GetClampedInt(int value, int minValue, int maxValue)
{
    if (value > maxValue)
    {
        return maxValue;
    }

    else if (value < minValue)
    {
        return minValue;
    }

	return value;
}

float GetClampedZeroToOne(float value)
{
    if (value > 1.f)
    {
        return 1.f;
    }

    if (value < 0.f)
    {
		return 0.f;

    }

	return value;
}


float Lerp(float start, float end, float fractionTowardEnd)
{
	return start + (fractionTowardEnd * (end - start));
}

Vec2 Lerp(Vec2 const& start, Vec2 const& end, float fractionTowardEnd, bool normalize)
{
	Vec2 pos;
	pos.x = Lerp(start.x, end.x, fractionTowardEnd);
	pos.y = Lerp(start.y, end.y, fractionTowardEnd);
	if (normalize)
	{
		pos.Normalize();
	}

	return pos;
}

Vec3 Lerp(Vec3 const& start, Vec3 const& end, float fractionTowardEnd, bool normalize)
{
	Vec3 pos;
	pos.x = Lerp(start.x, end.x, fractionTowardEnd);
	pos.y = Lerp(start.y, end.y, fractionTowardEnd);
	pos.z = Lerp(start.z, end.z, fractionTowardEnd);

	if (normalize)
	{
		pos.Normalize();
	}

	return pos;
}

float LerpAngleDegrees(float a, float b, float t)
{
	float diff = fmodf(b - a + 540.f, 360.f) - 180.f;
	return a + diff * t;
}


float GetFractionWithinRange(float value, float rangeStart, float rangeEnd)
{
	return (value - rangeStart) / (rangeEnd - rangeStart);
}

float GetClampedFractionWithinRange(float value, float rangeStart, float rangeEnd)
{
	return GetClampedZeroToOne((value - rangeStart) / (rangeEnd - rangeStart));
}

float RangeMapFrom01toNeg11(float value)
{
	return ((value * 2.0f) - 1.0f);
}

float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
    float fraction = GetFractionWithinRange(inValue, inStart, inEnd);
	return Lerp(outStart, outEnd, fraction);
}

float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	float fraction = GetFractionWithinRange(inValue, inStart, inEnd);
    fraction = GetClampedZeroToOne(fraction);
	return Lerp(outStart, outEnd, fraction);
}

Vec2 RangeMap(Vec2 const& inValue, Vec2 const& inStart, Vec2 const& inEnd, Vec2 const& outStart, Vec2 const& outEnd)
{
	float x = RangeMap(inValue.x, inStart.x, inEnd.x, outStart.x, outEnd.x);
	float y = RangeMap(inValue.y, inStart.y, inEnd.y, outStart.y, outEnd.y);
	return Vec2(x,y);
}

int RoundToNearestInt(float value)
{
	return (int)floorf(value + 0.5f);
}

int RoundDownToInt(float value)
{
	return (int)(floorf(value));
}



float GetMin(float a, float b)
{
	return a < b ? a : b;
}

int GetMin(int a, int b)
{
	return a < b ? a : b;
}

unsigned int GetMin(unsigned int a, unsigned int b)
{
	return a < b ? a : b;
}

int GetGreatestCommonDivisor(int a, int b)
{
	a = (a < 0) ? -a : a;
	b = (b < 0) ? -b : b;
	while (b != 0)
	{
		int t = (a % b);
		a = b;
		b = t;
	}
	return a;
}

//--------------------------------------------------------------------
//Angle utilities
float ConvertDegreesToRadians(float degrees)
{
    return degrees * (pi / 180.f);
}

float ConvertRadiansToDegrees(float radians)
{
    return radians * (180.f/ pi);
}

float CosDegrees(float degrees)
{
    float radians = ConvertDegreesToRadians(degrees);
    return cosf(radians);
}

float SinDegrees(float degrees)
{
    float radians = ConvertDegreesToRadians(degrees);
    return sinf(radians);
}

float TanDegrees(float degrees)
{
	float radians = ConvertDegreesToRadians(degrees);
	return tanf(radians);
}

float Atan2Degrees(float y, float x)
{
    float radians = atan2f(y, x);
    return ConvertRadiansToDegrees(radians);
}

float AsinDegrees(float x)
{
	float radians = asinf(x);
	return ConvertRadiansToDegrees(radians);
}

float AcosDegrees(float x)
{
	float radians = acosf(x);
	return ConvertRadiansToDegrees(radians);
}

float GetShortestAngularDispDegrees(float startDegrees, float endDegrees)
{
    float disp = endDegrees - startDegrees;

    while (disp > 180.f)
    {
        disp -= 360.f;
    }

    while (disp < -180.f)
    {
        disp += 360.f;
    }

    return disp;
}

float GetTurnedTowardDegrees(float currentDegrees, float goalDegrees, float maxDeltaDegrees)
{
    float angularDispDeg = GetShortestAngularDispDegrees(currentDegrees, goalDegrees);
    if (fabsf(angularDispDeg) < maxDeltaDegrees)
    {
        return goalDegrees;
    }

    if (angularDispDeg > 0.f)
    {
		return currentDegrees + maxDeltaDegrees;

    }
    
    return currentDegrees - maxDeltaDegrees;
}

float GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b)
{
    float cosOfAngle = DotProduct2D(a.GetNormalized(), b.GetNormalized());
    cosOfAngle = GetClamped(cosOfAngle, -1.f, 1.f);
	return ConvertRadiansToDegrees(acosf(cosOfAngle));
}

float GetAngleDegreesBetweenVectors3D(Vec3 const& a, Vec3 const& b)
{
	float cosOfAngle = DotProduct3D(a.GetNormalized(), b.GetNormalized());
	cosOfAngle = GetClamped(cosOfAngle, -1.f, 1.f);
	return ConvertRadiansToDegrees(acosf(cosOfAngle));
}

float DotProduct2D(Vec2 const& a, Vec2 const& b)
{
    return (a.x * b.x) + (a.y * b.y);
}

float DotProduct3D(Vec3 const& a, Vec3 const& b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

float DotProduct4D(Vec4 const& a, Vec4 const& b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}

float CrossProduct2D(Vec2 const& a, Vec2 const& b)
{
	return (a.x  * b.y) - (a.y * b.x);
}

Vec3 CrossProduct3D(Vec3 const& a, Vec3 const& b)
{
	return Vec3((a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x));
}

//--------------------------------------------------------------------
//Basic 2D & 3D utilities
//
float GetDistance2D(Vec2 const& positionA, Vec2 const& positionB)
{
    float deltaX = positionB.x - positionA.x;
    float deltaY = positionB.y - positionA.y;
    
    return sqrtf((deltaX * deltaX) + (deltaY * deltaY));
}

float GetDistanceSquared2D(Vec2 const& positionA, Vec2 const& positionB)
{
    float deltaX = positionB.x - positionA.x;
    float deltaY = positionB.y - positionA.y;
    
    return (deltaX * deltaX) + (deltaY * deltaY);
}

float GetDistance3D(Vec3 const& positionA, Vec3 const& positionB)
{
    float deltaX = positionB.x - positionA.x;
    float deltaY = positionB.y - positionA.y;
    float deltaZ = positionB.z - positionA.z;
    
    return sqrtf((deltaX * deltaX) + (deltaY * deltaY) + (deltaZ * deltaZ));
}

float GetDistanceSquared3D(Vec3 const& positionA, Vec3 const& positionB)
{
    float deltaX = positionB.x - positionA.x;
    float deltaY = positionB.y - positionA.y;
    float deltaZ = positionB.z - positionA.z;

    return (deltaX * deltaX) + (deltaY * deltaY) + (deltaZ * deltaZ);
}

float GetDistanceXY3D(Vec3 const& positionA, Vec3 const& positionB)
{
    float deltaX = positionB.x - positionA.x;
    float deltaY = positionB.y - positionA.y;

    return sqrtf((deltaX * deltaX) + (deltaY * deltaY));
}

float GetDistanceXYSquared3D(Vec3 const& positionA, Vec3 const& positionB)
{
    float deltaX = positionB.x - positionA.x;
    float deltaY = positionB.y - positionA.y;

    return (deltaX * deltaX) + (deltaY * deltaY);
}

int GetTaxicabDistance2D(IntVec2 const& pointA, IntVec2 const& pointB)
{
    IntVec2 direction = pointB - pointA;
    return direction.GetTaxicabLength();
}

float GetProjectedLength2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto)
{
    return DotProduct2D(vectorToProject, vectorToProjectOnto.GetNormalized());
}

Vec2 const GetProjectedOnto2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto)
{
    Vec2 normalizedVecToProjectOnto = vectorToProjectOnto.GetNormalized();
    return normalizedVecToProjectOnto * DotProduct2D(vectorToProject, normalizedVecToProjectOnto);
}

float GetProjectedLength3D(Vec3 const& vectorToProject, Vec3 const VectorToProjectOnto)
{
	return DotProduct3D(vectorToProject, VectorToProjectOnto.GetNormalized());
}

Vec3 const GetProjectedOnto3D(Vec3 const& vectorToProject, Vec3 const& vectorToProjectOnto)
{
	Vec3 normalizedVecToProjectOnto = vectorToProjectOnto.GetNormalized();
	return normalizedVecToProjectOnto * DotProduct3D(vectorToProject, normalizedVecToProjectOnto);
}


bool IsPointInsideDisc2D(Vec2 const& point, Vec2 const& discCenter, float discRadius)
{
    Vec2 directionFromDiscCenterToPoint = point - discCenter;
    return directionFromDiscCenterToPoint.GetLengthSquared() < (discRadius * discRadius);
}

bool IsPointInsideDisc2D(Vec2 const& point, Disc2 const& disc)
{
	Vec2 directionFromDiscCenterToPoint = point - disc.m_center;
	return directionFromDiscCenterToPoint.GetLengthSquared() < (disc.m_radius * disc.m_radius);
}

bool IsPointInsideAABB2D(Vec2 const& point, AABB2 const& axisAlignedBox)
{
	if (point.x < axisAlignedBox.m_mins.x || point.x > axisAlignedBox.m_maxs.x
		|| point.y < axisAlignedBox.m_mins.y || point.y > axisAlignedBox.m_maxs.y)
	{
		return false;
	}

	else
		return true;
}

bool IsPointInsideOBB2D(Vec2 const& point, OBB2 const& orientedBox)
{
    Vec2 displacement = point - orientedBox.m_center;
    float iDispPercent = DotProduct2D(displacement, orientedBox.m_iBasisNormal);

    Vec2 jBasisNormal = Vec2(-orientedBox.m_iBasisNormal.y, orientedBox.m_iBasisNormal.x);
    float jDispPercent = DotProduct2D(displacement, jBasisNormal);

    float halfWidth = orientedBox.m_halfDimensionsIJ.x;
    float halfHeight = orientedBox.m_halfDimensionsIJ.y;

    if(iDispPercent >= halfWidth || iDispPercent <= -halfWidth)
        return false;

    if (jDispPercent >= halfHeight || jDispPercent <= -halfHeight)
        return false;

    return true;
}

bool IsPointInsideCapsule2D(Vec2 const& point, Vec2 const& boneStart, Vec2 const& boneEnd, float radius)
{
    Vec2 displacement = boneEnd - boneStart;
    Vec2 nearestPoint = GetNearestPointOnLineSegment(point, boneStart, boneEnd);

    if (GetDistanceSquared2D(point, nearestPoint) < (radius * radius))
        return true;

    return false;
}

bool IsPointInsideCapsule2D(Vec2 const& point, Capsule2 const& capsule)
{
	Vec2 displacement = capsule.m_end - capsule.m_start;
	Vec2 nearestPoint = GetNearestPointOnLineSegment(point, capsule.m_start, capsule.m_end);

	if (GetDistanceSquared2D(point, nearestPoint) < (capsule.m_radius * capsule.m_radius))
		return true;

	return false;
}

bool IsPointInsideTriangle2D(Vec2 const& point, Vec2 const& triPointACounterClockwise, Vec2 const& triPointBCounterClockwise, Vec2 const& triPointCCounterClockwise)
{
    Vec2 dispAToPoint = point - triPointACounterClockwise;                  //Vector from input point to first point on triangle
    Vec2 dispAToB = triPointBCounterClockwise - triPointACounterClockwise;  //Vector from first point to second point on triangle
    Vec2 jVecAToB = dispAToB.GetRotated90Degrees();                         //jVector of dispAToB

    if (DotProduct2D(dispAToPoint, jVecAToB) <= 0.f)
       return false;

    Vec2 dispBToPoint = point - triPointBCounterClockwise;
    Vec2 dispBToC = triPointCCounterClockwise - triPointBCounterClockwise;
    Vec2 jVecBToC = dispBToC.GetRotated90Degrees();

    if(DotProduct2D(dispBToPoint, jVecBToC) <= 0.f)
        return false;

    Vec2 dispCToPoint = point - triPointCCounterClockwise;
    Vec2 dispCToA = triPointACounterClockwise - triPointCCounterClockwise;
    Vec2 jVecCToA = dispCToA.GetRotated90Degrees();

    if (DotProduct2D(dispCToPoint, jVecCToA) <= 0.f)
        return false;

    return true;
}

bool IsPointInsideTriangle2D(Vec2 const& point, Triangle2 const& triangle)
{
    Vec2 triPointACounterClockwise = triangle.m_pointsCounterClockwise[0];
    Vec2 triPointBCounterClockwise = triangle.m_pointsCounterClockwise[1];
    Vec2 triPointCCounterClockwise = triangle.m_pointsCounterClockwise[2];

	Vec2 dispAToPoint = point - triPointACounterClockwise; //Vector from input point to first point on triangle
	Vec2 dispAToB = triPointBCounterClockwise - triPointACounterClockwise; //Vector from first point to second point on triangle
	Vec2 jVecAToB = dispAToB.GetRotated90Degrees(); //jVector of dispAToB

	if (DotProduct2D(dispAToPoint, jVecAToB) <= 0.f)
		return false;

	Vec2 dispBToPoint = point - triPointBCounterClockwise;
	Vec2 dispBToC = triPointCCounterClockwise - triPointBCounterClockwise;
	Vec2 jVecBToC = dispBToC.GetRotated90Degrees();

	if (DotProduct2D(dispBToPoint, jVecBToC) <= 0.f)
		return false;

	Vec2 dispCToPoint = point - triPointCCounterClockwise;
	Vec2 dispCToA = triPointACounterClockwise - triPointCCounterClockwise;
	Vec2 jVecCToA = dispCToA.GetRotated90Degrees();

	if (DotProduct2D(dispCToPoint, jVecCToA) <= 0.f)
		return false;

	return true;
}


bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius)
{
    float distanceSqrd = GetDistanceSquared2D(point, sectorTip);
    if(distanceSqrd >= (sectorRadius * sectorRadius))
        return false;

    Vec2 displacement = point - sectorTip;
    Vec2 sectorFwrd = Vec2::MakeFromPolarDegrees(sectorForwardDegrees);
    float angleDifference = GetAngleDegreesBetweenVectors2D(displacement, sectorFwrd);

    if (angleDifference > (sectorApertureDegrees * 0.5f))
        return false;

    return true;

}

bool IsPointInsideDirectedSector2D(Vec2 const& point, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius)
{
	float distanceSqrd = GetDistanceSquared2D(point, sectorTip);
	if (distanceSqrd >= (sectorRadius * sectorRadius))
		return false;

	Vec2 displacement = point - sectorTip;
	float angleDifference = GetAngleDegreesBetweenVectors2D(displacement, sectorForwardNormal);

	if (angleDifference > (sectorApertureDegrees * 0.5f))
		return false;

	return true;
}

bool IsPointInsideSphere3D(Vec3 const& point, Vec3 const& sphereCenter, float sphereRadius)
{
	Vec3 displacement = point - sphereCenter;
	return displacement.GetLengthSquared() < (sphereRadius * sphereRadius);
}

bool IsPointInsideZCylinder3D(Vec3 const& point, ZCylinder3D const& cylinder)
{
	if (!cylinder.m_zRange.IsWithinRange(point.z))
		return false;

	Vec2 pointXY(point.x, point.y);
	return (GetDistanceSquared2D(pointXY, cylinder.m_centerXY) < (cylinder.m_radius * cylinder.m_radius));
}

bool IsPointInsideAABB3D(Vec3 const& point, AABB3 const& box)
{
	return (point.x > box.m_mins.x && point.x < box.m_maxs.x
		&& point.y > box.m_mins.y && point.y < box.m_maxs.y
		&& point.z > box.m_mins.z && point.z < box.m_maxs.z);
}

bool IsPointInsideOBB3D(Vec3 const& point, OBB3 const& orientedBox)
{
	Vec3 displacement = point - orientedBox.m_center;
	float iDispPercent = DotProduct3D(displacement, orientedBox.m_iBasis);


	float halfDepth = orientedBox.m_halfDimensionsIJK.x;
	float halfWidth = orientedBox.m_halfDimensionsIJK.y;
	float halfHeight = orientedBox.m_halfDimensionsIJK.z;

	if (iDispPercent >= halfDepth || iDispPercent <= -halfDepth)
		return false;

	float jDispPercent = DotProduct3D(displacement, orientedBox.m_jBasis);
	if (jDispPercent >= halfWidth || jDispPercent <= -halfWidth)
		return false;

	float kDispPercent = DotProduct3D(displacement, orientedBox.m_kBasis);
	if (kDispPercent >= halfHeight || kDispPercent <= -halfHeight)
		return false;

	return true;
}

//--------------------------------------------------------------------
//Geometric query utilities
//
bool DoDiscsOverlap(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB)
{
    float distanceBetweenCenters2D = GetDistanceSquared2D(centerA, centerB);
    float radiiSquared = (radiusA + radiusB) * (radiusA + radiusB);

    return (distanceBetweenCenters2D <= radiiSquared);
}

bool DoSpheresOverlap(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB)
{
    float distanceBetweenCenters3D = GetDistanceSquared3D(centerA, centerB);
    float radiiSquared = (radiusA + radiusB) * (radiusA + radiusB);

    return (distanceBetweenCenters3D <= radiiSquared);
}

bool DoAABB3sOverlap3D(AABB3 const& boxA, AABB3 const& boxB)
{
	FloatRange boxAXs(boxA.m_mins.x, boxA.m_maxs.x);
	FloatRange boxAYs(boxA.m_mins.y, boxA.m_maxs.y);
	FloatRange boxAZs(boxA.m_mins.z, boxA.m_maxs.z);
	FloatRange boxBXs(boxB.m_mins.x, boxB.m_maxs.x);
	FloatRange boxBYs(boxB.m_mins.y, boxB.m_maxs.y);
	FloatRange boxBZs(boxB.m_mins.z, boxB.m_maxs.z);

	return (boxAXs.IsOverlapping(boxBXs) && boxAYs.IsOverlapping(boxBYs) && boxAZs.IsOverlapping(boxBZs));
}

bool DoZCylindersOverlap3D(ZCylinder3D const& cylinderA, ZCylinder3D const& cylinderB)
{
	if(!cylinderA.m_zRange.IsOverlapping(cylinderB.m_zRange))
		return false;

	return DoDiscsOverlap(cylinderA.m_centerXY, cylinderA.m_radius, cylinderB.m_centerXY, cylinderB.m_radius);
}

bool DoSphereAndAABB3Overlap3D(Vec3 const& sphereCenter, float sphereRadius, AABB3 const& box)
{
	Vec3 nearestPointOnBox = box.GetNearestPoint(sphereCenter);
	return GetDistanceSquared3D(nearestPointOnBox, sphereCenter) < (sphereRadius * sphereRadius);
}

bool DoSphereAndZCylinderOverlap3D(Vec3 const& sphereCenter, float sphereRadius, ZCylinder3D const& cylinder)
{
	FloatRange zComparisonRange(cylinder.m_zRange.m_min - sphereRadius, cylinder.m_zRange.m_max + sphereRadius);
	if(!zComparisonRange.IsWithinRange(sphereCenter.z))
		return false;

	Vec3 nearestPoint = GetNearestPointOnZCylinder3D(sphereCenter, cylinder);
	return (GetDistanceSquared3D(sphereCenter, nearestPoint) < (sphereRadius * sphereRadius));
}

bool DoZCylinderAndAABB3Overlap3D(ZCylinder3D const& cylinder, AABB3 const& box)
{
	FloatRange boxZRange(box.m_mins.z, box.m_maxs.z);
	if(!cylinder.m_zRange.IsOverlapping(boxZRange))
		return false;

	AABB2 box2D = AABB2(Vec2(box.m_mins.x, box.m_mins.y), Vec2(box.m_maxs.x, box.m_maxs.y));
	Vec2 nearestPoint = box2D.GetNearestPoint(cylinder.m_centerXY);
	return IsPointInsideDisc2D(nearestPoint, cylinder.m_centerXY, cylinder.m_radius);
}

bool DoSphereAndOBB3Overlap3D(Vec3 const& sphereCenter, float sphereRadius, OBB3 const& orientedBox)
{
	Vec3 nearestPoint = GetNearestPointOnOBB3D(sphereCenter, orientedBox);
	if(GetDistanceSquared3D(sphereCenter, nearestPoint) < (sphereRadius * sphereRadius))
		return true;

	return false;
}

bool DoSphereAndPlane3Overlap3D(Vec3 const& sphereCenter, float sphereRadius, Plane3D const& plane)
{
	Vec3 nearestPoint = GetNearestPointOnPlane3D(sphereCenter, plane);
	return IsPointInsideSphere3D(nearestPoint, sphereCenter, sphereRadius);
}

bool DoAABB3AndPlane3Overlap3D(AABB3 const& box, Plane3D const& plane)
{
	Vec3 bL = box.m_mins;
	float altitudeBL = plane.GetAltitudeFromPoint(bL);

	Vec3 corner = box.m_maxs;
	float altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	corner = Vec3(box.m_maxs.x, box.m_mins.y, box.m_mins.z);
	altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	corner = Vec3(box.m_mins.x, box.m_maxs.y, box.m_mins.z);
	altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	corner = Vec3(box.m_mins.x, box.m_mins.y, box.m_maxs.z);
	altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	corner = Vec3(box.m_maxs.x, box.m_maxs.y, box.m_mins.z);
	 altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	corner = Vec3(box.m_maxs.x, box.m_mins.y, box.m_maxs.z);
	 altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	corner = Vec3(box.m_mins.x, box.m_maxs.y, box.m_maxs.z);
	 altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	return false;
}

bool DoOBB3AndPlane3Overlap3D(OBB3 const& orientedBox, Plane3D const& plane)
{
	Vec3 fwrd = orientedBox.m_iBasis;
	Vec3 left = orientedBox.m_jBasis;
	Vec3 up = orientedBox.m_kBasis;

	float halfDepth = orientedBox.m_halfDimensionsIJK.x;
	float halfWidth = orientedBox.m_halfDimensionsIJK.y;
	float halfHeight = orientedBox.m_halfDimensionsIJK.z;

	Mat44 boxTransform = Mat44(fwrd, left, up, orientedBox.m_center);
	Vec3 bL = boxTransform.TransformPosition3D(-orientedBox.m_halfDimensionsIJK);
	float altitudeBL = plane.GetAltitudeFromPoint(bL);

	Vec3 corner = boxTransform.TransformPosition3D(orientedBox.m_halfDimensionsIJK);
	float altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	corner = boxTransform.TransformPosition3D(Vec3(halfDepth, -halfWidth, -halfHeight));
	altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	corner = boxTransform.TransformPosition3D(Vec3(-halfDepth, halfWidth, halfHeight));
	altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	corner = boxTransform.TransformPosition3D(Vec3(-halfDepth, halfWidth, -halfHeight));
	altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	corner = boxTransform.TransformPosition3D(Vec3(halfDepth, -halfWidth, halfHeight));
	altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	corner = boxTransform.TransformPosition3D(Vec3(-halfDepth, -halfWidth, halfHeight));
	altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	corner = boxTransform.TransformPosition3D(Vec3(halfDepth, halfWidth, -halfHeight));
	altitude = plane.GetAltitudeFromPoint(corner);
	if (altitude * altitudeBL < 0.f)
		return true;

	return false;
}

bool DoPlanes2DIntersect(Vec2& out_intersectionPoint, Plane2D const& a, Plane2D const& b)
{
	return a.DoesPlaneIntersectWithPlane(out_intersectionPoint, b);
}

bool IsSphereInViewFrustum(Vec3 const& sphereCenter, float sphereRadius, Frustum const& frustrum)
{
	constexpr int NUM_PLANES = 6;
	for (int i = 0; i < NUM_PLANES; ++i)
	{
		Plane3D currentPlane = frustrum.m_planes[i];
		Vec3 referencePos = sphereCenter - (currentPlane.m_normal * sphereRadius);
		if(currentPlane.IsPointInFrontOf(referencePos))
			return false;
	}

	return true;
}

bool IsAABB3inViewFrustum(AABB3 const& bounds, Frustum const& frustum)
{
	Vec3 const boxCenter = ((bounds.m_mins + bounds.m_maxs) * 0.5f);
	Vec3 const boxExtents = ((bounds.m_maxs - bounds.m_mins) * 0.5f);

	for (int planeIndex = 0; planeIndex < 6; ++planeIndex)
	{
		Plane3D const& plane = frustum.m_planes[planeIndex];
		Vec3 const unitNormal = plane.m_normal.GetNormalized();

		float const projectedRadius =
			(fabsf(unitNormal.x) * boxExtents.x) +
			(fabsf(unitNormal.y) * boxExtents.y) +
			(fabsf(unitNormal.z) * boxExtents.z);

		float const signedDistanceToCenter = DotProduct3D(unitNormal, boxCenter) - plane.m_distanceAlongNormal;

		if (signedDistanceToCenter > projectedRadius)
		{
			return false;
		}
	}

	return true;
}


bool DoAABB2sOverlap(AABB2 const& boxA, AABB2 const& boxB)
{
	FloatRange boxAXs(boxA.m_mins.x, boxA.m_maxs.x);
	FloatRange boxAYs(boxA.m_mins.y, boxA.m_maxs.y);
	FloatRange boxBXs(boxB.m_mins.x, boxB.m_maxs.x);
	FloatRange boxBYs(boxB.m_mins.y, boxB.m_maxs.y);

	if (boxAXs.IsOverlapping(boxBXs) && boxAYs.IsOverlapping(boxBYs))
		return true;

	return false;
}

bool DoDiscAndAABB2Overlap(Vec2 const& center, float radius, AABB2 const& box)
{
	Vec2 nearestPoint = box.GetNearestPoint(center);
	return GetDistanceSquared2D(center, nearestPoint) < (radius * radius);
}

Vec2 GetNearestPointOnDisc2D(Vec2 const& referencePos, Vec2 const& discCenter, float discRadius)
{
    Vec2 displacement = referencePos - discCenter;
    displacement.ClampLength(discRadius);
    return discCenter + displacement;
}

Vec2 GetNearestPointOnDisc2D(Vec2 const& referencePos, Disc2 const& disc)
{
	Vec2 displacement = referencePos - disc.m_center;
	displacement.ClampLength(disc.m_radius);
	return disc.m_center + displacement;
}

Vec2 GetNearestPointOnAABB2D(Vec2 const& referencePos, AABB2 const& axisAlignedBox)
{
	float nearestX = GetClamped(referencePos.x, axisAlignedBox.m_mins.x, axisAlignedBox.m_maxs.x);
	float nearestY = GetClamped(referencePos.y, axisAlignedBox.m_mins.y, axisAlignedBox.m_maxs.y);
	return Vec2(nearestX, nearestY);
}

Vec2 GetNearestPointOnOBB2D(Vec2 const& referencePos, OBB2 const& orientedBox)
{
	Vec2 displacement = referencePos - orientedBox.m_center;
	float iDispPercent = DotProduct2D(displacement, orientedBox.m_iBasisNormal);

	Vec2 jBasisNormal = Vec2(-orientedBox.m_iBasisNormal.y, orientedBox.m_iBasisNormal.x);
	float jDispPercent = DotProduct2D(displacement, jBasisNormal);

	float halfWidth = orientedBox.m_halfDimensionsIJ.x;
	float halfHeight = orientedBox.m_halfDimensionsIJ.y;

	if (iDispPercent < halfWidth && iDispPercent > -halfWidth && jDispPercent < halfHeight && jDispPercent > -halfHeight)
		return referencePos; // point is inside

    float nearestPointI = GetClamped(iDispPercent, -orientedBox.m_halfDimensionsIJ.x, orientedBox.m_halfDimensionsIJ.x);
    float nearestPointJ = GetClamped(jDispPercent, -orientedBox.m_halfDimensionsIJ.y, orientedBox.m_halfDimensionsIJ.y);

    Vec2 nearestPointXY = orientedBox.m_center + (nearestPointI * orientedBox.m_iBasisNormal) + (nearestPointJ * jBasisNormal);
    return nearestPointXY;
}

Vec2 GetNearestPointOnInfiniteLine2D(Vec2 const& referencePos, Vec2 const& pointOnLine, Vec2 const& anotherPointOnLine)
{
    Vec2 dispStartToEnd = anotherPointOnLine - pointOnLine;
    Vec2 dispStartToPoint = referencePos - pointOnLine;

    Vec2 startToNearestPoint = GetProjectedOnto2D(dispStartToPoint, dispStartToEnd);
    Vec2 nearestPoint = pointOnLine + startToNearestPoint;

    return nearestPoint;
}

Vec2 GetNearestPointOnInfiniteLine2D(Vec2 const& referencePos, LineSegment2 const& infiniteLine)
{
	Vec2 dispStartToEnd = infiniteLine.m_end - infiniteLine.m_start;
	Vec2 dispStartToPoint = referencePos - infiniteLine.m_start;

	Vec2 startToNearestPoint = GetProjectedOnto2D(dispStartToPoint, dispStartToEnd);
	Vec2 nearestPoint = infiniteLine.m_start + startToNearestPoint;

	return nearestPoint;
}

Vec2 GetNearestPointOnLineSegment(Vec2 const& referencePos, Vec2 const& start, Vec2 const& end)
{
    Vec2 dispStartToPoint = referencePos - start;
    Vec2 dispStartToEnd = end - start;

    if (DotProduct2D(dispStartToPoint, dispStartToEnd) <= 0.f)
        return start;

    Vec2 dispEndToPoint = referencePos - end;
	if (DotProduct2D(dispEndToPoint, dispStartToEnd) >= 0.f)
		return end;

    Vec2 startToNearestPoint = GetProjectedOnto2D(dispStartToPoint, dispStartToEnd);
    Vec2 nearestPoint = start + startToNearestPoint;

    return nearestPoint;
}

Vec2 GetNearestPointOnLineSegment(Vec2 const& referencePos, LineSegment2 const& line)
{
	Vec2 dispStartToPoint = referencePos - line.m_start;
	Vec2 dispStartToEnd = line.m_end - line.m_start;

	if (DotProduct2D(dispStartToPoint, dispStartToEnd) <= 0.f)
		return line.m_start;

	Vec2 dispEndToPoint = referencePos - line.m_end;
	if (DotProduct2D(dispEndToPoint, dispStartToEnd) >= 0.f)
		return line.m_end;

	Vec2 startToNearestPoint = GetProjectedOnto2D(dispStartToPoint, dispStartToEnd);
	Vec2 nearestPoint = line.m_start + startToNearestPoint;

	return nearestPoint;
}

Vec2 GetNearestPointOnCapsule2D(Vec2 const& referencePos, Vec2 const& boneStart, Vec2 const& boneEnd, float const& radius)
{

	Vec2 displacement = boneEnd - boneStart;
	Vec2 nearestPointOnBone = GetNearestPointOnLineSegment(referencePos, boneStart, boneEnd);

	if (GetDistanceSquared2D(referencePos, nearestPointOnBone) < (radius * radius))
		return referencePos; //point is inside

    displacement = referencePos - nearestPointOnBone;
    displacement.ClampLength(radius);
    return nearestPointOnBone + displacement;
}

Vec2 GetNearestPointOnCapsule2D(Vec2 const& referencePos, Capsule2 const& capsule)
{
	Vec2 displacement = capsule.m_end - capsule.m_start;
	Vec2 nearestPointOnBone = GetNearestPointOnLineSegment(referencePos, capsule.m_start, capsule.m_end);

	if (GetDistanceSquared2D(referencePos, nearestPointOnBone) < (capsule.m_radius * capsule.m_radius))
		return referencePos; //point is inside

	displacement = referencePos - nearestPointOnBone;
	displacement.ClampLength(capsule.m_radius);
	return nearestPointOnBone + displacement;
}

Vec2 GetNearestPointOnTriangle2D(Vec2 const& referencePos, Vec2 const& triPointACounterClockwise, Vec2 const& triPointBCounterClockwise, Vec2 const& triPointCCounterClockwise)
{
	//#TODO: could be optomized to use voronoi regions

	if (IsPointInsideTriangle2D(referencePos, triPointACounterClockwise, triPointBCounterClockwise, triPointCCounterClockwise))
	{
		return referencePos;
	}

    Vec2 nearestPointAToB = GetNearestPointOnLineSegment(referencePos, triPointACounterClockwise, triPointBCounterClockwise);
    Vec2 nearestPointBToC = GetNearestPointOnLineSegment(referencePos, triPointBCounterClockwise, triPointCCounterClockwise);

    
    //if both AtoB and BtoC segments find pointB as the nearest point, then there is no need to try CtoA, the referencePos is by the B corner
    if (nearestPointAToB == triPointBCounterClockwise && nearestPointBToC == triPointBCounterClockwise)
    {
        return triPointBCounterClockwise;
    }
        
    Vec2 nearestPointCToA = GetNearestPointOnLineSegment(referencePos, triPointCCounterClockwise, triPointACounterClockwise);

    //Check to see if the nearest point is one of the corners
    if (nearestPointAToB == triPointACounterClockwise && nearestPointCToA == triPointACounterClockwise)
    {
        return triPointACounterClockwise;
    }

    if (nearestPointBToC == triPointCCounterClockwise && nearestPointCToA == triPointCCounterClockwise)
    {
        return triPointCCounterClockwise;
    }
    

    //Find distance to all three nearest points and compare
    float distanceToNAB = GetDistanceSquared2D(referencePos, nearestPointAToB);
    float distanceToNBC = GetDistanceSquared2D(referencePos, nearestPointBToC);
    float distanceToNCA = GetDistanceSquared2D(referencePos, nearestPointCToA);

    float smallestDistance = distanceToNAB;
    Vec2 nearestPoint = nearestPointAToB;

	if (distanceToNBC < smallestDistance)
	{
		nearestPoint = nearestPointBToC;
		smallestDistance = distanceToNBC;
	}

	if (distanceToNCA < smallestDistance)
	{
		nearestPoint = nearestPointCToA;
		smallestDistance = distanceToNCA;
	}
    
    return nearestPoint;
}

Vec2 GetNearestPointOnTriangle2D(Vec2 const& referencePos, Triangle2 const& triangle)
{
	Vec2 triPointACounterClockwise = triangle.m_pointsCounterClockwise[0];
	Vec2 triPointBCounterClockwise = triangle.m_pointsCounterClockwise[1];
	Vec2 triPointCCounterClockwise = triangle.m_pointsCounterClockwise[2];

	if (IsPointInsideTriangle2D(referencePos, triPointACounterClockwise, triPointBCounterClockwise, triPointCCounterClockwise))
	{
		return referencePos;
	}

	Vec2 nearestPointAToB = GetNearestPointOnLineSegment(referencePos, triPointACounterClockwise, triPointBCounterClockwise);
	Vec2 nearestPointBToC = GetNearestPointOnLineSegment(referencePos, triPointBCounterClockwise, triPointCCounterClockwise);

	//if both AtoB and BtoC segments find pointB as the nearest point, then there is no need to try CtoA, the referencePos is by the B corner
	if (nearestPointAToB == triPointBCounterClockwise && nearestPointBToC == triPointBCounterClockwise)
	{
		return triPointBCounterClockwise;
	}

	Vec2 nearestPointCToA = GetNearestPointOnLineSegment(referencePos, triPointCCounterClockwise, triPointACounterClockwise);

	//Check to see if the nearest point is one of the corners
	if (nearestPointAToB == triPointACounterClockwise && nearestPointCToA == triPointACounterClockwise)
	{
		return triPointACounterClockwise;
	}

	if (nearestPointBToC == triPointCCounterClockwise && nearestPointCToA == triPointCCounterClockwise)
	{
		return triPointCCounterClockwise;
	}

	//Find distance to all three nearest points and compare
	float distanceToNAB = GetDistanceSquared2D(referencePos, nearestPointAToB);
	float distanceToNBC = GetDistanceSquared2D(referencePos, nearestPointBToC);
	float distanceToNCA = GetDistanceSquared2D(referencePos, nearestPointCToA);

	float smallestDistance = distanceToNAB;
	Vec2 nearestPoint = nearestPointAToB;

	if (distanceToNBC < smallestDistance)
	{
		nearestPoint = nearestPointBToC;
        smallestDistance = distanceToNBC;
	}

	if (distanceToNCA < smallestDistance)
	{
		nearestPoint = nearestPointCToA;
        smallestDistance = distanceToNCA;
	}

	return nearestPoint;
}

Vec3 GetNearestPointOnSphere3D(Vec3 const& referencePos, Vec3 const& sphereCenter, float sphereRadius)
{
	Vec3 displacement = referencePos - sphereCenter;
	displacement.ClampLength(sphereRadius);
	return sphereCenter + displacement;
}

Vec3 GetNearestPointOnAABB3D(Vec3 const& referencePos, AABB3 const& box)
{
	float nearestX = GetClamped(referencePos.x, box.m_mins.x, box.m_maxs.x);
	float nearestY = GetClamped(referencePos.y, box.m_mins.y, box.m_maxs.y);
	float nearestZ = GetClamped(referencePos.z, box.m_mins.z, box.m_maxs.z);
	return Vec3(nearestX, nearestY, nearestZ);
}

Vec3 GetNearestPointOnZCylinder3D(Vec3 const& referencePos, ZCylinder3D const& cylinder)
{
	Vec2 refPosXY(referencePos.x, referencePos.y);
	Vec2 nearestPointXY = GetNearestPointOnDisc2D(refPosXY, cylinder.m_centerXY, cylinder.m_radius);
	float nearestPointZ = GetClamped(referencePos.z, cylinder.m_zRange.m_min, cylinder.m_zRange.m_max);
	return Vec3(nearestPointXY.x, nearestPointXY.y, nearestPointZ);
}

Vec3 GetNearestPointOnOBB3D(Vec3 const& referencePos, OBB3 const& orientedBox)
{
	Vec3 displacement = referencePos - orientedBox.m_center;
	float iDispPercent = DotProduct3D(displacement, orientedBox.m_iBasis);

	float jDispPercent = DotProduct3D(displacement, orientedBox.m_jBasis);

	float kDispPercent = DotProduct3D(displacement, orientedBox.m_kBasis);

	if (iDispPercent < orientedBox.m_halfDimensionsIJK.x && iDispPercent > -orientedBox.m_halfDimensionsIJK.x
		&& jDispPercent < orientedBox.m_halfDimensionsIJK.y && jDispPercent > -orientedBox.m_halfDimensionsIJK.y
		&& kDispPercent < orientedBox.m_halfDimensionsIJK.z && kDispPercent > -orientedBox.m_halfDimensionsIJK.z)
	{
		return referencePos; // point is inside
	}

	float nearestPointI = GetClamped(iDispPercent, -orientedBox.m_halfDimensionsIJK.x, orientedBox.m_halfDimensionsIJK.x);
	float nearestPointJ = GetClamped(jDispPercent, -orientedBox.m_halfDimensionsIJK.y, orientedBox.m_halfDimensionsIJK.y);
	float nearestPointZ = GetClamped(kDispPercent, -orientedBox.m_halfDimensionsIJK.z, orientedBox.m_halfDimensionsIJK.z);

	Vec3 nearestPoint = orientedBox.m_center + (nearestPointI * orientedBox.m_iBasis) + (nearestPointJ * orientedBox.m_jBasis) + (nearestPointZ * orientedBox.m_kBasis);
	return nearestPoint;
}

Vec3 GetNearestPointOnPlane3D(Vec3 const& referencePos, Plane3D const& plane)
{
	return plane.GetNearestPoint(referencePos);
}

bool PushDiscOutOfFixedPoint2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedPoint)
{
    Vec2 displacement = mobileDiscCenter - fixedPoint;
    if (displacement.GetLengthSquared() >= mobileDiscRadius * mobileDiscRadius)
        return false;

    //only if disc needs to be pushed
    displacement.SetLength(mobileDiscRadius);
    mobileDiscCenter = fixedPoint + displacement;
    return true;
}

bool PushPointOutOfFixedDisc2D(Vec2& point, Vec2 const& fixedDiscCenter, float fixedDiscRadius)
{
    Vec2 displacement = point - fixedDiscCenter;
    if (displacement.GetLengthSquared() >= fixedDiscRadius * fixedDiscRadius)
        return false;

    displacement.SetLength(fixedDiscRadius);
    point = fixedDiscCenter + displacement;
    return true;
}

bool PushDiscOutOfFixedDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius)
{
	Vec2 displacement = mobileDiscCenter - fixedDiscCenter;
	if (displacement.GetLengthSquared() >= (fixedDiscRadius + mobileDiscRadius) * (fixedDiscRadius + mobileDiscRadius))
		return false;

    //only if disc needs to be pushed
	displacement.SetLength(fixedDiscRadius + mobileDiscRadius);
    mobileDiscCenter = fixedDiscCenter + displacement;
    return true;
}

bool PushDiscsOutOfEachOther2D(Vec2& discACenter, float discARadius, Vec2& discBCenter, float discBRadius)
{
	Vec2 displacement = discBCenter - discACenter;
    if(displacement.GetLengthSquared() >= (discARadius + discBRadius) * (discARadius + discBRadius))
        return false;

	float overlapDepth = (discARadius + discBRadius) - GetDistance2D(discACenter,discBCenter);
    Vec2 pushCorrection = displacement;
    pushCorrection.SetLength(overlapDepth * 0.5f);

    discBCenter += pushCorrection;
    discACenter += pushCorrection * -1.f;
    return true;
}

bool PushDiscOutOfFixedAABB2D(Vec2& mobileDiscCenter, float discRadius, AABB2 const& fixedBox)
{
    Vec2 nearestPoint = fixedBox.GetNearestPoint(mobileDiscCenter);
    Vec2 nearestPointToDiscCenter = mobileDiscCenter - nearestPoint;
    if(nearestPointToDiscCenter.GetLengthSquared() >= discRadius * discRadius)
        return false;

    float overlapDepth = discRadius - GetDistance2D(mobileDiscCenter,nearestPoint);
    Vec2 pushCorrection = nearestPointToDiscCenter;
    pushCorrection.SetLength(overlapDepth);

    mobileDiscCenter += pushCorrection;
    return true;
}

bool BounceDiscOutOfFixedPoint2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2& mobileDiscVelocity, Vec2 const& fixedPoint, float combinedElasticity)
{
	Vec2 displacement = mobileDiscCenter - fixedPoint;
	if (displacement.GetLengthSquared() >= (mobileDiscRadius * mobileDiscRadius))
		return false;

	displacement.SetLength(mobileDiscRadius);
	mobileDiscCenter = fixedPoint + displacement;

	Vec2 normal = displacement.GetNormalized();

	float projectedLength = DotProduct2D(mobileDiscVelocity, normal);
	Vec2 vectorAlongNormal = normal * projectedLength;
	Vec2 vectorAlongPerpendicular = mobileDiscVelocity - vectorAlongNormal;
	mobileDiscVelocity = vectorAlongPerpendicular - (vectorAlongNormal * combinedElasticity);

	return true;
}

bool BounceDiscsOutOfEachOther2D(Vec2& discACenter, float discARadius, Vec2& discAVelocity, Vec2& discBCenter, float discBRadius, Vec2& discBVelocity, float combinedElasticity)
{
	Vec2 displacementAtoB = discBCenter - discACenter;
	if (displacementAtoB.GetLengthSquared() >= (discARadius + discBRadius) * (discARadius + discBRadius))
		return false;

	float overlapDepth = (discARadius + discBRadius) - GetDistance2D(discACenter, discBCenter);
	Vec2 pushCorrection = displacementAtoB;
	pushCorrection.SetLength(overlapDepth * 0.5f);

	discBCenter += pushCorrection;
	discACenter += pushCorrection * -1.f;
	
	Vec2 collisionNormal = displacementAtoB.GetNormalized();
	float aSpeedAlongNormal = DotProduct2D(discAVelocity, collisionNormal);
	float bSpeedAlongNormal = DotProduct2D(discBVelocity, collisionNormal);

	if (bSpeedAlongNormal < aSpeedAlongNormal)
	{
		Vec2 discANormalVelocity = aSpeedAlongNormal * collisionNormal;
		Vec2 discATangentVelocity = discAVelocity - discANormalVelocity;

		Vec2 discBNormalVelocity = bSpeedAlongNormal * collisionNormal;
		Vec2 discBTangentVelocity = discBVelocity - discBNormalVelocity;

		discAVelocity = discATangentVelocity + (discBNormalVelocity * combinedElasticity);
		discBVelocity = discBTangentVelocity + (discANormalVelocity * combinedElasticity);
		return true;
	}

	return false;
}

bool BounceDiscOutOfFixedDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2& mobileDiscVelocity, Vec2 const& fixedDiscCenter, float fixedDiscRadius, float combinedElasticity)
{
	Vec2 displacement = mobileDiscCenter - fixedDiscCenter;
	if (displacement.GetLengthSquared() >= (mobileDiscRadius + fixedDiscRadius) * (mobileDiscRadius + fixedDiscRadius))
		return false;

	displacement.SetLength(mobileDiscRadius + fixedDiscRadius);
	mobileDiscCenter = fixedDiscCenter + displacement;

	Vec2 normal = displacement.GetNormalized();
	float projectedLength = DotProduct2D(mobileDiscVelocity, normal);
	Vec2 vectorAlongNormal = normal * projectedLength;
	Vec2 vectorAlongPerpendicular = mobileDiscVelocity - vectorAlongNormal;
	mobileDiscVelocity = vectorAlongPerpendicular - (vectorAlongNormal * combinedElasticity);

	return true;
}

bool BounceDiscOutOfFixedOBB2D(Vec2& discCenter, float discRadius, Vec2& discVelocity, OBB2 const& orientedBox, float combinedElasticity)
{
	Vec2 nearestPoint = GetNearestPointOnOBB2D(discCenter, orientedBox);
	return BounceDiscOutOfFixedPoint2D(discCenter, discRadius, discVelocity, nearestPoint, combinedElasticity);
}

bool BounceDiscOutOfFixedCapsule2D(Vec2& discCenter, float discRadius, Vec2& discVelocity, Capsule2 const& capsule, float combinedElasticity)
{
	Vec2 nearestPointOnLine = GetNearestPointOnLineSegment(discCenter, capsule.GetBone());
	return BounceDiscOutOfFixedDisc2D(discCenter, discRadius, discVelocity, nearestPointOnLine, capsule.m_radius, combinedElasticity);
}


//--------------------------------------------------------------------
//Transform utilities
//
void TransformPosition2D(Vec2& posToTransform, float uniformScale, float rotationDegrees, Vec2 const& translation)
{
    posToTransform *= uniformScale;
    posToTransform.RotateDegrees(rotationDegrees);
    posToTransform += translation;
}

void TransformPosition2D(Vec2& posToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation)
{
    posToTransform = translation + (posToTransform.x * iBasis) + (posToTransform.y * jBasis);
}

void TransformPositionXY3D(Vec3& positionToTransform, float scaleXY, float zRotationDegrees, Vec2 const& translationXY)
{
    Vec2 vecXY(positionToTransform.x, positionToTransform.y);
    TransformPosition2D(vecXY, scaleXY, zRotationDegrees, translationXY);
    positionToTransform = Vec3(vecXY.x, vecXY.y, positionToTransform.z);
}

void TransformPositionXY3D(Vec3& positionToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translationXY)
{
    Vec2 posToTransformXY = translationXY + (positionToTransform.x * iBasis) + (positionToTransform.y * jBasis);
    positionToTransform = Vec3(posToTransformXY.x, posToTransformXY.y, positionToTransform.z);
}

void TransformPosition3D(Vec3& positionToTransform, Vec3 const& iBasis, Vec3 const& jBasis, Vec3 const& kBasis, Vec3 const& translationIJK)
{
	Mat44 transform = Mat44(iBasis, jBasis, kBasis, Vec3::ZERO);
	positionToTransform = transform.TransformPosition3D(translationIJK);
}

void TransformPosition3D(Vec3& positionToTransform, Mat44 const& transform)
{
	positionToTransform = transform.TransformPosition3D(positionToTransform);
}

Mat44 GetBillboardTransform(BillboardType billboardType, Mat44 const& targetTransform, Vec3 const& billboardPosition, Vec2 const& billboardScale)
{
	Mat44 transform;
	Vec3 iBasis = Vec3::FORWARD;
	Vec3 jBasis = Vec3::LEFT;
	Vec3 kBasis = Vec3::UP;
	Vec3 targetPos = targetTransform.GetTranslation3D();

	switch (billboardType)
	{
	case BillboardType::NONE:
		break;
	case BillboardType::WORLD_UP_FACING:
		iBasis = targetPos - billboardPosition;
		iBasis.z = 0.f;
		iBasis.Normalize();
		kBasis = Vec3::UP;
		jBasis = CrossProduct3D(kBasis, iBasis);
		transform = Mat44(iBasis, jBasis, kBasis, billboardPosition);
		break;

	case BillboardType::WORLD_UP_OPPOSING:
		iBasis = -targetTransform.GetIBasis3D();
		iBasis.z = 0.f;
		iBasis.Normalize();
		kBasis = Vec3::UP;
		jBasis = CrossProduct3D(kBasis, iBasis);
		transform = Mat44(iBasis, jBasis, kBasis, billboardPosition);
		break;

	case BillboardType::FULL_FACING:
		transform = GetLookAtTransform(billboardPosition, targetPos);
		break;

	case BillboardType::FULL_OPPOSING:
		iBasis = -targetTransform.GetIBasis3D();
		jBasis = -targetTransform.GetJBasis3D();
		kBasis = targetTransform.GetKBasis3D();
		transform = Mat44(iBasis, jBasis, kBasis, billboardPosition);
		break;
	default:
		break;
	}

	transform.AppendScaleNonUniform2D(billboardScale);
	return transform;
}

Mat44 GetLookAtTransform(Vec3 const& pos, Vec3 const& targetPos)
{
	Vec3 iBasis = Vec3::FORWARD;
	Vec3 jBasis = Vec3::LEFT;
	Vec3 kBasis = Vec3::UP;

	iBasis = (targetPos - pos).GetNormalized();
	if (fabsf(DotProduct3D(iBasis, Vec3::UP)) < 0.999999f)
	{
		jBasis = CrossProduct3D(Vec3::UP, iBasis);
		jBasis.Normalize();
		kBasis = CrossProduct3D(iBasis, jBasis);
	}

	else
	{
		kBasis = CrossProduct3D(iBasis, Vec3::LEFT);
		kBasis.Normalize();
		jBasis = CrossProduct3D(kBasis, iBasis);
	}

	return Mat44(iBasis, jBasis, kBasis, pos);
}


RaycastResult2D RaycastVsDisc2D(Vec2 const& startPos, Vec2 const& fwrdNormal, float maxDist, Vec2 const& discCenter, float discRadius)
{
	RaycastResult2D raycastResult;

	//Start is inside disc
	if (IsPointInsideDisc2D(startPos, discCenter, discRadius))
	{
		//Raycast hit
		raycastResult.m_didImpact = true;
		raycastResult.m_impactDistance = 0.f;
		raycastResult.m_impactPos = startPos;
		raycastResult.m_impactNormal = -fwrdNormal;
		return raycastResult;
	}

	Vec2 dispToDiscCenter = discCenter - startPos;
	Vec2 jBasis = fwrdNormal.GetRotated90Degrees();
	float percDispOnJBasis = DotProduct2D(dispToDiscCenter, jBasis);

	//Too far left or right of ray
	if (percDispOnJBasis >= discRadius || percDispOnJBasis <= -discRadius)
	{
		raycastResult.m_didImpact = false;
		return raycastResult;
	}

	//Too far away from start or behind start
	float percDispOnFwrdNormal = DotProduct2D(dispToDiscCenter, fwrdNormal);
	if (percDispOnFwrdNormal >= maxDist + discRadius || percDispOnFwrdNormal <= -discRadius)
	{
		raycastResult.m_didImpact = false;
		return raycastResult;
	}

	//Final edge cases
	float adjustAmount = sqrtf((discRadius * discRadius) - (percDispOnJBasis * percDispOnJBasis));
	float impactDist = percDispOnFwrdNormal - adjustAmount;
	if (impactDist >= maxDist)
	{
		raycastResult.m_didImpact = false;
		return raycastResult;
	}

	if (impactDist <= 0.f)
	{
		raycastResult.m_didImpact = false;
		return raycastResult;
	}

	//Raycast hit
	raycastResult.m_didImpact = true;
	raycastResult.m_impactDistance = impactDist;
	raycastResult.m_impactPos = startPos + (fwrdNormal * impactDist);
	raycastResult.m_impactNormal = (raycastResult.m_impactPos - discCenter).GetNormalized();

	return raycastResult;
}

RaycastResult2D RaycastVsDisc2D(Vec2 const& startPos, Vec2 const& fwrdNormal, float maxDist, Disc2 const& disc)
{
    Vec2 discCenter = disc.m_center;
    float discRadius = disc.m_radius;

	RaycastResult2D raycastResult;

    //Start is inside disc
    if (IsPointInsideDisc2D(startPos, disc)) 
    {
        //Raycast hit
		raycastResult.m_didImpact = true;
		raycastResult.m_impactDistance = 0.f;
		raycastResult.m_impactPos = startPos;
		raycastResult.m_impactNormal = -fwrdNormal;
        return raycastResult;
    }

	Vec2 dispToDiscCenter = discCenter - startPos;
	Vec2 jBasis = fwrdNormal.GetRotated90Degrees();
	float percDispOnJBasis = DotProduct2D(dispToDiscCenter, jBasis);

    //Too far left or right of ray
	if (percDispOnJBasis >= discRadius || percDispOnJBasis <= -discRadius)
	{
		raycastResult.m_didImpact = false;
		return raycastResult;
	}

    //Too far away from start or behind start
	float percDispOnFwrdNormal = DotProduct2D(dispToDiscCenter, fwrdNormal);
	if (percDispOnFwrdNormal >= maxDist + discRadius || percDispOnFwrdNormal <= -discRadius)
	{
		raycastResult.m_didImpact = false;
		return raycastResult;
	}

    //Final edge cases
	float adjustAmount = sqrtf((discRadius * discRadius) - (percDispOnJBasis * percDispOnJBasis));
	float impactDist = percDispOnFwrdNormal - adjustAmount;
	if (impactDist >= maxDist)
	{
		raycastResult.m_didImpact = false;
		return raycastResult;
	}

    if (impactDist <= 0.f)
    {
        raycastResult.m_didImpact = false;
        return raycastResult;
    }

    //Raycast hit
	raycastResult.m_didImpact = true;
	raycastResult.m_impactDistance = impactDist;
	raycastResult.m_impactPos = startPos + (fwrdNormal * impactDist);
	raycastResult.m_impactNormal = (raycastResult.m_impactPos - discCenter).GetNormalized();

	return raycastResult;
}

RaycastResult2D RaycastVsTileHeatMap(Vec2 const& startPos, Vec2 const& fwrdNormal, float maxDist, TileHeatMap const& solidMap, float tileSolidValue)
{
	//#TODO: This only currently works with tiles 1x1
	IntVec2 tileDimensions = IntVec2::ONE;

	RaycastResult2D result;
	result.m_impactDistance = maxDist;
	result.m_impactPos = startPos + (fwrdNormal * maxDist);
	result.m_impactNormal = -fwrdNormal;

	result.m_didImpact = false;
	IntVec2 mapDimensions = solidMap.m_dimensions;
	IntVec2 tileCoords((int)(floorf(startPos.x)), (int)(floorf(startPos.y)));
	int tileIndex = (tileCoords.y * mapDimensions.x) + tileCoords.x;

	//Check if starting pos is inside solid tile
	if (solidMap.m_values[tileIndex] == tileSolidValue)
	{
		result.m_didImpact = true;
		result.m_impactDistance = 0.f;
		result.m_impactNormal = -fwrdNormal;
		result.m_impactPos = startPos;
		return result;
	}

	float fwrdDistPerXCrossing = 1.f / abs(fwrdNormal.x);
	int tileStepDirectionX = 1;
	if (fwrdNormal.x < 0.f)
	{
		tileStepDirectionX = -1;
	}

	//X initialization
	float xAtFirstXCrossing = (float)(tileCoords.x) + (((float)(tileStepDirectionX) + 1.f) * 0.5f);
	float xDistToFirstXCrossing = xAtFirstXCrossing - startPos.x;
	float fwrdDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwrdDistPerXCrossing;

	//Y initialization
	float fwrdDistPerYCrossing = 1.f / abs(fwrdNormal.y);
	int tileStepDirectionY = 1;
	if (fwrdNormal.y < 0.f)
	{
		tileStepDirectionY = -1;
	}

	float yAtFirstYCrossing = (float)(tileCoords.y) + (((float)(tileStepDirectionY) + 1.f) * 0.5f);
	float yDistToFirstCrossing = yAtFirstYCrossing - startPos.y;
	float fwrdDistAtNextYCrossing = fabsf(yDistToFirstCrossing) * fwrdDistPerYCrossing;

	while (true)
	{
		//If next x is closer than next y
		if (fwrdDistAtNextXCrossing <= fwrdDistAtNextYCrossing)
		{
			if (fwrdDistAtNextXCrossing > maxDist)
			{
				//ray went past max distance without hitting
				result.m_didImpact = false;
				result.m_impactDistance = maxDist;
				result.m_impactPos = startPos + (fwrdNormal * maxDist);
				return result;
			}

			tileCoords.x += tileStepDirectionX;
			tileIndex = (tileCoords.y * mapDimensions.x) + tileCoords.x;

			if (solidMap.m_values[tileIndex] == tileSolidValue)
			{
				//RaycastHit
				result.m_didImpact = true;
				result.m_impactDistance = fwrdDistAtNextXCrossing;

				Vec2 hitPos = startPos + (fwrdNormal * fwrdDistAtNextXCrossing);
				result.m_impactPos = hitPos;

				AABB2 tileBounds(Vec2(tileCoords), Vec2(tileCoords + tileDimensions));
				Vec2 nearestPoint = tileBounds.GetNearestPoint(hitPos);

				if (fwrdNormal.x > 0.f)
					result.m_impactNormal = Vec2(-1.f, 0.f);
				else if (fwrdNormal.x < 0.f)
					result.m_impactNormal = Vec2(1.f, 0.f);
				else
					ERROR_AND_DIE("Invalid impact normal");

				return result;
			}

			fwrdDistAtNextXCrossing += fwrdDistPerXCrossing;
		}

		else
		{
			if (fwrdDistAtNextYCrossing > maxDist)
			{
				result.m_didImpact = false;
				result.m_impactDistance = maxDist;
				result.m_impactPos = startPos + (fwrdNormal * maxDist);
				return result;
			}

			tileCoords.y += tileStepDirectionY;
			tileIndex = (tileCoords.y * mapDimensions.x) + tileCoords.x;

			if (solidMap.m_values[tileIndex] == tileSolidValue)
			{
				//RaycastHit
				result.m_didImpact = true;
				result.m_impactDistance = fwrdDistAtNextYCrossing;

				Vec2 hitPos = startPos + (fwrdNormal * fwrdDistAtNextYCrossing);
				result.m_impactPos = hitPos;

				AABB2 tileBounds(Vec2(tileCoords), Vec2(tileCoords + tileDimensions));
				Vec2 nearestPoint = tileBounds.GetNearestPoint(hitPos);

				if (fwrdNormal.y > 0.f)
					result.m_impactNormal = Vec2(0.f, -1.f);
				else if (fwrdNormal.y < 0.f)
					result.m_impactNormal = Vec2(0.f, 1.f);
				else
					ERROR_AND_DIE("Invalid impact normal");

				return result;
			}

			fwrdDistAtNextYCrossing += fwrdDistPerYCrossing;
		}
	}


}

RaycastResult2D RaycastVsTileHeatMap(Ray2 const& ray, TileHeatMap const& solidMap, float tileSolidValue)
{
	//#TODO: This only currently works with tiles 1x1
	IntVec2 tileDimensions = IntVec2::ONE;

	float maxDist = ray.m_maxLength;
	Vec2 fwrdNormal = ray.m_fwrdNormal;
	Vec2 startPos = ray.m_startPos;

	RaycastResult2D result;
	result.m_impactDistance = ray.m_maxLength;
	result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);
	result.m_impactNormal = -ray.m_fwrdNormal;
	result.m_didImpact = false;

	IntVec2 mapDimensions = solidMap.m_dimensions;
	IntVec2 tileCoords((int)(floorf(startPos.x)), (int)(floorf(startPos.y)));
	int tileIndex = (tileCoords.y * mapDimensions.x) + tileCoords.x;

	//Check if starting pos is inside solid tile
	if (solidMap.m_values[tileIndex] == tileSolidValue)
	{
		result.m_didImpact = true;
		result.m_impactDistance = 0.f;
		result.m_impactNormal = -fwrdNormal;
		result.m_impactPos = startPos;
		return result;
	}

	float fwrdDistPerXCrossing = 1.f / abs(fwrdNormal.x);
	int tileStepDirectionX = 1;
	if (fwrdNormal.x < 0.f)
	{
		tileStepDirectionX = -1;
	}

	//X initialization
	float xAtFirstXCrossing = (float)(tileCoords.x) + (((float)(tileStepDirectionX) + 1.f) * 0.5f);
	float xDistToFirstXCrossing = xAtFirstXCrossing - startPos.x;
	float fwrdDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwrdDistPerXCrossing;

	//Y initialization
	float fwrdDistPerYCrossing = 1.f / abs(fwrdNormal.y);
	int tileStepDirectionY = 1;
	if (fwrdNormal.y < 0.f)
	{
		tileStepDirectionY = -1;
	}

	float yAtFirstYCrossing = (float)(tileCoords.y) + (((float)(tileStepDirectionY) + 1.f) * 0.5f);
	float yDistToFirstCrossing = yAtFirstYCrossing - startPos.y;
	float fwrdDistAtNextYCrossing = fabsf(yDistToFirstCrossing) * fwrdDistPerYCrossing;

	while (true)
	{
		//If next x is closer than next y
		if (fwrdDistAtNextXCrossing <= fwrdDistAtNextYCrossing)
		{
			if (fwrdDistAtNextXCrossing > maxDist)
			{
				//ray went past max distance without hitting
				result.m_didImpact = false;
				result.m_impactDistance = maxDist;
				result.m_impactPos = startPos + (fwrdNormal * maxDist);
				return result;
			}

			tileCoords.x += tileStepDirectionX;
			tileIndex = (tileCoords.y * mapDimensions.x) + tileCoords.x;

			if (solidMap.m_values[tileIndex] == tileSolidValue)
			{
				//RaycastHit
				result.m_didImpact = true;
				result.m_impactDistance = fwrdDistAtNextXCrossing;

				Vec2 hitPos = startPos + (fwrdNormal * fwrdDistAtNextXCrossing);
				result.m_impactPos = hitPos;

				AABB2 tileBounds(Vec2(tileCoords), Vec2(tileCoords + tileDimensions));
				Vec2 nearestPoint = tileBounds.GetNearestPoint(hitPos);

				if (fwrdNormal.x > 0.f)
					result.m_impactNormal = Vec2(-1.f, 0.f);
				else if (fwrdNormal.x < 0.f)
					result.m_impactNormal = Vec2(1.f, 0.f);
				else
					ERROR_AND_DIE("Invalid impact normal");

				return result;
			}

			fwrdDistAtNextXCrossing += fwrdDistPerXCrossing;
		}

		else
		{
			if (fwrdDistAtNextYCrossing > maxDist)
			{
				result.m_didImpact = false;
				result.m_impactDistance = maxDist;
				result.m_impactPos = startPos + (fwrdNormal * maxDist);
				return result;
			}

			tileCoords.y += tileStepDirectionY;
			tileIndex = (tileCoords.y * mapDimensions.x) + tileCoords.x;

			if (solidMap.m_values[tileIndex] == tileSolidValue)
			{
				//RaycastHit
				result.m_didImpact = true;
				result.m_impactDistance = fwrdDistAtNextYCrossing;

				Vec2 hitPos = startPos + (fwrdNormal * fwrdDistAtNextYCrossing);
				result.m_impactPos = hitPos;

				AABB2 tileBounds(Vec2(tileCoords), Vec2(tileCoords + tileDimensions));
				Vec2 nearestPoint = tileBounds.GetNearestPoint(hitPos);

				if (fwrdNormal.y > 0.f)
					result.m_impactNormal = Vec2(0.f, -1.f);
				else if (fwrdNormal.y < 0.f)
					result.m_impactNormal = Vec2(0.f, 1.f);
				else
					ERROR_AND_DIE("Invalid impact normal");

				return result;
			}

			fwrdDistAtNextYCrossing += fwrdDistPerYCrossing;
		}
	}
}

RaycastResult2D RaycastVsLineSegment2D(Ray2 const& ray, LineSegment2 const& lineSegment)
{
	Vec2 jBasis = ray.m_fwrdNormal.GetRotated90Degrees();
	Vec2 rayToStart = lineSegment.m_start - ray.m_startPos;
	Vec2 rayToEnd = lineSegment.m_end - ray.m_startPos;
	float jLengthRS = GetProjectedLength2D(rayToStart, jBasis);
	float jLengthRE = GetProjectedLength2D(rayToEnd, jBasis);

	RaycastResult2D result;

	if (jLengthRS * jLengthRE >= 0.f)
	{
		//failed strattle test
		result.m_didImpact = false;
		result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);
		result.m_impactNormal = ray.m_fwrdNormal;
		result.m_impactDistance = ray.m_maxLength;
		return result;
	}

	float iLengthRS = GetProjectedLength2D(rayToStart, ray.m_fwrdNormal);
	float iLengthRE = GetProjectedLength2D(rayToEnd, ray.m_fwrdNormal);
	if (iLengthRS >= ray.m_maxLength && iLengthRE >= ray.m_maxLength)
	{
		//both points farther than max length
		result.m_didImpact = false;
		result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);
		result.m_impactNormal = ray.m_fwrdNormal;
		result.m_impactDistance = ray.m_maxLength;
		return result;
	}

	if (iLengthRS <= 0.f && iLengthRE <= 0.f)
	{
		//line is behind ray
		result.m_didImpact = false;
		result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);
		result.m_impactNormal = ray.m_fwrdNormal;
		result.m_impactDistance = ray.m_maxLength;
		return result;
	}

	float t = jLengthRS / (jLengthRE - jLengthRS);
	float impactDist = iLengthRS + (t * (iLengthRS - iLengthRE));

	if (impactDist <= 0.f || impactDist > ray.m_maxLength)
	{
		result.m_didImpact = false;
		result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);
		result.m_impactNormal = ray.m_fwrdNormal;
		result.m_impactDistance = ray.m_maxLength;
		return result;
	}

	result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * impactDist);
	result.m_impactNormal = (lineSegment.m_start - lineSegment.m_end).GetNormalized().GetRotated90Degrees();
	if (DotProduct2D(ray.m_fwrdNormal, result.m_impactNormal) > 0.f)
	{
		result.m_impactNormal = -result.m_impactNormal;
	}

	result.m_didImpact = true;
	result.m_impactDistance = (result.m_impactPos - ray.m_startPos).GetLength();
	return result;
}

RaycastResult2D RaycastVsAABB2D(Ray2 const& ray, AABB2 const& alignedBox)
{
	Vec2 rayEnd = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);
	AABB2 rayBounds(ray.m_startPos, ray.m_startPos);
	rayBounds.StretchToIncludePoint(rayEnd);

	RaycastResult2D result;
	result.m_didImpact = false;
	result.m_impactNormal = ray.m_fwrdNormal;
	result.m_impactPos = rayEnd;
	result.m_impactDistance = ray.m_maxLength;

	if (!DoAABB2sOverlap(alignedBox, rayBounds)) // ray is nowhere near box
		return result;

	if (alignedBox.IsPointOnOrInside(ray.m_startPos))
	{
		result.m_didImpact = true;
		result.m_impactNormal = -ray.m_fwrdNormal;
		result.m_impactDistance = 0.f;
		result.m_impactPos = ray.m_startPos;
		return result;
	}

	float maxLenFrac = 1.f / ray.m_maxLength;

	//t x range
	float deltaX = alignedBox.m_mins.x - ray.m_startPos.x;
	FloatRange tRangeX(maxLenFrac * (deltaX / ray.m_fwrdNormal.x));
	deltaX = alignedBox.m_maxs.x - ray.m_startPos.x;
	tRangeX.StretchToIncludeValue(maxLenFrac * (deltaX / ray.m_fwrdNormal.x));

	//t y range
	float deltaY = alignedBox.m_mins.y - ray.m_startPos.y;
	FloatRange tRangeY(maxLenFrac * (deltaY / ray.m_fwrdNormal.y));
	deltaY = alignedBox.m_maxs.y - ray.m_startPos.y;
	tRangeY.StretchToIncludeValue(maxLenFrac * (deltaY / ray.m_fwrdNormal.y));

	if (!tRangeX.IsOverlapping(tRangeY))
		return result;

	float tHit = tRangeX.m_min > tRangeY.m_min ? tRangeX.m_min : tRangeY.m_min;
	
	result.m_impactDistance = tHit * ray.m_maxLength;
	result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * result.m_impactDistance);
	result.m_didImpact = true;
	
	if (tHit == tRangeX.m_min)
	{
		//Left Side
		if (ray.m_fwrdNormal.x >= 0.f)
		{
			result.m_impactNormal = Vec2::WEST;
			return result;
		}

		//Right Side
		else
		{
			result.m_impactNormal = Vec2::EAST;
			return result;
		}
	}

	else
	{
		//Bottom
		if (ray.m_fwrdNormal.y >= 0.f)
		{
			result.m_impactNormal = Vec2::SOUTH;
			return result;
		}

		//Top
		result.m_impactNormal = Vec2::NORTH;
		return result;
	}
}

RaycastResult2D RaycastVsOBB2D(Ray2 const& ray, OBB2 const& orientedBox)
{
	Vec2 rayEnd = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);

	RaycastResult2D result;
	result.m_didImpact = false;
	result.m_impactNormal = ray.m_fwrdNormal;
	result.m_impactPos = rayEnd;
	result.m_impactDistance = ray.m_maxLength;

	Vec2 jBasis = orientedBox.m_iBasisNormal.GetRotated90Degrees();

	//Convert ray start and direction to local space to do raycast vs AABB2
	Vec2 displacement = ray.m_startPos - orientedBox.m_center;
	Vec2 rayStartIJ = Vec2(DotProduct2D(displacement, orientedBox.m_iBasisNormal), DotProduct2D(displacement, jBasis));
	Vec2 rayFwrdIJ = Vec2(DotProduct2D(ray.m_fwrdNormal, orientedBox.m_iBasisNormal), DotProduct2D(ray.m_fwrdNormal, jBasis));
	AABB2 box = AABB2(-orientedBox.m_halfDimensionsIJ, orientedBox.m_halfDimensionsIJ);

	Ray2 localRay(rayStartIJ, rayFwrdIJ, ray.m_maxLength);
	RaycastResult2D localResult = RaycastVsAABB2D(localRay, box);

	if (localResult.m_didImpact)
	{
		result.m_impactNormal = orientedBox.m_iBasisNormal * localResult.m_impactNormal.x + jBasis * localResult.m_impactNormal.y;
		result.m_didImpact = true;
		result.m_impactDistance = localResult.m_impactDistance;
		result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * localResult.m_impactDistance);
	}
	
	return result;
}

RaycastResult2D RaycastVsPlane2D(Ray2 const& ray, Plane2D const& plane)
{
	RaycastResult2D result;
	result.m_didImpact = false;
	result.m_impactDistance = ray.m_maxLength;
	result.m_impactNormal = ray.m_fwrdNormal;
	result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);

	Vec2 rayEndPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);
	float startAltitude = plane.GetAltitudeFromPoint(ray.m_startPos);
	float endAltitude = plane.GetAltitudeFromPoint(rayEndPos);

	if (startAltitude * endAltitude >= 0.f)
	{
		return result; //failed straddle check
	}

	float distanceToPlaneAlongRayFwrd = -startAltitude / DotProduct2D(ray.m_fwrdNormal, plane.m_normal);
	if (distanceToPlaneAlongRayFwrd < ray.m_maxLength)
	{
		result.m_didImpact = true;
		result.m_impactDistance = distanceToPlaneAlongRayFwrd;
		result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * distanceToPlaneAlongRayFwrd);
		result.m_impactNormal = plane.m_normal;

		if (startAltitude < 0)
		{
			result.m_impactNormal = -plane.m_normal;
		}
	}

	return result;
}

RaycastResult2D RaycastVsConvexHull2(Ray2 const& ray, ConvexHull2 const& convexHull)
{
	RaycastResult2D result;
	result.m_didImpact = false;
	result.m_impactDistance = ray.m_maxLength;
	result.m_impactNormal = ray.m_fwrdNormal;
	result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);

	std::vector<float> entranceLengths;
	entranceLengths.reserve(1 + convexHull.m_planes.size());
	entranceLengths.push_back(0);

	std::vector<float> exitLengths;
	exitLengths.reserve(1 + convexHull.m_planes.size());
	exitLengths.push_back(ray.m_maxLength);

	std::vector<RaycastResult2D> entranceHits;
	std::vector<RaycastResult2D> exitHits;
	int lastEnterIndex = -1;
	int firstExitIndex = -1;

	for (int i = 0; i < (int)convexHull.m_planes.size(); ++i)
	{
		
		for (int j = 0; j < (int)exitLengths.size(); ++j)
		{
			for (int n = 0; n < (int)entranceHits.size(); ++n)
			{
				if(exitLengths[j] < entranceLengths[n])
					return result;
			}
		}
		
		

		RaycastResult2D planeResult = RaycastVsPlane2D(ray, convexHull.m_planes[i]);
		if (planeResult.m_didImpact)
		{
			if (convexHull.m_planes[i].IsPointInFrontOf(ray.m_startPos))
			{
				entranceHits.push_back(planeResult);
				entranceLengths.push_back(planeResult.m_impactDistance);

				int newEnterIdx = (int)(entranceHits.size() - 1);
				if (lastEnterIndex >= 0)
				{
					if (planeResult.m_impactDistance > entranceHits[lastEnterIndex].m_impactDistance)
					{
						lastEnterIndex = newEnterIdx;
					}
				}

				else if (planeResult.m_impactDistance > 0.f)
				{
					lastEnterIndex = newEnterIdx;
				}
			}

			else
			{
				exitHits.push_back(planeResult);
				exitLengths.push_back(planeResult.m_impactDistance);
				int newExitIdx = (int)(exitHits.size() - 1);

				if (firstExitIndex >= 0)
				{
					if (planeResult.m_impactDistance < exitHits[firstExitIndex].m_impactDistance)
					{
						firstExitIndex = newExitIdx;
					}
				}

				else if (planeResult.m_impactDistance < ray.m_maxLength)
				{
					firstExitIndex = newExitIdx;
				}
			}
		}
	}

	if (lastEnterIndex == -1)
	{
		if (convexHull.IsPointInside(ray.m_startPos))
		{
			result.m_impactNormal = -ray.m_fwrdNormal;
			result.m_impactDistance = 0.f;
			result.m_didImpact = true;
			result.m_impactPos = ray.m_startPos;
		}

		return result;
	}

	else
	{

		Vec2 firstEntrancePos = entranceHits[lastEnterIndex].m_impactPos;
		
		Vec2 lastExitPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);
		if (firstExitIndex >= 0)
		{
			lastExitPos = exitHits[firstExitIndex].m_impactPos;
		}

		Vec2 middlePoint = (firstEntrancePos + lastExitPos) * 0.5f;

		if (convexHull.IsPointInside(middlePoint))
		{
			result = entranceHits[lastEnterIndex];
		}
	}

	return result;
}

RaycastResult3D RaycastVsSphere3D(Ray3 const& ray, Vec3 const& sphereCenter, float sphereRadius)
{
	RaycastResult3D result;
	result.m_didImpact = false;
	result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);
	result.m_impactDistance = ray.m_maxLength;
	result.m_impactNormal = ray.m_fwrdNormal;

	if (IsPointInsideSphere3D(ray.m_startPos, sphereCenter, sphereRadius))
	{
		result.m_didImpact = true;
		result.m_impactDistance = 0.f;
		result.m_impactPos = ray.m_startPos;
		result.m_impactNormal = -ray.m_fwrdNormal;
		return result;
	}

	Vec3 startToCenter = sphereCenter - ray.m_startPos;
	Vec3 end = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);
	Vec3 startToEnd = end - ray.m_startPos;
	Vec3 startToCenterI = GetProjectedOnto3D(startToCenter, startToEnd);

	Vec3 jBasis = startToCenter - startToCenterI;
	jBasis.Normalize();

	float percDispOnJBasis = DotProduct3D(startToCenter, jBasis);

	if (percDispOnJBasis >= sphereRadius || percDispOnJBasis <= -sphereRadius)
		return result; //failed because ray was too far left or right in terms of jBasis

	float percDispOnFwrdNormal = DotProduct3D(startToCenter, ray.m_fwrdNormal);
	if (percDispOnFwrdNormal >= ray.m_maxLength + sphereRadius || percDispOnFwrdNormal <= -sphereRadius)
		return result; //failed because sphere is too far away from start or behind it

	//edge case where ray barely misses
	float adjustAmount = sqrtf((sphereRadius * sphereRadius) - (percDispOnJBasis * percDispOnJBasis));
	float impactDist = percDispOnFwrdNormal - adjustAmount;
	if (impactDist >= ray.m_maxLength)
		return result;

	if (impactDist <= 0.f)
		return result;

	//raycast hit
	result.m_didImpact = true;
	result.m_impactDistance = impactDist;
	result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * impactDist);
	result.m_impactNormal = (result.m_impactPos - sphereCenter).GetNormalized();

	return result;
}

RaycastResult3D RaycastVsAABB3D(Ray3 const& ray, AABB3 const& box)
{
	Vec3 rayEnd = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);
	AABB3 rayBounds(ray.m_startPos, ray.m_startPos);
	rayBounds.StretchToIncludePoint(rayEnd);

	RaycastResult3D result;
	result.m_didImpact = false;
	result.m_impactNormal = ray.m_fwrdNormal;
	result.m_impactPos = rayEnd;
	result.m_impactDistance = ray.m_maxLength;

	if (!DoAABB3sOverlap3D(box, rayBounds))
		return result;
	
	if (box.IsPointOnOrInside(ray.m_startPos))
	{
		result.m_didImpact = true;
		result.m_impactNormal = -ray.m_fwrdNormal;
		result.m_impactDistance = 0.f;
		result.m_impactPos = ray.m_startPos;
		return result;
	}

	float maxLenFrac = 1.f / ray.m_maxLength;

	//t x range
	float deltaX = box.m_mins.x - ray.m_startPos.x;
	FloatRange tRangeX(maxLenFrac * (deltaX / ray.m_fwrdNormal.x));
	deltaX = box.m_maxs.x - ray.m_startPos.x;
	tRangeX.StretchToIncludeValue(maxLenFrac * (deltaX / ray.m_fwrdNormal.x));

	//t y range
	float deltaY = box.m_mins.y - ray.m_startPos.y;
	FloatRange tRangeY(maxLenFrac * (deltaY / ray.m_fwrdNormal.y));
	deltaY = box.m_maxs.y - ray.m_startPos.y;
	tRangeY.StretchToIncludeValue(maxLenFrac * (deltaY / ray.m_fwrdNormal.y));

	//t z range
	float deltaZ = box.m_mins.z - ray.m_startPos.z;
	FloatRange tRangeZ(maxLenFrac * (deltaZ / ray.m_fwrdNormal.z));
	deltaZ = box.m_maxs.z - ray.m_startPos.z;
	tRangeZ.StretchToIncludeValue(maxLenFrac * (deltaZ / ray.m_fwrdNormal.z));

	if (!tRangeX.IsOverlapping(tRangeY) || !tRangeX.IsOverlapping(tRangeZ) || !tRangeY.IsOverlapping(tRangeZ))
		return result;

	float tHit = fmaxf(tRangeX.m_min, tRangeY.m_min);
	tHit = fmaxf(tHit, tRangeZ.m_min);

	result.m_impactDistance = tHit * ray.m_maxLength;
	result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * result.m_impactDistance);
	result.m_didImpact = true;

	if (tHit == tRangeX.m_min)
	{
		if (ray.m_fwrdNormal.x >= 0.f)
		{
			result.m_impactNormal = -Vec3::FORWARD;
			return result;
		}

		else
		{
			result.m_impactNormal = Vec3::FORWARD;
			return result;
		}

	}

	else if (tHit == tRangeY.m_min)
	{
		if (ray.m_fwrdNormal.y >= 0.f)
		{
			result.m_impactNormal = -Vec3::LEFT;
			return result;
		}

		else
		{
			result.m_impactNormal = Vec3::LEFT;
			return result;
		}
	}

	else
	{
		if (ray.m_fwrdNormal.z >= 0.f)
		{
			result.m_impactNormal = -Vec3::UP;
			return result;
		}

		else
		{
			result.m_impactNormal = Vec3::UP;
			return result;
		}
	}
}

RaycastResult3D RaycastVsAABB3D(Vec3 const& startPos, Vec3 const& fwrdNormal, float maxDist, AABB3 const& box)
{
	Vec3 rayEnd = startPos + (fwrdNormal * maxDist);

	RaycastResult3D result;
	result.m_didImpact = false;
	result.m_impactNormal = fwrdNormal;
	result.m_impactPos = rayEnd;
	result.m_impactDistance = maxDist;

	if (box.IsPointOnOrInside(startPos))
	{
		result.m_didImpact = true;
		result.m_impactNormal = -fwrdNormal;
		result.m_impactDistance = 0.f;
		result.m_impactPos = startPos;
		return result;
	}

	AABB3 rayBounds(startPos, startPos);
	rayBounds.StretchToIncludePoint(rayEnd);

	if (!DoAABB3sOverlap3D(box, rayBounds))
		return result;

	float maxLenFrac = 1.f / maxDist;

	//t x range
	float deltaX = box.m_mins.x - startPos.x;
	FloatRange tRangeX(maxLenFrac * (deltaX / fwrdNormal.x));
	deltaX = box.m_maxs.x - startPos.x;
	tRangeX.StretchToIncludeValue(maxLenFrac * (deltaX / fwrdNormal.x));

	//t y range
	float deltaY = box.m_mins.y - startPos.y;
	FloatRange tRangeY(maxLenFrac * (deltaY / fwrdNormal.y));
	deltaY = box.m_maxs.y - startPos.y;
	tRangeY.StretchToIncludeValue(maxLenFrac * (deltaY / fwrdNormal.y));

	//t z range
	float deltaZ = box.m_mins.z - startPos.z;
	FloatRange tRangeZ(maxLenFrac * (deltaZ / fwrdNormal.z));
	deltaZ = box.m_maxs.z - startPos.z;
	tRangeZ.StretchToIncludeValue(maxLenFrac * (deltaZ / fwrdNormal.z));

	if (!tRangeX.IsOverlapping(tRangeY) || !tRangeX.IsOverlapping(tRangeZ) || !tRangeY.IsOverlapping(tRangeZ))
		return result;

	float tHit = fmaxf(tRangeX.m_min, tRangeY.m_min);
	tHit = fmaxf(tHit, tRangeZ.m_min);

	result.m_impactDistance = tHit * maxDist;
	result.m_impactPos = startPos + (fwrdNormal * result.m_impactDistance);
	result.m_didImpact = true;

	if (tHit == tRangeX.m_min)
	{
		if (fwrdNormal.x >= 0.f)
		{
			result.m_impactNormal = -Vec3::FORWARD;
			return result;
		}

		else
		{
			result.m_impactNormal = Vec3::FORWARD;
			return result;
		}

	}

	else if (tHit == tRangeY.m_min)
	{
		if (fwrdNormal.y >= 0.f)
		{
			result.m_impactNormal = -Vec3::LEFT;
			return result;
		}

		else
		{
			result.m_impactNormal = Vec3::LEFT;
			return result;
		}
	}

	else
	{
		if (fwrdNormal.z >= 0.f)
		{
			result.m_impactNormal = -Vec3::UP;
			return result;
		}

		else
		{
			result.m_impactNormal = Vec3::UP;
			return result;
		}
	}
}

RaycastResult3D RaycastVsAABB3D(Vec3 const& startPos, Vec3 const& endPosition, AABB3 const& box)
{
	Vec3 disp = endPosition - startPos;
	float maxDist = disp.GetLength();
	if (maxDist == 0.f || maxDist == 1.f) RaycastResult3D();;
	float scale = 1.f / maxDist;
	Vec3 fwrdNormal = disp * scale;

	RaycastResult3D result;
	result.m_didImpact = false;
	result.m_impactNormal = fwrdNormal;
	result.m_impactPos = endPosition;
	result.m_impactDistance = maxDist;

	if (box.IsPointOnOrInside(startPos))
	{
		result.m_didImpact = true;
		result.m_impactNormal = -fwrdNormal;
		result.m_impactDistance = 0.f;
		result.m_impactPos = startPos;
		return result;
	}

	AABB3 rayBounds(startPos, startPos);
	rayBounds.StretchToIncludePoint(endPosition);

	if (!DoAABB3sOverlap3D(box, rayBounds))
		return result;

	float maxLenFrac = 1.f / maxDist;

	//t x range
	float deltaX = box.m_mins.x - startPos.x;
	FloatRange tRangeX(maxLenFrac * (deltaX / fwrdNormal.x));
	deltaX = box.m_maxs.x - startPos.x;
	tRangeX.StretchToIncludeValue(maxLenFrac * (deltaX / fwrdNormal.x));

	//t y range
	float deltaY = box.m_mins.y - startPos.y;
	FloatRange tRangeY(maxLenFrac * (deltaY / fwrdNormal.y));
	deltaY = box.m_maxs.y - startPos.y;
	tRangeY.StretchToIncludeValue(maxLenFrac * (deltaY / fwrdNormal.y));

	//t z range
	float deltaZ = box.m_mins.z - startPos.z;
	FloatRange tRangeZ(maxLenFrac * (deltaZ / fwrdNormal.z));
	deltaZ = box.m_maxs.z - startPos.z;
	tRangeZ.StretchToIncludeValue(maxLenFrac * (deltaZ / fwrdNormal.z));

	if (!tRangeX.IsOverlapping(tRangeY) || !tRangeX.IsOverlapping(tRangeZ) || !tRangeY.IsOverlapping(tRangeZ))
		return result;

	float tHit = fmaxf(tRangeX.m_min, tRangeY.m_min);
	tHit = fmaxf(tHit, tRangeZ.m_min);

	result.m_impactDistance = tHit * maxDist;
	result.m_impactPos = startPos + (fwrdNormal * result.m_impactDistance);
	result.m_didImpact = true;

	if (tHit == tRangeX.m_min)
	{
		if (fwrdNormal.x >= 0.f)
		{
			result.m_impactNormal = -Vec3::FORWARD;
			return result;
		}

		else
		{
			result.m_impactNormal = Vec3::FORWARD;
			return result;
		}

	}

	else if (tHit == tRangeY.m_min)
	{
		if (fwrdNormal.y >= 0.f)
		{
			result.m_impactNormal = -Vec3::LEFT;
			return result;
		}

		else
		{
			result.m_impactNormal = Vec3::LEFT;
			return result;
		}
	}

	else
	{
		if (fwrdNormal.z >= 0.f)
		{
			result.m_impactNormal = -Vec3::UP;
			return result;
		}

		else
		{
			result.m_impactNormal = Vec3::UP;
			return result;
		}
	}
}

RaycastResult3D RaycastVsOBB3D(Ray3 const& ray, OBB3 const& orientedBox)
{
	Vec3 rayEnd = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);

	RaycastResult3D result;
	result.m_didImpact = false;
	result.m_impactNormal = ray.m_fwrdNormal;
	result.m_impactPos = rayEnd;
	result.m_impactDistance = ray.m_maxLength;

	Vec3 displacement = ray.m_startPos - orientedBox.m_center;
	Vec3 rayStartIJK;
	rayStartIJK.x = DotProduct3D(displacement, orientedBox.m_iBasis);
	rayStartIJK.y = DotProduct3D(displacement, orientedBox.m_jBasis);
	rayStartIJK.z = DotProduct3D(displacement, orientedBox.m_kBasis);
	Vec3 rayFwrdIJK = Vec3(DotProduct3D(ray.m_fwrdNormal, orientedBox.m_iBasis), DotProduct3D(ray.m_fwrdNormal, orientedBox.m_jBasis), DotProduct3D(ray.m_fwrdNormal, orientedBox.m_kBasis));

	AABB3 box = AABB3(-orientedBox.m_halfDimensionsIJK, orientedBox.m_halfDimensionsIJK);
	Ray3 localRay(rayStartIJK, rayFwrdIJK, ray.m_maxLength);
	RaycastResult3D localResult = RaycastVsAABB3D(localRay, box);

	if (localResult.m_didImpact)
	{
		result.m_impactNormal = (orientedBox.m_iBasis * localResult.m_impactNormal.x) + (orientedBox.m_jBasis * localResult.m_impactNormal.y) + (orientedBox.m_kBasis * localResult.m_impactNormal.z);
		result.m_didImpact = true;
		result.m_impactDistance = localResult.m_impactDistance;
		result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * localResult.m_impactDistance);
	}

	return result;
}

RaycastResult3D RaycastVsZCylinder3D(Ray3 const& ray, ZCylinder3D const& cylinder)
{
	RaycastResult3D result;
	result.m_didImpact = false;
	result.m_impactNormal = ray.m_fwrdNormal;
	result.m_impactDistance = ray.m_maxLength;
	result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);

	if (IsPointInsideZCylinder3D(ray.m_startPos, cylinder))
	{
		result.m_didImpact = true;
		result.m_impactDistance = 0.f;
		result.m_impactPos = ray.m_startPos;
		result.m_impactNormal = -ray.m_fwrdNormal;
		return result;
	}

	//Find Z t range
	float maxLenFrac = 1.f / ray.m_maxLength;
	float deltaZ = cylinder.m_zRange.m_min - ray.m_startPos.z;
	FloatRange tRangeZ(maxLenFrac * (deltaZ / ray.m_fwrdNormal.z));
	deltaZ = cylinder.m_zRange.m_max - ray.m_startPos.z;
	tRangeZ.StretchToIncludeValue(maxLenFrac * (deltaZ / ray.m_fwrdNormal.z));
	FloatRange zeroToOne(0.f, 1.f);
	if (!tRangeZ.IsOverlapping(zeroToOne))
		return result;

	Vec2 xyStartPos = Vec2(ray.m_startPos.x, ray.m_startPos.y);
	Vec2 xyIBasis = Vec2(ray.m_fwrdNormal.x, ray.m_fwrdNormal.y).GetNormalized();
	Vec2 xyJBasis = xyIBasis.GetRotated90Degrees();
	Vec2 displacement = cylinder.m_centerXY - xyStartPos;

	float dispJPercent = DotProduct2D(displacement, xyJBasis);
	if (dispJPercent >= cylinder.m_radius || dispJPercent <= -cylinder.m_radius)
		return result;

	Vec3 endPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);
	Vec2 xyEndPos = Vec2(endPos.x, endPos.y);
	float xyMaxLength = (Vec2(endPos.x, endPos.y) - xyStartPos).GetLength();
	float dispIPercent = DotProduct2D(displacement, xyIBasis);
	if (dispIPercent >= xyMaxLength + cylinder.m_radius || dispIPercent <= -cylinder.m_radius)
		return result;

	float adjustAmount = sqrtf((cylinder.m_radius * cylinder.m_radius) - (dispJPercent * dispJPercent));
	float entryDist = dispIPercent - adjustAmount;
	float exitDist = dispIPercent + adjustAmount;
	float xyMaxLengthFrac = 1.f / xyMaxLength;
	FloatRange tRangeEntryExit(entryDist * xyMaxLengthFrac);
	tRangeEntryExit.StretchToIncludeValue(exitDist * xyMaxLengthFrac);

	if (!tRangeEntryExit.IsOverlapping(zeroToOne) || !tRangeEntryExit.IsOverlapping(tRangeZ))
		return result;

	float tHit = tRangeZ.m_min > tRangeEntryExit.m_min ? tRangeZ.m_min : tRangeEntryExit.m_min;

	result.m_impactDistance = tHit * ray.m_maxLength;
	result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * result.m_impactDistance);
	result.m_didImpact = true;

	if (tHit == tRangeZ.m_min)
	{
		if (ray.m_fwrdNormal.z >= 0.f)
		{
			result.m_impactNormal = Vec3::DOWN;
			return result;
		}

		else
		{
			result.m_impactNormal = Vec3::UP;
			return result;
		}
	}

	Vec3 cylinderCenterXYZ = Vec3(cylinder.m_centerXY.x, cylinder.m_centerXY.y, result.m_impactPos.z);
	result.m_impactNormal = (result.m_impactPos - cylinderCenterXYZ).GetNormalized();
	return result;
}

RaycastResult3D RaycastVsPlane3D(Ray3 const& ray, Plane3D const& plane)
{
	RaycastResult3D result;
	result.m_didImpact = false;
	result.m_impactDistance = ray.m_maxLength;
	result.m_impactNormal = ray.m_fwrdNormal;
	result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);

	Vec3 rayEndPos = ray.m_startPos + (ray.m_fwrdNormal * ray.m_maxLength);

	float startAltitude = plane.GetAltitudeFromPoint(ray.m_startPos);
	float endAltitude = plane.GetAltitudeFromPoint(rayEndPos);

	if (startAltitude * endAltitude >= 0.f)
	{
		return result; //Miss failed straddle check
	}

	float distanceToPlaneAlongRayFwrd = -startAltitude / DotProduct3D(ray.m_fwrdNormal, plane.m_normal);

	if (distanceToPlaneAlongRayFwrd < ray.m_maxLength)
	{
		result.m_didImpact = true;
		result.m_impactDistance = distanceToPlaneAlongRayFwrd;
		result.m_impactPos = ray.m_startPos + (ray.m_fwrdNormal * distanceToPlaneAlongRayFwrd);
		result.m_impactNormal = plane.m_normal;

		if (startAltitude < 0)
		{
			result.m_impactNormal = -plane.m_normal;
		}
	}

	return result;
}

Ray3 GetRayFromMousePosition(Vec2 const& mouseUV, Camera const* camera, float maxLength)
{
	Vec2 screenPos = RangeMap(mouseUV, Vec2::ZERO, Vec2::ONE, Vec2(-1.f, -1.f), Vec2(1.f, 1.f));
	float nearPlaneHeight = 2.f * camera->GetPerspectiveNearDistance() * tanf(camera->GetFOV() / 2.f);
	float nearPlaneWidth = nearPlaneHeight * camera->GetAspect();
	float yDir = screenPos.x * (nearPlaneWidth / 2.f);
	float zDir = -(screenPos.y * (nearPlaneHeight / 2.f));
	Vec3 iForward = Vec3(1.f, yDir, zDir).GetNormalized();
	Mat44 cameraTransform = camera->GetOrientation().GetAsMatrix_IFwd_JLeft_KUp();
	Ray3 ray;
	ray.m_fwrdNormal = cameraTransform.TransformVectorQuantity3D(iForward).GetNormalized();
	ray.m_startPos = camera->GetPosition();
	float farDistance = camera->GetPerspectiveFarDistance();
	ray.m_maxLength = maxLength == -1.f ? farDistance : maxLength;
	return ray;
}

float ComputeCubicBezier1D(float A, float B, float C, float D, float t)
{
	float aB = Lerp(A, B, t);
	float bC = Lerp(B, C, t);
	float cD = Lerp(C, D, t);
	float aBC = Lerp(aB, bC, t);
	float bCD = Lerp(bC, cD, t);
	return Lerp(aBC, bCD, t);
}

float ComputeQuinticBezier1D(float A, float B, float C, float D, float E, float F, float t)
{
	float aB = Lerp(A, B, t);
	float bC = Lerp(B, C, t);
	float cD = Lerp(C, D, t);
	float dE = Lerp(D, E, t);
	float eF = Lerp(E, F, t);
	float aBC = Lerp(aB, bC, t);
	float bCD = Lerp(bC, cD, t);
	float cDE = Lerp(cD, dE, t);
	float dEF = Lerp(dE, eF, t);
	float aBCD = Lerp(aBC, bCD, t);
	float bCDE = Lerp(bCD, cDE, t);
	float cDEF = Lerp(cDE, dEF, t);
	float aBCDE = Lerp(aBCD, bCDE, t);
	float bCDEF = Lerp(bCDE, cDEF, t);
	return Lerp(aBCDE, bCDEF, t);
}

float SmoothStart2(float t)
{
	return t * t;
}

float SmoothStart3(float t)
{
	return t * t * t;
}

float SmoothStart4(float t)
{
	return t * t * t * t;
}

float SmoothStart5(float t)
{
	return t * t * t * t * t;
}

float SmoothStart6(float t)
{
	return t * t * t * t * t * t;
}

float SmoothStop2(float t)
{
	float s = 1.f - t;
	return 1.f - (s * s);
}

float SmoothStop3(float t)
{
	float s = 1.f - t;
	return 1.f - (s * s * s);
}

float SmoothStop4(float t)
{
	float x = 1.f - t;
	return 1.f - (x * x * x * x);
}

float SmoothStop5(float t)
{
	float s = 1.f - t;
	return 1.f - (s * s * s * s * s);
}

float SmoothStop6(float t)
{
	float s = 1.f - t;
	return 1.f - (s * s * s * s * s * s);
}

float SmoothStep3(float t)
{
	return t * t * (3.f - (2.f * t));
}

float SmoothStep5(float t)
{
	return t * t * t * (t * ((6.f * t) - 15.f) + 10.f);
}

float SmoothStep3Range(float value, float start, float end)
{
	if (value <= start)
	{
		return 0.f;
	}
	if (value >= end)
	{
		return 1.f;
	}

	float t = (value - start) / (end - start);
	return t * t * (3.f - (2.f * t));
}

float Hesitate3(float t)
{
	float s = 1 - t;
	return (3.f * ((s * s) * t)) + (t * t * t);
}

float Hesitate5(float t)
{
	float s = 1 - t;
	return (5.f * t * (s * s * s * s)) + (10.f * (t * t * t) * (s * s)) + (t * t * t * t * t);
}

float SmoothMin(float a, float b, float t)
{
	float h = GetClamped((0.5f + (0.5f * ((b - a) / t))), 0.0f, 1.0f);
	return (Lerp(b, a, h) - (t * h * (1.0f - h)));
}

float SmoothMax(float a, float b, float t)
{
	return -SmoothMin(-a, -b, t);
}

float SmoothPulse3(float value, float start, float riseEnd, float fallStart, float end)
{
	float rise = SmoothStep3Range(value, start, riseEnd);
	float fall = 1.f - SmoothStep3Range(value, fallStart, end);
	return rise * fall;
}


//--------------------------------------------------------------------
//Byte utilities
//
float NormalizeByte(unsigned char byte)
{
	return GetFractionWithinRange((float)(byte), 0.f, 255.f);
}

unsigned char DenormalizeByte(float byteFloatValue)
{
	float step = 1.f / 256.f;
	int byteIntValue = GetClampedInt((int)(byteFloatValue / step), 0, 255);

	return static_cast<unsigned char>(byteIntValue);
}
