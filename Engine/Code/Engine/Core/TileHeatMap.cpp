#include "Engine/Core/TileHeatMap.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"

TileHeatMap::TileHeatMap(IntVec2 const& dimensions, float defaultValue)
	:m_dimensions(dimensions)
{
	int numTiles = dimensions.x * dimensions.y;
	m_values = new float[numTiles];
	SetAllValues(defaultValue);
}

TileHeatMap::TileHeatMap(int dimensionsX, int dimensionsY, float defaultValue)
	:m_dimensions(IntVec2(dimensionsX, dimensionsY))
{
	int numTiles = dimensionsX * dimensionsY;
	m_values = new float[numTiles];
	SetAllValues(defaultValue);
}

void TileHeatMap::SetAllValues(float value)
{
	int numTiles = m_dimensions.x * m_dimensions.y;
	for (int valueIndex = 0; valueIndex < numTiles; ++valueIndex)
	{
		m_values[valueIndex] = value;
	}
}

void TileHeatMap::SetValue(int index, float value)
{
	m_values[index] = value;
}


float TileHeatMap::GetValue(int index) const
{
	return m_values[index];
}

float TileHeatMap::GetValue(IntVec2 const& coords) const
{
	return GetValue(GetIndexFromCoords(coords));
}

int TileHeatMap::GetIndexFromCoords(IntVec2 const& coords) const
{
	return (coords.y + m_dimensions.x) + coords.x;
}

void TileHeatMap::AddValue(int index, float value)
{
	m_values[index] += value;
}

FloatRange const TileHeatMap::GetRangeOfValues(float specialValueToIgnore) const
{
	FloatRange valueRange(FLT_MAX, -FLT_MAX); //mins = large number and maxs = large negative number to intialize comparisons
	int numTiles = m_dimensions.x * m_dimensions.y;
	for (int tileIndex = 0; tileIndex < numTiles; ++tileIndex)
	{
		float value = m_values[tileIndex];

		if (value == specialValueToIgnore)
			continue;

		valueRange.StretchToIncludeValue(value);
	}

	return valueRange;
}

void TileHeatMap::AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 const& totalBounds, FloatRange const& valueRange, Rgba8 const& lowColor, Rgba8 const& highColor)
{
	float totalWidth = totalBounds.m_maxs.x - totalBounds.m_mins.x;
	float totalHeight = totalBounds.m_maxs.y - totalBounds.m_mins.y;

	float xDims = totalWidth / m_dimensions.x;
	float yDims = totalHeight / m_dimensions.y;
	int currentTileIndex;
	Rgba8 currentColor;

	for (int rowIndex = 0; rowIndex < m_dimensions.y; ++rowIndex)
	{
		for (int columnIndex = 0; columnIndex < m_dimensions.x; ++columnIndex)
		{
			currentTileIndex = (rowIndex * m_dimensions.x) + columnIndex;
			float rowIndexF = (float)rowIndex;
			float columnIndexF = (float)columnIndex;
			AABB2 tileBounds(columnIndexF * xDims, rowIndexF * yDims, (columnIndexF + 1.f) * xDims, (rowIndexF + 1.f) * yDims);
			float valueFraction = RangeMapClamped(m_values[currentTileIndex], valueRange.m_min, valueRange.m_max, 0.f, 1.f);
			currentColor = Rgba8::ColorLerp(lowColor, highColor, valueFraction);
			AddVertsForAABB2D(verts, tileBounds, currentColor);
		}
	}
}

void TileHeatMap::AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 const& totalBounds, FloatRange const& valueRange, float specialValue, Rgba8 const& lowColor, Rgba8 const& highColor, Rgba8 const& specialValueColor)
{
	float totalWidth = totalBounds.m_maxs.x - totalBounds.m_mins.x;
	float totalHeight = totalBounds.m_maxs.y - totalBounds.m_mins.y;

	float xDims = totalWidth / m_dimensions.x;
	float yDims = totalHeight / m_dimensions.y;
	int currentTileIndex;
	Rgba8 currentColor;

	for (int rowIndex = 0; rowIndex < m_dimensions.y; ++rowIndex)
	{
		for (int columnIndex = 0; columnIndex < m_dimensions.x; ++columnIndex)
		{
			currentTileIndex = (rowIndex * m_dimensions.x) + columnIndex;
			if (m_values[currentTileIndex] == specialValue)
			{
				currentColor = specialValueColor;
			}

			else
			{
				float valueFraction = RangeMapClamped(m_values[currentTileIndex], valueRange.m_min, valueRange.m_max, 0.f, 1.f);
				currentColor = Rgba8::ColorLerp(lowColor, highColor, valueFraction);
			}

			float rowIndexF = (float)(rowIndex);
			float columnIndexF = (float)(columnIndex);
			AABB2 tileBounds(columnIndexF * xDims, rowIndexF * yDims, (columnIndexF + 1.f) * xDims, (rowIndexF + 1.f) * yDims);
			AddVertsForAABB2D(verts, tileBounds, currentColor);
		}
	}
}

void TileHeatMap::AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 const& totalBounds, float specialValue, Rgba8 const& lowColor, Rgba8 const& highColor, Rgba8 const& specialValueColor)
{
	float totalWidth = totalBounds.m_maxs.x - totalBounds.m_mins.x;
	float totalHeight = totalBounds.m_maxs.y - totalBounds.m_mins.y;

	float xDims = totalWidth / m_dimensions.x;
	float yDims = totalHeight / m_dimensions.y;
	int currentTileIndex;
	Rgba8 currentColor;
	FloatRange valueRange = GetRangeOfValues(specialValue);

	for (int rowIndex = 0; rowIndex < m_dimensions.y; ++rowIndex)
	{
		for (int columnIndex = 0; columnIndex < m_dimensions.x; ++columnIndex)
		{
			currentTileIndex = (rowIndex * m_dimensions.x) + columnIndex;
			if (m_values[currentTileIndex] == specialValue)
			{
				currentColor = specialValueColor;
			}

			else
			{
				float valueFraction = RangeMapClamped(m_values[currentTileIndex], valueRange.m_min, valueRange.m_max, 0.f, 1.f);
				currentColor = Rgba8::ColorLerp(lowColor, highColor, valueFraction);
			}

			float rowIndexF = (float)(rowIndex);
			float columnIndexF = (float)(columnIndex);
			AABB2 tileBounds(columnIndexF * xDims, rowIndexF * yDims, (columnIndexF + 1.f) * xDims, (rowIndexF + 1.f) * yDims);
			AddVertsForAABB2D(verts, tileBounds, currentColor);
		}
	}
}
