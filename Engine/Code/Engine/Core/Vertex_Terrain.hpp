#pragma once
#include "Engine/Math/Vec3.hpp"
#include <cstdint>

struct Vertex_Terrain
{
public:
	Vec3 m_position;
	Vec3 m_normal;

	uint8_t m_splatIndex0 = 0;
	uint8_t m_splatIndex1 = 0;
	uint8_t m_splatIndex2 = 0;
	uint8_t m_splatIndex3 = 0;
	

	uint8_t m_splatWeight0 = 255; // 0..255
	uint8_t m_splatWeight1 = 0;   // 0..255
	uint8_t m_splatWeight2 = 0;   // 0..255
	uint8_t m_splatWeight3 = 0;   // 4th weight could be deduced from the shader, but alignment with take up the space anyway

	

	Vertex_Terrain() {}
	explicit Vertex_Terrain(Vec3 const& position, Vec3 const& normal, uint8_t index0, uint8_t index1, uint8_t index2, uint8_t index3, uint8_t weight0, uint8_t weight1, uint8_t weight2, uint8_t weight3);
	~Vertex_Terrain() {}
	uint8_t QuantizeWeightToByte(float normalizedWeight);
};

