#pragma once
#include "Game/Chunk.hpp"
#include <vector>
struct Block;
class Chunk;


enum class BlockDirection : int
{
	NORTH,
	EAST,
	SOUTH,
	WEST,
	UP,
	DOWN,
	COUNT
};

class BlockIterator
{
public:
	BlockIterator();
	BlockIterator(Chunk* chunk, int blockIndex);
	~BlockIterator();

	BlockIterator GetIterator(BlockDirection direction) const;
	int GetValidNeighborIterators(BlockIterator* out_neighbors);
	int GetValidNonOpaqueNeighborIterators(BlockIterator* out_neighbors);
	Block* GetBlock() const;
	Chunk* GetChunk() const;
	std::vector<Chunk*> GetEdgeNeighborChunks() const;
	IntVec3 GetGlobalCoords() const;
	Vec3 GetPosition() const;

	static BlockDirection GetOppositeDirection(BlockDirection blockDirection);
	std::vector<BlockIterator> GetAllValidBlockIteratorsInRadius(int radius);
	BlockIterator Walk(int dx, int dy, int dz);



	int GetNorthIndex() const;
	int GetEastIndex() const;
	int GetSouthIndex() const;
	int GetWestIndex() const;
	int GetUpIndex() const;
	int GetDownIndex() const;
	void GetCardinalIndexes_NESWUD(int out_indexes[6]) const;

public:
	Chunk* m_chunk;
	int m_blockIndex = -1;
};

inline Block* BlockIterator::GetBlock() const
{
	if (m_chunk && m_chunk->m_state == ChunkState::ACTIVE)
	{
		return &m_chunk->m_blocks[m_blockIndex];
	}

	return nullptr;
}

