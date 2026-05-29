#include "Game/BlockIterator.hpp"
#include "Engine/Math/IntVec3.hpp"

BlockIterator::BlockIterator()
	:m_chunk(nullptr)
	,m_blockIndex(-1)
{
}

BlockIterator::BlockIterator(Chunk* chunk, int blockIndex)
	:m_chunk(chunk)
	,m_blockIndex(blockIndex)
{
}

BlockIterator::~BlockIterator()
{
	m_chunk = nullptr;
}

/*
BlockIterator BlockIterator::GetIterator(BlockDirection direction)
{
	if(!m_chunk)
		return BlockIterator(nullptr, -1);

	constexpr int NORTH = 0;
	constexpr int EAST = 1;
	constexpr int SOUTH = 2;
	constexpr int WEST = 3;

	switch (direction)
	{
	case BlockDirection::NORTH:
	{
		int y = (m_blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X;
		if(y == CHUNK_MAX_Y)
			return BlockIterator(m_chunk->m_neighbors[NORTH], m_blockIndex & ~CHUNK_MASK_Y);
		
		return BlockIterator(m_chunk, m_blockIndex + STRIDE_Y);
	}
	case BlockDirection::EAST:
	{
		int x = (m_blockIndex & CHUNK_MASK_X);
		if(x == CHUNK_MAX_X)
			return BlockIterator(m_chunk->m_neighbors[EAST], m_blockIndex & ~CHUNK_MASK_X);

		return BlockIterator(m_chunk, m_blockIndex + STRIDE_X);
	}
		
	case BlockDirection::SOUTH:
	{
		int y = (m_blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X;
		if (y == 0)
			return BlockIterator(m_chunk->m_neighbors[SOUTH], m_blockIndex | CHUNK_MASK_Y);

		return BlockIterator(m_chunk, m_blockIndex - STRIDE_Y);
	}
	case BlockDirection::WEST:
	{
		int x = (m_blockIndex & CHUNK_MASK_X);
		if (x == 0)
			return BlockIterator(m_chunk->m_neighbors[WEST], (m_blockIndex & ~CHUNK_MASK_X) | CHUNK_MAX_X);

		return BlockIterator(m_chunk, m_blockIndex - STRIDE_X);
	}
	case BlockDirection::UP:
	{
		int z = (m_blockIndex & CHUNK_MASK_Z) >> (CHUNK_BITS_X + CHUNK_BITS_Y);
		if(z == CHUNK_MAX_Z)
			return BlockIterator(nullptr, -1); // invalid iterator exceeds chunk bounds

		return BlockIterator(m_chunk, m_blockIndex + STRIDE_Z);
	}
	case BlockDirection::DOWN:
	{
		int z = (m_blockIndex & CHUNK_MASK_Z) >> (CHUNK_BITS_X + CHUNK_BITS_Y);
		if (z == 0)
			return BlockIterator(nullptr, -1); // invalid iterator is below chunk bounds

		return BlockIterator(m_chunk, m_blockIndex - STRIDE_Z);
	}
	default:
		return BlockIterator(m_chunk, m_blockIndex);
	}
}
*/

BlockIterator BlockIterator::GetIterator(BlockDirection direction) const
{
	if (!m_chunk)
		return BlockIterator(nullptr, -1);

	IntVec3 local;
	local.x = m_blockIndex & CHUNK_MASK_X;
	local.y = (m_blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X;
	local.z = (m_blockIndex & CHUNK_MASK_Z) >> (CHUNK_BITS_X + CHUNK_BITS_Y);

	Chunk* neighbor = nullptr;

	switch (direction)
	{
	case BlockDirection::NORTH:
		if (local.y == CHUNK_MAX_Y)
		{
			neighbor = m_chunk->m_neighbors[(int)BlockDirection::NORTH];
			if (!neighbor) return BlockIterator(nullptr, -1);
			local.y = 0;  // wrap to neighbor
		}
		else
		{
			local.y += 1;
			neighbor = m_chunk;
		}
		break;

	case BlockDirection::SOUTH:
		if (local.y == 0)
		{
			neighbor = m_chunk->m_neighbors[(int)BlockDirection::SOUTH];
			if (!neighbor) return BlockIterator(nullptr, -1);
			local.y = CHUNK_MAX_Y;
		}
		else
		{
			local.y -= 1;
			neighbor = m_chunk;
		}
		break;

	case BlockDirection::EAST:
		if (local.x == CHUNK_MAX_X)
		{
			neighbor = m_chunk->m_neighbors[(int)BlockDirection::EAST];
			if (!neighbor) return BlockIterator(nullptr, -1);
			local.x = 0;
		}
		else
		{
			local.x += 1;
			neighbor = m_chunk;
		}
		break;

	case BlockDirection::WEST:
		if (local.x == 0)
		{
			neighbor = m_chunk->m_neighbors[(int)BlockDirection::WEST];
			if (!neighbor) return BlockIterator(nullptr, -1);
			local.x = CHUNK_MAX_X;
		}
		else
		{
			local.x -= 1;
			neighbor = m_chunk;
		}
		break;

	case BlockDirection::UP:
		if (local.z == CHUNK_MAX_Z)
			return BlockIterator(nullptr, -1);
		local.z += 1;
		neighbor = m_chunk;
		break;

	case BlockDirection::DOWN:
		if (local.z == 0)
			return BlockIterator(nullptr, -1);
		local.z -= 1;
		neighbor = m_chunk;
		break;

	default:
		return BlockIterator(m_chunk, m_blockIndex);
	}

	// Rebuild new block index safely
	int newIndex =
		local.x +
		(local.y << CHUNK_BITS_X) +
		(local.z << (CHUNK_BITS_X + CHUNK_BITS_Y));

	return BlockIterator(neighbor, newIndex);
}



int BlockIterator::GetValidNeighborIterators(BlockIterator* out_neighbors)
{
	int count = 0;
	for (int i = 0; i < (int)BlockDirection::COUNT; ++i)
	{
		BlockIterator it = GetIterator((BlockDirection)i);
		if (it.GetBlock())
			out_neighbors[count++] = it;
	}
	return count;
}

int BlockIterator::GetValidNonOpaqueNeighborIterators(BlockIterator* out_neighbors)
{
	int count = 0;
	for (int i = 0; i < (int)BlockDirection::COUNT; ++i)
	{
		BlockIterator it = GetIterator((BlockDirection)i);
		Block* b = it.GetBlock();
		if (b && !b->IsFullOpaque())
			out_neighbors[count++] = it;
	}
	return count;
}

Chunk* BlockIterator::GetChunk() const
{
	if (m_chunk && m_chunk->m_state == ChunkState::ACTIVE)
	{
		return m_chunk;
	}

	return nullptr;
}

std::vector<Chunk*> BlockIterator::GetEdgeNeighborChunks() const
{
	std::vector<Chunk*> neighbors;
	if (!m_chunk)
		return neighbors;

	// Compute local voxel coords from blockIndex
	const int localX = m_blockIndex & CHUNK_MASK_X;
	const int localY = (m_blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X;

	// X edges
	if (localX == 0)
	{
		Chunk* west = m_chunk->m_neighbors[(int)BlockDirection::WEST];
		if (west) neighbors.push_back(west);
	}
	else if (localX == CHUNK_MAX_X)
	{
		Chunk* east = m_chunk->m_neighbors[(int)BlockDirection::EAST];
		if (east) neighbors.push_back(east);
	}

	// Y edges
	if (localY == 0)
	{
		Chunk* south = m_chunk->m_neighbors[(int)BlockDirection::SOUTH];
		if (south) neighbors.push_back(south);
	}
	else if (localY == CHUNK_MAX_Y)
	{
		Chunk* north = m_chunk->m_neighbors[(int)BlockDirection::NORTH];
		if (north) neighbors.push_back(north);
	}

	return neighbors;
}

IntVec3 BlockIterator::GetGlobalCoords() const
{
	if(!m_chunk)
		return IntVec3(-1,-1,-1);

	return m_chunk->GetBlockGlobalCoordsFromIndex(m_blockIndex);;
}

BlockDirection BlockIterator::GetOppositeDirection(BlockDirection blockDirection)
{
	switch (blockDirection)
	{
	case BlockDirection::NORTH: return BlockDirection::SOUTH;
	case BlockDirection::SOUTH: return BlockDirection::NORTH;
	case BlockDirection::EAST:  return BlockDirection::WEST;
	case BlockDirection::WEST:  return BlockDirection::EAST;
	default: return blockDirection;
	}
}

std::vector<BlockIterator> BlockIterator::GetAllValidBlockIteratorsInRadius(int radius)
{
	std::vector<BlockIterator> out;
	out.reserve((radius * 2 + 1) * (radius * 2 + 1) * (radius * 2 + 1));

	for (int dz = -radius; dz <= radius; dz++)
	{
		for (int dy = -radius; dy <= radius; dy++)
		{
			for (int dx = -radius; dx <= radius; dx++)
			{
				int distSq = dx * dx + dy * dy + dz * dz;
				if (distSq > radius * radius)
					continue;

				BlockIterator it = Walk(dx, dy, dz);
				if (it.m_chunk && it.m_chunk->m_state == ChunkState::ACTIVE)
					out.push_back(it);
			}
		}
	}

	return out;
}

BlockIterator BlockIterator::Walk(int dx, int dy, int dz)
{
	BlockIterator iter = *this;

	// Move in x
	if (dx > 0)  for (int i = 0; i < dx; i++) iter = iter.GetIterator(BlockDirection::EAST);
	if (dx < 0)  for (int i = 0; i < -dx; i++) iter = iter.GetIterator(BlockDirection::WEST);

	// Move in y
	if (dy > 0)  for (int i = 0; i < dy; i++) iter = iter.GetIterator(BlockDirection::NORTH);
	if (dy < 0)  for (int i = 0; i < -dy; i++) iter = iter.GetIterator(BlockDirection::SOUTH);

	// Move in z
	if (dz > 0)  for (int i = 0; i < dz; i++) iter = iter.GetIterator(BlockDirection::UP);
	if (dz < 0)  for (int i = 0; i < -dz; i++) iter = iter.GetIterator(BlockDirection::DOWN);

	return iter;
}



Vec3 BlockIterator::GetPosition() const
{
	return Vec3(GetGlobalCoords());
}


int BlockIterator::GetNorthIndex() const
{
	int y = (m_blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X;
	return (y + 1 >= CHUNK_SIZE_Y) ? -1 : m_blockIndex + STRIDE_Y;
}

int BlockIterator::GetEastIndex() const
{
	int x = m_blockIndex & CHUNK_MASK_X;
	return (x + 1 >= CHUNK_SIZE_X) ? -1 : m_blockIndex + 1;
}

int BlockIterator::GetSouthIndex() const
{
	int y = (m_blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X;
	return (y - 1 < 0) ? -1 : m_blockIndex - STRIDE_Y;
}

int BlockIterator::GetWestIndex() const
{
	int x = m_blockIndex & CHUNK_MASK_X;
	return (x - 1 < 0) ? -1 : m_blockIndex - 1;
}

int BlockIterator::GetUpIndex() const
{
	int z = (m_blockIndex & CHUNK_MASK_Z) >> (CHUNK_BITS_X + CHUNK_BITS_Y);
	return (z + 1 >= CHUNK_SIZE_Z) ? -1 : m_blockIndex + STRIDE_Z;
}

int BlockIterator::GetDownIndex() const
{
	int z = (m_blockIndex & CHUNK_MASK_Z) >> (CHUNK_BITS_X + CHUNK_BITS_Y);
	return (z - 1 < 0) ? -1 : m_blockIndex - STRIDE_Z;
}

void BlockIterator::GetCardinalIndexes_NESWUD(int out_indexes[6]) const
{
	int x = m_blockIndex & CHUNK_MASK_X;
	int y = (m_blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X;
	int z = (m_blockIndex & CHUNK_MASK_Z) >> (CHUNK_BITS_X + CHUNK_BITS_Y);

	// +Y
	out_indexes[0] = (y + 1 >= CHUNK_SIZE_Y) ? -1 : m_blockIndex + STRIDE_Y;

	// +X
	out_indexes[1] = (x + 1 >= CHUNK_SIZE_X) ? -1 : m_blockIndex + 1;

	// -Y
	out_indexes[2] = (y - 1 < 0) ? -1 : m_blockIndex - STRIDE_Y;

	// -X
	out_indexes[3] = (x - 1 < 0) ? -1 : m_blockIndex - 1;

	// +Z
	out_indexes[4] = (z + 1 >= CHUNK_SIZE_Z) ? -1 : m_blockIndex + STRIDE_Z;

	// -Z
	out_indexes[5] = (z - 1 < 0) ? -1 : m_blockIndex - STRIDE_Z;
}
