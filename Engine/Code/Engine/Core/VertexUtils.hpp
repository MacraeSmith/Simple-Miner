#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/Vertex_Terrain.hpp"
#include <vector>
#include <array>
struct IntRange;
struct Disc2;
struct OBB2;
struct LineSegment2;
struct Capsule2;
struct Triangle2;
struct Mat44;
struct OBB3;
struct ConvexPoly2;

typedef std::vector<Vertex_PCU> Verts;
typedef std::vector<Vertex_PCUTBN> VertTBNs;
typedef std::vector<Vertex_Terrain> TerrainVerts;
typedef std::vector<unsigned int> IndexList;
typedef std::array<Vertex_PCU,3> Triangle_PCU;
typedef std::array<Vertex_PCUTBN, 3> Triangle_PCUTBN;

Verts GetPCUsFromPCUTBNs(VertTBNs const& vertTBNs);
void AppendVertexPCUs(Verts& out_verts, Verts const& vertsToAdd);
void AppendVertexPCUTBNs(VertTBNs out_verts, VertTBNs const& vertsToAdd);

void TransformVertexArrayXY3D(int numVerts, Vertex_PCU* verts, float uniformScaleXY, float rotationDegreesAboutZ, Vec2 const& translationXY);

//uses I J coordinate system
void TransformVertexArrayXY3D(int numVerts, Vertex_PCU* verts, Vec2 const& vectorFwrd, Vec2 const& vectorLeft, Vec2 const& translationXY);
void TransformVertexArrayXY3D(Verts& verts, Vec2 const& vectorFwrd, Vec2 const& vectorLeft, Vec2 const& translationXY);
void TransformVertexArrayXY3D(Verts& verts, Vec2 const& vectorFwrd, Vec2 const& vectorLeft, Vec2 const& translationXY, IntRange const& vertsToChangeIndexRange);
void TransformVertexArray3D(Verts& verts, Mat44 const& transform);
void TransformVertexArray3D(Verts& verts, Mat44 const& transform, IntRange const& vertsToChangeIndexRange);
void TransformVertexArray3D(VertTBNs& verts, Mat44 const& transform, IntRange const& vertsToChangeIndexRange);

void ChangeColorsOfVertexArray(int numVerts, Vertex_PCU* verts, Rgba8 const& color);

//Add verts for 2D shapes
//-------------------------------------------------------------------------------
void AddVertsForDisc2D(Verts& verts, Vec2 const& discCenter, float const& discRadius, Rgba8 const& color, int numSlices = 32);
void AddVertsForDisc2D(Verts& verts, Disc2 const& disc, Rgba8 const& color);

void AddVertsForAABB2D(Verts& verts, AABB2 const& alignedBox, Rgba8 const& color, Vec2 const& uvMins, Vec2 const& uvMaxs);
void AddVertsForAABB2D(Verts& verts, AABB2 const& alignedBox, Rgba8 const& color = Rgba8::WHITE , AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForAABB2DFrame(Verts& verts, AABB2 const& alignedBox, float thickness, Rgba8 const& color);

void AddVertsForOBB2D(Verts& verts, OBB2 const& orientedBox, Rgba8 const& color);
void AddVertsForOBB2D(Verts& verts, Vec2 const& center, Vec2 const& iBasisNormal, Vec2 const& halfDimensionsIJ, Rgba8 const& color);

void AddVertsForCapsule2D(Verts& verts, Vec2 const& boneStart, Vec2 const& boneEnd, float const& radius, Rgba8 const& color);
void AddVertsForCapsule2D(Verts& verts, Capsule2 const& capsule, Rgba8 const& color);

void AddVertsForTriangle2D(Verts& verts, Vec2 const& triPointACounterClockwise, Vec2 const& triPointBCounterClockwise, Vec2 const& triPointCCounterClockwise, Rgba8 const& color);
void AddVertsForTriangle2D(Verts& verts,Triangle2 const& triangle, Rgba8 const& color);

void AddVertsForLineSegment2D(Verts& verts, Vec2 const& start, Vec2 const& end, float const& thickness, Rgba8 const& color);
void AddVertsForLineSegment2D(Verts& verts, Vec2 const& start, Vec2 const& end, float const& thickness, Rgba8 const& startColor, Rgba8 const& endColor);
void AddVertsForLineSegment2D(Verts& verts, LineSegment2 const& lineSegment, float const& thickness, Rgba8 const& color);
void AddVertsForLineSegment2D(Verts& verts, LineSegment2 const& lineSegment, float const& thickness, Rgba8 const& startColor, Rgba8 const& endColor);

void AddVertsForArrow2D(Verts& verts, Vec2 const& start, Vec2 const& end, float arrowSize, float thickness, Rgba8 const& color);

void AddVertsForRing2D(Verts& verts, Vec2 const& center, float radius, float thickness, Rgba8 const& color);

void AddVertsForConvexPoly2(Verts& verts, ConvexPoly2 const& poly, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForConvexPoly2Edge(Verts& verts, ConvexPoly2 const& poly, float thickness, Rgba8 const& color = Rgba8::WHITE);

//Add verts for 3D shapes
//-------------------------------------------------------------------------------
void AddVertsForQuad3D(Verts& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedQuad3D(Verts& verts, std::vector<unsigned int>& indexes, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedQuad3D(VertTBNs& verts, std::vector<unsigned int>& indexes, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForRoundedQuad(VertTBNs& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedGerstnerQuadMesh(VertTBNs& verts, IndexList& indexes, IntVec2 const& subDivisionsXY, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);

void AddVertsForAABB3D(Verts& verts, AABB3 const& bounds, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForAABB3D(Verts& verts, AABB3 const& bounds, Rgba8* faceColors_XPos_XNeg_YPos_YNeg_ZPos_ZNeg, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedAABB3D(Verts& verts, IndexList& indexes, AABB3 const& bounds, Rgba8* faceColors_XPos_XNeg_YPos_YNeg_ZPos_ZNeg, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedAABB3D(Verts& verts, IndexList& indexes, AABB3 const& bounds, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForInvertedIndexedAABB3D(Verts& verts, IndexList& indexes, AABB3 const& bounds, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedAABB3D(VertTBNs& verts, std::vector<unsigned int>& indexes, AABB3 const& bounds, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForSkybox(Verts& verts, IndexList& indexes, AABB3 const& bounds, AABB2 const& uvs = AABB2::ZERO_TO_ONE);

void AddVertsForOBB3D(Verts& verts, OBB3 const& orientedBox, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedOBB3D(VertTBNs& verts, std::vector<unsigned int>& indexes, OBB3 const& orientedBox, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);

void AddVertsForGridPlane3D(Verts& verts, IntVec2 const& xyUnitDimensions, Vec3 const& center = Vec3::ZERO, int unitScale = 1, int accentUnitScale = 5, float lineThickness = 0.01f);
void AddVertsForGridPlane3D(Verts& verts, Vec3 const& planeNormal, float planeDistanceFromOriginAlongNormal, IntVec2 const& visibleDimensions, int unitScale = 1, int accentUnitScale = 5, float lineThickness = 0.01f);
void AddVertsForSingleColoredXYGridPlane3D(Verts& verts, IntVec2 const& xyUnitDimensions, Vec3 const& center = Vec3::ZERO, int unitScale = 1, float lineThicknes = 0.01f, Rgba8 const& color = Rgba8::GREY);

void AddVertsForArrow3D(Verts& verts, Vec3 const& start, Vec3 const& direction, float length, float thickness = 0.25f, int numSlices = 16, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);

void AddVertsForUVSphereZ3D(Verts& verts, Vec3 const& center, float radius, int numSlices = 32, int numStacks = 16, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedZSphere3D(VertTBNs& verts, IndexList& indexes, Vec3 const& center, float radius, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE, int numSlices = 32, int numStacks = 16);
void AddVertsForIndexedZSphere3D(Verts& verts, IndexList& indexes, Vec3 const& center, float radius, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE, int numSlices = 32, int numStacks = 16);

void AddVertsForInvertedIndexedZSphere3D(Verts& verts, std::vector<unsigned int>& indexes, Vec3 const& center, float radius, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE, int numSlices = 32, int numStacks = 16);

void AddVertsForCylinder3D(Verts& verts, Vec3 const& start, Vec3 const& end, float radius, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE, int numSlices = 16);
void AddVertsForZCylinder3D(Verts& verts, Vec3 const& bottom, float length, float radius, int numSlices = 16, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedZCylinder3D(Verts& verts, IndexList& indexes, Vec3 const& bottom, float length, float radius, int numSlices = 16, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForToplessIndexedZCylinder3D(Verts& verts, IndexList& indexes, Vec3 const& bottom, float length, float radius, int numSlices = 16, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedZCylinder3D(VertTBNs& verts, std::vector<unsigned int>& indexes, Vec3 const& bottom, float length, float radius, 
	Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE, int numSlices = 16);

void AddVertsForIndexedZCylinder3D(VertTBNs& verts,IndexList& indexes, Vec3 const& bottom, float length, float radius, int numSlices, int numSegments, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);

void AddVertsForCone3D(Verts& verts, Vec3 const& start, Vec3 const& end, float radius, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE, int numSlices = 16);
void AddVertsForIndexedCone3D(VertTBNs& verts, std::vector<unsigned int>& indexes, Vec3 const& start, Vec3 const& end, float radius, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE, int numSlices = 16);
void AddVertsForLineSegment3D(Verts& verts, Vec3 const& start, Vec3 const& end, float thickness, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE, float xNudgeFrac = 0.35f);
void AddVertsForIndexedLineSegment3D(Verts& verts, IndexList& indexes, Vec3 const& start, Vec3 const& end, float thickness, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE, float xNudgeFrac = 0.35f);
void AddVertsFor3DAsterisk(Verts& verts, Vec3 const& center, float radius, float lineThickness, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);

void AddVertsForGrassBlade(Verts& verts, Vec3 const& bottom, float width, float height, int numSegments, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForGrassBlade(VertTBNs& verts, IndexList& indexes, Vec3 const& bottom, float width, float height, int numSegments, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);

void AddVertsForSphereCappedCylinder(VertTBNs& verts, IndexList& indexes, Vec3 const& bottom, float radius, float length, int numSlices, int numCylinderStacks, int numCapStacks, Rgba8 const& tint, AABB2 const& uvs = AABB2::ZERO_TO_ONE);

//WireFrames
void AddVertsForWireFrameAABB3D(Verts& verts, AABB3 const& bounds, float lineThickness, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedWireFrameAABB3D(Verts& verts, IndexList& indexes, AABB3 const& bounds, float lineThickness, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForWireFrameOBB3D(Verts& verts, OBB3 const& orientedBox, float lineThickness, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForWireFrameZCylinder3D(Verts& verts, float lineThickness, Vec3 const& bottom, float length, float radius, int numSlices = 16, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForWireFrameZSphere3D(Verts& verts, float lineThickness, Vec3 const& center, float radius, int numSlices = 32, int numStacks = 16, Rgba8 const& tint = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
void AddVertsForWireFrameCone3D(Verts& verts, float lineThickness, Vec3 const& start, Vec3 const& end, float radius, Rgba8 const& tint = Rgba8::WHITE, int numSlices = 16);
void AddVertsForWireFrameQuad3D(Verts& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, float thickness, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uvs = AABB2::ZERO_TO_ONE);
