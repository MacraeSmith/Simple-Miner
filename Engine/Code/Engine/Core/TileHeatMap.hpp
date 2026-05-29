#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <vector>

struct Vertex_PCU;
struct AABB2;
struct FloatRange;
struct Rgba8;
class TileHeatMap
{
public:
	explicit TileHeatMap(IntVec2 const& dimensions, float defaultValue = 0.f);
	explicit TileHeatMap(int dimensionsX, int dimensionsY, float defaultValue = 0.f);

	void SetAllValues(float value);
	void SetValue(int index, float value);
	float GetValue(int index) const;
	float GetValue(IntVec2 const& coords) const;
	int GetIndexFromCoords(IntVec2 const& coords) const;
	void AddValue(int index, float value);

	FloatRange const GetRangeOfValues(float specialValueToIgnore) const;

	void AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 const& totalBounds, FloatRange const& valueRange, Rgba8 const& lowColor = Rgba8::BLACK, Rgba8 const& highColor = Rgba8::WHITE);
	void AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 const& totalBounds, FloatRange const& valueRange, float specialValue, Rgba8 const& lowColor = Rgba8::BLACK, Rgba8 const& highColor = Rgba8::WHITE, Rgba8 const& specialValueColor = Rgba8::BLUE);
	void AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 const& totalBounds, float specialValue, Rgba8 const& lowColor = Rgba8::BLACK, Rgba8 const& highColor = Rgba8::WHITE, Rgba8 const& specialValueColor = Rgba8::BLUE);

public:
	float* m_values = nullptr;
	IntVec2 m_dimensions;


};

