#include "GameCommon.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/RendererDX11.hpp"

void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color)
{
	float halfThickness = 0.5f * thickness;
	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;
	constexpr int NUM_SIDES = 64;
	constexpr int NUM_TRIS = 2 * NUM_SIDES;
	constexpr int NUM_VERTS = 3 * NUM_TRIS;
	Vertex_PCU verts[NUM_VERTS];
	constexpr float DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);

	for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
	{
		//compute angle related terms
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
		float endDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);
		float cosStart = CosDegrees(startDegrees);
		float sinStart = SinDegrees(startDegrees);
		float cosEnd = CosDegrees(endDegrees);
		float sinEnd = SinDegrees(endDegrees);

		//compute inner and outer positions
		Vec3 innerStartPos(center.x + innerRadius * cosStart, center.y + innerRadius * sinStart, 0.0f);
		Vec3 outerStartPos(center.x + outerRadius * cosStart, center.y + outerRadius * sinStart, 0.f);
		Vec3 outerEndPos(center.x + outerRadius * cosEnd, center.y + outerRadius * sinEnd, 0.f);
		Vec3 innerEndPos(center.x + innerRadius * cosEnd, center.y + innerRadius * sinEnd, 0.f);

		//Trapezoid is made of two triangles: ABC and DEF
		//A is inner end; B is inner start; C is outer start
		//D is inner end; E is outer start; F is outer end
		int vertIndexA = (6 * sideNum) + 0;
		int vertIndexB = (6 * sideNum) + 1;
		int vertIndexC = (6 * sideNum) + 2;
		int vertIndexD = (6 * sideNum) + 3;
		int vertIndexE = (6 * sideNum) + 4;
		int vertIndexF = (6 * sideNum) + 5;

		verts[vertIndexA].m_position = innerEndPos;
		verts[vertIndexB].m_position = innerStartPos;
		verts[vertIndexC].m_position = outerStartPos;
		verts[vertIndexA].m_color = color;
		verts[vertIndexB].m_color = color;
		verts[vertIndexC].m_color = color;

		verts[vertIndexD].m_position = innerEndPos;
		verts[vertIndexE].m_position = outerStartPos;
		verts[vertIndexF].m_position = outerEndPos;
		verts[vertIndexD].m_color = color;
		verts[vertIndexE].m_color = color;
		verts[vertIndexF].m_color = color;
	}

	g_renderer->BindTexture(nullptr);
	g_renderer->DrawVertexArray(NUM_VERTS, &verts[0]);
}

void DebugDrawLine2D(Vec2 const& start, Vec2 const& end, float thickness, Rgba8 color)
{
	constexpr int NUM_VERTS = 6;
	Vec2 startEndVector = end - start;
	float halfThickness = thickness * 0.5f;
	Vec2 stepFwrd = halfThickness * startEndVector.GetNormalized();
	Vec2 stepLeft = stepFwrd.GetRotated90Degrees();
	Vec2 endLeftVert = end + stepFwrd + stepLeft;
	Vec2 endRightVert = end + stepFwrd - stepLeft;
	Vec2 startLeftVert = start - stepFwrd + stepLeft;
	Vec2 startRightVert = start - stepFwrd - stepLeft;

	Vec3 startRightPos(startRightVert.x, startRightVert.y, 0.f);
	Vec3 startLeftPos(startLeftVert.x, startLeftVert.y, 0.f);
	Vec3 endRightPos(endRightVert.x, endRightVert.y, 0.f);
	Vec3 endLeftPos(endLeftVert.x, endLeftVert.y, 0.f);

	Vertex_PCU verts[NUM_VERTS];
	verts[0].m_position = startRightPos;
	verts[1].m_position = endLeftPos;
	verts[2].m_position = startLeftPos;
	verts[3].m_position = endLeftPos;
	verts[4].m_position = startRightPos;
	verts[5].m_position = endRightPos;

	verts[0].m_color = color;
	verts[1].m_color = color;
	verts[2].m_color = color;
	verts[3].m_color = color;
	verts[4].m_color = color;
	verts[5].m_color = color;

	g_renderer->BindTexture(nullptr);
	g_renderer->DrawVertexArray(NUM_VERTS, &verts[0]);
}
