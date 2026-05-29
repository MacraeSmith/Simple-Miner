#include "Engine/Core/Vertex_Terrain.hpp"
#include "Engine/Math/MathUtils.hpp"

Vertex_Terrain::Vertex_Terrain(Vec3 const& position, Vec3 const& normal, uint8_t index0, uint8_t index1, uint8_t index2, uint8_t index3, uint8_t weight0, uint8_t weight1, uint8_t weight2, uint8_t weight3)
	: m_position(position)
	, m_normal(normal)
	, m_splatIndex0(index0)
	, m_splatIndex1(index1)
	, m_splatIndex2(index2)
	, m_splatIndex3(index3)
	, m_splatWeight0(weight0)
	, m_splatWeight1(weight1)
	, m_splatWeight2(weight2)
	, m_splatWeight3(weight3)
{

}

uint8_t Vertex_Terrain::QuantizeWeightToByte(float normalizedWeight)
{
	float clampedWeight = GetClamped(normalizedWeight, 0.f, 1.f);
	int weightAsInt = (int)RoundToNearestInt(clampedWeight * 255.f);
	weightAsInt = GetClampedInt(weightAsInt, 0, 255);
	return (uint8_t)weightAsInt;
}

