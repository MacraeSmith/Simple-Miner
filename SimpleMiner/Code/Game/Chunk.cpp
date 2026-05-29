#include "Game/Chunk.hpp"
#include "Game/World.hpp"
#include "Game/BlockDefinition.hpp"
#include "Game/BlockIterator.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/WorldDefinition.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/RendererDX11.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/SmoothNoise.hpp"
#include "Engine/Math/RawNoise.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Time.hpp"
#include <unordered_set>

Chunk::Chunk(World* world, IntVec2 const& chunkCoords)
:m_world(world)
,m_chunkCoords(chunkCoords)
,m_animateTimer(new Timer(1.0, g_game->m_gameClock, false))

{
	int chunkX = m_chunkCoords.x * CHUNK_SIZE_X;
	int chunkY = m_chunkCoords.y * CHUNK_SIZE_Y;
	Vec3 mins(chunkX, chunkY, 0);
	Vec3 maxs(chunkX + CHUNK_SIZE_X, chunkY + CHUNK_SIZE_Y, CHUNK_SIZE_Z);
	m_chunkBounds = AABB3(mins, maxs);
	m_frustrumBoundsRadius = GetDistance3D(m_chunkBounds.GetCenterPos(), m_chunkBounds.m_maxs);
	AddVertsForWireFrameAABB3D(m_debugVerts, m_chunkBounds,0.1f);
}

Chunk::~Chunk()
{	
	m_world = nullptr;
	delete m_vbo;
	m_vbo = nullptr;

	delete m_ibo;
	m_ibo = nullptr;

	delete m_waterVbo;
	m_waterVbo = nullptr;
	delete m_waterIbo;
	m_waterIbo = nullptr;

	delete m_animateTimer;
	m_animateTimer = nullptr;
	
}

void Chunk::Update()
{
	// Skip if not active
	if (m_state != ChunkState::ACTIVE)
		return;

	if (m_isAnimatingIn && m_animateTimer->Tick())
	{
		m_isAnimatingIn = false;
		m_hasAnimatedIn = true;
	}

}

void Chunk::Render(Frustum const& cameraViewFrustrum) const
{
	if (m_vbo && m_ibo && IsChunkInFrustum(cameraViewFrustrum))
	{
		g_renderer->SetBlendMode(BlendMode::OPAQUE);
		g_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_renderer->BindTexture(&m_world->m_worldDef->m_blockSpriteSheet->GetTexture());
		g_renderer->BindShader(m_world->m_worldDef->m_chunkShader);

		Mat44 transform = Mat44();

		if (m_isAnimatingIn)
		{
			Vec3 translation = Vec3::ZERO;
			translation.z = Lerp(-(float)CHUNK_SIZE_Z * 0.75f, 0.f, SmoothStep3(m_animateTimer->GetElapsedFraction()));
			transform = Mat44::MakeTranslation3D(translation);
		}
		g_renderer->SetModelConstants(transform);
		g_renderer->DrawIndexedVertexBuffer(m_vbo, m_ibo, m_ibo->GetNumIndexes());

	}
	

	if (g_debugMode)
	{
		g_renderer->SetBlendMode(BlendMode::OPAQUE);
		g_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_renderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
		g_renderer->BindTexture(nullptr);
		g_renderer->BindShader(nullptr);
		Rgba8 color = Rgba8(100,255,100);
		float distanceSqrdFromPlayer = GetDistanceSquared2D(GetCenterXYCoords(), g_game->m_player->m_position.GetXY());
		if (distanceSqrdFromPlayer > CHUNK_ACTIVATION_RANGE * CHUNK_ACTIVATION_RANGE)
		{
			color = Rgba8::WHITE;
		}

		if (distanceSqrdFromPlayer > CHUNK_DEACTIVATION_RANGE * CHUNK_DEACTIVATION_RANGE)
		{
			color = Rgba8(255,100,100);
		}

		g_renderer->SetModelConstants(Mat44(), color);
		g_renderer->DrawVertexArray(m_debugVerts);
	}
}

void Chunk::RenderWater(Frustum const& cameraViewFrustrum) const
{
	if (m_waterVbo && m_waterIbo && IsChunkInFrustum(cameraViewFrustrum))
	{
		Mat44 transform = Mat44();

		if (m_isAnimatingIn)
		{
			Vec3 translation = Vec3::ZERO;
			translation.z = Lerp(-(float)CHUNK_SIZE_Z * 0.75f, 0.f, SmoothStep3(m_animateTimer->GetElapsedFraction()));
			transform = Mat44::MakeTranslation3D(translation);
		}

		//g_renderer->BindTexture(sceneColor, 1);
		//g_renderer->BindTexture(sceneDepth, 2);
		g_renderer->SetModelConstants(transform);
		g_renderer->DrawIndexedVertexBuffer(m_waterVbo, m_waterIbo, m_waterIbo->GetNumIndexes());
	}
}

void Chunk::InitNeighbors()
{
	constexpr int NORTH = 0;
	constexpr int EAST = 1;
	constexpr int SOUTH = 2;
	constexpr int WEST = 3;

	//Assign all neighbors and make sure they assign me as a neighbor
	for (int i = 0; i < IntVec2::NUM_DIRECTIONS_N_E_S_W; ++i)
	{
		m_neighbors[i] = m_world->GetChunkFromChunkCoords(m_chunkCoords + IntVec2::DIRECTIONS_N_E_S_W[i]);

		if (!m_neighbors[i] || m_neighbors[i]->m_state != ChunkState::ACTIVE)
			continue;

		if (i == NORTH)
		{
			m_neighbors[i]->m_neighbors[SOUTH] = this;
		}
		else if (i == EAST)
		{
			m_neighbors[i]->m_neighbors[WEST] = this;
		}
		else if (i == SOUTH)
		{
			m_neighbors[i]->m_neighbors[NORTH] = this;
		}
		else if (i == WEST)
		{
			m_neighbors[i]->m_neighbors[EAST] = this;
		}
	}
}

void Chunk::ClearNeighbors()
{
	constexpr int NORTH = 0;
	constexpr int EAST = 1;
	constexpr int SOUTH = 2;
	constexpr int WEST = 3;


	//Dereference all neighbors and make sure they dereference me
	for (int i = 0; i < IntVec2::NUM_DIRECTIONS_N_E_S_W; ++i)
	{
		if (m_neighbors[i])
		{
			if (i == NORTH)
			{
				m_neighbors[i]->m_neighbors[SOUTH] = nullptr;
			}
			else if (i == EAST)
			{
				m_neighbors[i]->m_neighbors[WEST] = nullptr;
			}
			else if (i == SOUTH)
			{
				m_neighbors[i]->m_neighbors[NORTH] = nullptr;
			}
			else if (i == WEST)
			{
				m_neighbors[i]->m_neighbors[EAST] = nullptr;
			}
		}

		m_neighbors[i] = nullptr;
	}
}

Chunk* Chunk::GetNeighbor(IntVec3 const& direction) const
{
	// Only horizontal directions are supported
	if (direction.z != 0)
		return nullptr;

	int index = -1;
	if (direction == IntVec3(0, 1, 0))       index = 0; // NORTH
	else if (direction == IntVec3(1, 0, 0))  index = 1; // EAST
	else if (direction == IntVec3(0, -1, 0)) index = 2; // SOUTH
	else if (direction == IntVec3(-1, 0, 0)) index = 3; // WEST
	else
		return nullptr; // invalid direction

	Chunk* neighbor = m_neighbors[index];
	if (neighbor == nullptr)
		return nullptr;

	//Ensure neighbor is safe to access across threads
	ChunkState state = neighbor->m_state.load(std::memory_order_acquire);
	if (state != ChunkState::ACTIVE)
		return nullptr;

	return neighbor;
}

bool Chunk::HasAllNeighbors() const
{
	for (int i = 0; i < IntVec2::NUM_DIRECTIONS_N_E_S_W; ++i)
	{
		if(!m_neighbors[i])
			return false;

		if(m_neighbors[i]->m_state != ChunkState::ACTIVE)
			return false;
	}

	return true;
}

bool Chunk::AllNeighborsFullyActivated() const
{
	for (int i = 0; i < IntVec2::NUM_DIRECTIONS_N_E_S_W; ++i)
	{
		if (!m_neighbors[i])
			return false;

		if (m_neighbors[i]->m_state != ChunkState::ACTIVE)
			return false;

		if(!m_neighbors[i]->m_hasActivatedLighting)
			return false;

		if (!m_neighbors[i]->m_hasActivatedLiquid)
			return false;
	}

	return true;
}



bool Chunk::GrowTreeAtGlobalCoords(IntVec3 const& globalCoords, TreeType treeType)
{
	TreeStamp stamp = m_world->m_worldDef->m_treeStamps[(int)treeType];

	bool isCactus = treeType == TreeType::CACTUS;
	
	float heightOffsetNoise = Get2dNoiseZeroToOne(globalCoords.x, globalCoords.y, m_world->m_worldDef->m_gameSeed + 300);
	int heightOffset = RoundToNearestInt(Lerp((float)stamp.m_heightRange.m_min, (float)stamp.m_heightRange.m_max, heightOffsetNoise));
	int totalTreeHeight = globalCoords.z + stamp.m_baseHeight + heightOffset;
	if (totalTreeHeight >= CHUNK_SIZE_Z)
	{
		if(totalTreeHeight - heightOffset < CHUNK_SIZE_Z)
			heightOffset = 0;

		else
		{
			return false; // if tree height will clip top of chunk, return;
		}
	}

	IntVec3 baseCoords = globalCoords;
	baseCoords.z += 1;
	if (!isCactus)
	{
		for (int i = 0; i < heightOffset; ++i)
		{
			int blockIndex = GetBlockIndexFromGlobalCoords(baseCoords);
			if (blockIndex > 0 && blockIndex < NUM_BLOCKS_IN_CHUNK)
			{
				m_blocks[blockIndex].m_blockType = stamp.m_logType;
				m_blocks[blockIndex].SetIsSolid(true);
				m_blocks[blockIndex].SetIsFullOpaque(!isCactus);
				m_blocks[blockIndex].SetIsVisible(true);
				m_blocks[blockIndex].SetIsLiquid(false);
				m_blocks[blockIndex].SetIsLiquidSource(false);
			}
			baseCoords.z++;
		}
	}

	float xMultiplierNoise = Get2dNoiseNegOneToOne(globalCoords.x, globalCoords.y, m_world->m_worldDef->m_gameSeed);
	float yMultiplierNoise = Get2dNoiseNegOneToOne(globalCoords.x, globalCoords.y, m_world->m_worldDef->m_gameSeed + 20);
	float flipXYNoise = Get2dNoiseNegOneToOne(globalCoords.x, globalCoords.y, m_world->m_worldDef->m_gameSeed + 200);
	bool flipXY = flipXYNoise < 0 ? true : false;
	int xMulitplier = xMultiplierNoise < 0 ? -1 : 1;
	int yMultiplier = yMultiplierNoise < 0 ? -1 : 1;

	uint8_t airType = BlockDefinition::GetBlockDefinitionIndexFromName("Air");

	for (int i = 0; i < (int)stamp.m_treeBlocks.size(); ++i)
	{
		IntVec3 offset(stamp.m_treeBlocks[i].m_offsetFromBase.x * xMulitplier,
			stamp.m_treeBlocks[i].m_offsetFromBase.y * yMultiplier, stamp.m_treeBlocks[i].m_offsetFromBase.z);

		if (flipXY)
		{
			int x = offset.x;
			int y = offset.y;
			offset.x = y;
			offset.y = x;
		}

		IntVec3 currentCoords = baseCoords + offset;
		if(isCactus && offset.z > heightOffset)
			continue;

		{
			int blockIndex = GetBlockIndexFromGlobalCoords(currentCoords);
			if (blockIndex >= 0) // block is inside chunk
			{
				uint8_t blockType = stamp.m_treeBlocks[i].m_blockType;
				
				// small chance of leaves being air which gets higher the further away from the base trunk
				if (m_world->m_worldDef->m_fadeLeaves && blockType == stamp.m_leavesType)
				{
					float airNoise = Get3dNoiseZeroToOne(currentCoords.x, currentCoords.y, currentCoords.z, m_world->m_worldDef->m_gameSeed);
					int maxLateralOffset = GetMax(abs(offset.x), abs(offset.y));

					if (airNoise > 1 - (0.05f * maxLateralOffset))
					{
						blockType = airType;
					}
				}

				bool isNotAir = blockType != airType;
				bool isSolid = isNotAir;
				m_blocks[blockIndex].SetIsSolid(isSolid);
				m_blocks[blockIndex].SetIsFullOpaque(false);
				m_blocks[blockIndex].SetIsVisible(isNotAir);
				m_blocks[blockIndex].SetIsLiquid(false);
				m_blocks[blockIndex].SetIsLiquidSource(false);
				m_blocks[blockIndex].m_blockType = blockType;
			}
		}
	}

	return true;
}

void Chunk::ActivateLightingData()
{
	//sky blocks
	
	/*
	{
		
		//Might be able to move to Load / Generate job functions
		for (int rowNum = 0; rowNum < CHUNK_SIZE_Y; ++rowNum)
		{
			for (int columnNum = 0; columnNum < CHUNK_SIZE_X; ++columnNum)
			{
				int blockIndex = GetBlockIndexFromLocalCoords(IntVec3(columnNum, rowNum, CHUNK_MAX_Z));
				BlockIterator iter(this, blockIndex);
				Block* block = iter.GetBlock();
				BlockIterator lastSkyIter(nullptr, -1);


				while (block && !block->IsVisible())
				{
					block->SetIsSky(true);
					block->SetOutdoorLight(SKY_LIGHT_INFLUENCE);
					block->SetIndoorLight(0);
					lastSkyIter = iter;
					iter = iter.GetIterator(BlockDirection::DOWN);
					block = iter.GetBlock();
				}

				m_world->MarkLightingDirty(lastSkyIter);
			}
		}
		
	

		//Mark air blocks (below trees or overhangs but above terrain) as dirty
		for (int rowNum = 0; rowNum < CHUNK_SIZE_Y; ++rowNum)
		{
			for (int columnNum = 0; columnNum < CHUNK_SIZE_X; ++columnNum)
			{
				int blockIndex = GetBlockIndexFromLocalCoords(IntVec3(columnNum, rowNum, CHUNK_MAX_Z));
				BlockIterator iter(this, blockIndex);
				Block* block = iter.GetBlock();

				while (block && !block->IsVisible())
				{
					iter = iter.GetIterator(BlockDirection::DOWN);
					block = iter.GetBlock();

					
					for (int i = 0; i < (int)BlockDirection::UP; ++i)
					{
						BlockIterator neighborIt = iter.GetIterator((BlockDirection)i);
						if (neighborIt.GetChunk() != this)
							continue;

						Block* neighborBlock = neighborIt.GetBlock();
						if (neighborBlock && !neighborBlock->IsFullOpaque() && !neighborBlock->IsSky())
						{
							m_world->MarkLightingDirty(neighborIt);
						}
					}
					
				}
			}
		}
		
	}

	
	//All emitting blocks
	{
		for (int i = 0; i < NUM_BLOCKS_IN_CHUNK; ++i)
		{
			Block& block = m_blocks[i];
			BlockDefinition* blockDef = BlockDefinition::GetBlockDefinitionFromIndex(block.m_blockType);
			int indoorLighting = blockDef->m_indoorLighting;
			if (indoorLighting > 0)
			{
				BlockIterator iter(this, i);
				m_world->MarkLightingDirty(iter);
			}
		}
	}
	
	*/
	
	
	for (int dir = 0; dir < 4; ++dir)
	{
		Chunk* neighbor = m_neighbors[dir];
		if (neighbor && neighbor->m_hasActivatedLighting)
		{
			// Mark both sides of the shared face ONCE.
			MarkBorderLightBlocksDirty((BlockDirection)dir);
			neighbor->MarkBorderLightBlocksDirty(BlockIterator::GetOppositeDirection((BlockDirection)dir));
		}
	}

	m_hasActivatedLighting = true;
	
}

void Chunk::MarkBorderLightBlocksDirty(BlockDirection side)
{
	if (!m_world) return;

	int zMax = CHUNK_SIZE_Z;
	if (side == BlockDirection::NORTH)
	{
		for (int z = 0; z < zMax; ++z)
		{
			for (int x = 0; x < CHUNK_SIZE_X; ++x)
			{
				int idx = GetBlockIndexFromLocalCoords(IntVec3(x, CHUNK_MAX_Y, z));
				BlockIterator it(this, idx);
				if (Block* b = it.GetBlock(); b && !b->IsFullOpaque())
					m_world->MarkLightingDirty(it);
			}
		}
	}
	else if (side == BlockDirection::SOUTH)
	{
		for (int z = 0; z < zMax; ++z)
		{
			for (int x = 0; x < CHUNK_SIZE_X; ++x)
			{
				int idx = GetBlockIndexFromLocalCoords(IntVec3(x, 0, z));
				BlockIterator it(this, idx);
				if (Block* b = it.GetBlock(); b && !b->IsFullOpaque())
					m_world->MarkLightingDirty(it);
			}
		}
	}
	else if (side == BlockDirection::EAST)
	{
		for (int z = 0; z < zMax; ++z)
		{
			for (int y = 0; y < CHUNK_SIZE_Y; ++y)
			{
				int idx = GetBlockIndexFromLocalCoords(IntVec3(CHUNK_MAX_X, y, z));
				BlockIterator it(this, idx);
				if (Block* b = it.GetBlock(); b && !b->IsFullOpaque())
					m_world->MarkLightingDirty(it);
			}
		}
	}
	else if (side == BlockDirection::WEST)
	{
		for (int z = 0; z < zMax; ++z)
		{
			for (int y = 0; y < CHUNK_SIZE_Y; ++y)
			{
				int idx = GetBlockIndexFromLocalCoords(IntVec3(0, y, z));
				BlockIterator it(this, idx);
				if (Block* b = it.GetBlock(); b && !b->IsFullOpaque())
					m_world->MarkLightingDirty(it);
			}
		}
	}
}

void Chunk::ThreadProcessLightData()
{
	if (m_world->m_isShuttingDown)
		return;

	std::deque<int> dirtyLightBlocks;
	auto markDirty = [&](int blockIndex)
		{
			if (blockIndex < 0 || blockIndex >= NUM_BLOCKS_IN_CHUNK)
				return;

			Block& b = m_blocks[blockIndex];
			if (b.IsLightDirty())
				return;

			b.SetIsLightDirty(true);
			dirtyLightBlocks.push_back(blockIndex);
		};

	
	//sky blocks
	{
		for (int rowNum = 0; rowNum < CHUNK_SIZE_Y; ++rowNum)
		{
			for (int columnNum = 0; columnNum < CHUNK_SIZE_X; ++columnNum)
			{
				int blockIndex = GetBlockIndexFromLocalCoords(IntVec3(columnNum, rowNum, CHUNK_MAX_Z));
				if(blockIndex < 0 || blockIndex >= NUM_BLOCKS_IN_CHUNK)
					continue;

				BlockIterator iter(this, blockIndex);
				Block* block = &m_blocks[blockIndex];
				int lastSkyIndex = -1;

				while ( blockIndex >= 0 && blockIndex < NUM_BLOCKS_IN_CHUNK && !block->IsVisible())
				{
					block->SetIsSky(true);
					block->SetOutdoorLight(SKY_LIGHT_INFLUENCE);
					block->SetIndoorLight(0);
					lastSkyIndex = blockIndex;
					iter = BlockIterator(this, blockIndex);
					blockIndex = iter.GetDownIndex();
					if(blockIndex >= 0 && blockIndex < NUM_BLOCKS_IN_CHUNK)
						block = &m_blocks[blockIndex];
				}

				markDirty(lastSkyIndex);
			}
		}

		auto trySpread = [&](int nIndex)
			{
				if (nIndex < 0 || nIndex >= NUM_BLOCKS_IN_CHUNK)
					return;

				Block& nb = m_blocks[nIndex];
				if (!nb.IsFullOpaque() && !nb.IsSky())
				{
					markDirty(nIndex);
				}
			};

		//Mark air blocks (below trees or overhangs but above terrain) as dirty
		for (int rowNum = 0; rowNum < CHUNK_SIZE_Y; ++rowNum)
		{
			for (int columnNum = 0; columnNum < CHUNK_SIZE_X; ++columnNum)
			{
				int blockIndex = GetBlockIndexFromLocalCoords(IntVec3(columnNum, rowNum, CHUNK_MAX_Z));
				if (blockIndex < 0 || blockIndex >= NUM_BLOCKS_IN_CHUNK)
					continue;

				BlockIterator iter(this, blockIndex);
				Block* block = &m_blocks[blockIndex];

				while (blockIndex >= 0 && blockIndex < NUM_BLOCKS_IN_CHUNK && !block->IsVisible())
				{
					if (blockIndex < 0 || blockIndex >= NUM_BLOCKS_IN_CHUNK)
						break;

					
					block = &m_blocks[blockIndex];

					trySpread(iter.GetNorthIndex());
					trySpread(iter.GetEastIndex());
					trySpread(iter.GetSouthIndex());
					trySpread(iter.GetWestIndex());

					blockIndex = iter.GetDownIndex();
					iter = BlockIterator(this, blockIndex);

				}
			}
		}

	}

	if (m_world->m_isShuttingDown)
		return;
	

	//All emitting blocks
	{
		for (int i = 0; i < NUM_BLOCKS_IN_CHUNK; ++i)
		{
			Block& block = m_blocks[i];
			BlockDefinition* blockDef = BlockDefinition::GetBlockDefinitionFromIndex(block.m_blockType);
			int indoorLighting = blockDef->m_indoorLighting;
			if (indoorLighting > 0)
			{
				markDirty(i);
			}
		}
	}


	const uint8_t LAVA_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Lava");
	
	while (!dirtyLightBlocks.empty())
	{
		if(m_world->m_isShuttingDown)
			return;

		int blockIndex = dirtyLightBlocks.front();
		dirtyLightBlocks.pop_front();
		if(blockIndex < 0 || blockIndex >= NUM_BLOCKS_IN_CHUNK)
			continue;

		Block& block = m_blocks[blockIndex];
		BlockIterator iter(this, blockIndex);
		block.SetIsLightDirty(false);

		uint8_t curIndoor = block.GetIndoorLight();
		uint8_t curOutdoor = block.GetOutdoorLight();
		uint8_t desiredIndoor = 0;
		uint8_t desiredOutdoor = 0;

		// handle emissive opaque blocks
		if (block.IsFullOpaque())
		{
			const BlockDefinition* def = BlockDefinition::GetBlockDefinitionFromIndex(block.m_blockType);
			desiredIndoor = (uint8_t)def->m_indoorLighting;

			if (block.m_blockType == LAVA_TYPE)
			{
				desiredOutdoor = MAX_LIGHT_INFLUENCE;
			}
		}

		else
		{
			uint8_t maxOut = 0;
			uint8_t maxIn = 0;

			auto tryCheckNeighbor = [&](int neighborIndex)
				{
					if(neighborIndex < 0 || neighborIndex >= NUM_BLOCKS_IN_CHUNK)
						return;

					Block& nb = m_blocks[neighborIndex];
					uint8_t nOut = nb.GetOutdoorLight();
					uint8_t nIn = nb.GetIndoorLight();

					if (nOut > 0) --nOut;
					if (nIn > 0) --nIn;

					if (nOut > maxOut) maxOut = nOut;
					if (nIn > maxIn)  maxIn = nIn;
				};

			tryCheckNeighbor(iter.GetNorthIndex());
			tryCheckNeighbor(iter.GetEastIndex());
			tryCheckNeighbor(iter.GetSouthIndex());
			tryCheckNeighbor(iter.GetWestIndex());
			tryCheckNeighbor(iter.GetDownIndex());
			tryCheckNeighbor(iter.GetUpIndex());

			// Sky blocks get full outdoor, others get neighbor max
			desiredOutdoor = block.IsSky() ? SKY_LIGHT_INFLUENCE : maxOut;
			desiredIndoor = maxIn;
		}

		// apply new values
		bool changedLight = false;
		if (curOutdoor != desiredOutdoor)
		{
			block.SetOutdoorLight(desiredOutdoor);
			changedLight = true;
		}
		if (curIndoor != desiredIndoor)
		{
			block.SetIndoorLight(desiredIndoor);
			changedLight = true;
		}

		if (!changedLight)
			continue;

		bool isDarkening = (desiredOutdoor < curOutdoor) || (desiredIndoor < curIndoor);

		auto tryMark = [&](int neighborIndex)
			{
				if(neighborIndex < 0 || neighborIndex >= NUM_BLOCKS_IN_CHUNK)
					return;

				Block& nb = m_blocks[neighborIndex];
				if (nb.IsFullOpaque())
					return;

				uint8_t nOut = nb.GetOutdoorLight();
				uint8_t nIn = nb.GetIndoorLight();

				if (isDarkening)
				{
					if (nOut > desiredOutdoor || nIn > desiredIndoor)
						markDirty(neighborIndex);
				}
				else if ((nOut + 1 < desiredOutdoor) || (nIn + 1 < desiredIndoor))
				{
					markDirty(neighborIndex);
				}
			};

		tryMark(iter.GetNorthIndex());
		tryMark(iter.GetEastIndex());
		tryMark(iter.GetSouthIndex());
		tryMark(iter.GetWestIndex());
		tryMark(iter.GetDownIndex());
		tryMark(iter.GetUpIndex());
	}

}

void Chunk::ActivateLiquidData()
{
	if (!FLOWING_WATER)
	{
		m_hasActivatedLiquid = true;
		return;
	}
		
	// 1. Mark all border blocks as liquid dirty to ensure propagation across chunk boundaries
	{
		MarkBorderLiquidBlocksDirty(BlockDirection::NORTH);
		MarkBorderLiquidBlocksDirty(BlockDirection::SOUTH);
		MarkBorderLiquidBlocksDirty(BlockDirection::EAST);
		MarkBorderLiquidBlocksDirty(BlockDirection::WEST);
	}

	/*
	for (int i = 0; i < NUM_BLOCKS_IN_CHUNK; ++i)
	{
		Block& block = m_blocks[i];
		if(!block.IsLiquid())
			continue;

		block.SetFlowValue(LIQUID_INFLUENCE);
		block.SetIsLiquidSource(true);
		BlockIterator iter(this, i);
		m_world->MarkLiquidDirty(iter);

		// Add this: mark neighbors
		BlockIterator n;

		n = iter.GetIterator(BlockDirection::DOWN);
		Block* nBlock = n.GetBlock();
		if (nBlock && nBlock->IsAir())
		{
			m_world->MarkLiquidDirty(n);
		}

		else
		{
			n = iter.GetIterator(BlockDirection::NORTH);
			m_world->MarkLiquidDirty(n);

			n = iter.GetIterator(BlockDirection::SOUTH);
			m_world->MarkLiquidDirty(n);

			n = iter.GetIterator(BlockDirection::EAST);
			m_world->MarkLiquidDirty(n);

			n = iter.GetIterator(BlockDirection::WEST);
			m_world->MarkLiquidDirty(n);
		}
	}
	*/

	m_hasActivatedLiquid = true;
}

void Chunk::MarkBorderLiquidBlocksDirty(BlockDirection side)
{
	if (!m_world) return;

	int zMax = CHUNK_SIZE_Z;
	if (side == BlockDirection::NORTH)
	{
		for (int z = 0; z < zMax; ++z)
		{
			for (int x = 0; x < CHUNK_SIZE_X; ++x)
			{
				int idx = GetBlockIndexFromLocalCoords(IntVec3(x, CHUNK_MAX_Y, z));
				BlockIterator it(this, idx);
				if (Block* b = it.GetBlock(); b && !b->IsSolid() && !b->IsLiquidSource())
					m_world->MarkLiquidDirty(it);
			}
		}
	}
	else if (side == BlockDirection::SOUTH)
	{
		for (int z = 0; z < zMax; ++z)
		{
			for (int x = 0; x < CHUNK_SIZE_X; ++x)
			{
				int idx = GetBlockIndexFromLocalCoords(IntVec3(x, 0, z));
				BlockIterator it(this, idx);
				if (Block* b = it.GetBlock(); b && !b->IsSolid() && !b->IsLiquidSource())
					m_world->MarkLiquidDirty(it);
			}
		}
	}
	else if (side == BlockDirection::EAST)
	{
		for (int z = 0; z < zMax; ++z)
		{
			for (int y = 0; y < CHUNK_SIZE_Y; ++y)
			{
				int idx = GetBlockIndexFromLocalCoords(IntVec3(CHUNK_MAX_X, y, z));
				BlockIterator it(this, idx);
				if (Block* b = it.GetBlock(); b && !b->IsSolid() && !b->IsLiquidSource())
					m_world->MarkLiquidDirty(it);
			}
		}
	}
	else if (side == BlockDirection::WEST)
	{
		for (int z = 0; z < zMax; ++z)
		{
			for (int y = 0; y < CHUNK_SIZE_Y; ++y)
			{
				int idx = GetBlockIndexFromLocalCoords(IntVec3(0, y, z));
				BlockIterator it(this, idx);
				if (Block* b = it.GetBlock(); b && !b->IsSolid() && !b->IsLiquidSource())
					m_world->MarkLiquidDirty(it);
			}
		}
	}
}

void Chunk::ThreadProcessLiquidData()
{
	if (m_world->m_isShuttingDown)
		return;

	std::deque<int> dirtyLiquidBlocks;

	auto markDirty = [&](int blockIndex)
		{
			if(blockIndex < 0 || blockIndex >= NUM_BLOCKS_IN_CHUNK)
				return;

			Block& b = m_blocks[blockIndex];
			if(b.IsLiquidDirty())
				return;

			b.SetIsLiquidDirty(true);
			dirtyLiquidBlocks.push_back(blockIndex);
		};


	for (int i = 0; i < NUM_BLOCKS_IN_CHUNK; ++i)
	{
		Block& block = m_blocks[i];
		if (!block.IsLiquid())
			continue;

		block.SetFlowValue(LIQUID_INFLUENCE);
		block.SetIsLiquidSource(true);
		markDirty(i);
		BlockIterator iter(this, i);

		int index = iter.GetDownIndex();

		if (index >= 0)
		{
			if (m_blocks[index].IsAir())
			{
				markDirty(index);
			}

			else if(m_blocks[index].IsSolid())
			{
				index = iter.GetNorthIndex();

				if (index >= 0)
					markDirty(index);


				index = iter.GetEastIndex();

				if (index >= 0)
					markDirty(index);

				index = iter.GetSouthIndex();

				if (index >= 0)
					markDirty(index);

				index = iter.GetWestIndex();

				if (index >= 0)
					markDirty(index);

			}
		}
	}

	if(dirtyLiquidBlocks.empty())
		return;

	const uint8_t WATER_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Water");
	const uint8_t STONE_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Stone");
	const uint8_t OBSIDIAN_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Obsidian");
	const uint8_t LAVA_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Lava");

	while (!dirtyLiquidBlocks.empty())
	{
		if(m_world->m_isShuttingDown)
			return;

		int blockIndex = dirtyLiquidBlocks.front();
		dirtyLiquidBlocks.pop_front();
		Block& block = m_blocks[blockIndex];
		block.SetIsLiquidDirty(false);

		if(block.IsSolid())
			continue;

		BlockIterator iter(this, blockIndex);
		int localZ = (blockIndex & CHUNK_MASK_Z) >> (CHUNK_BITS_X + CHUNK_BITS_Y);
		bool isSource = block.IsLiquidSource();
		uint8_t oldFlow = block.GetFlowValue();
		uint8_t desiredFlow = isSource ? LIQUID_INFLUENCE : 0;
		uint8_t curBlockType = block.m_blockType;
		uint8_t desiredBlockType = WATER_TYPE;
		if (block.IsLiquid())
		{
			desiredBlockType = curBlockType;
		}

		//----------------------------------------------------------------------
		// RULE 1: fed from above?
		//----------------------------------------------------------------------

		bool fedFromAbove = false;
		if (localZ < CHUNK_MAX_Z && !isSource)
		{
			int aboveIndex = blockIndex + STRIDE_Z;
			Block& above = m_blocks[aboveIndex];

			if (above.IsLiquid() && !above.IsSolid())
			{
				uint8_t aboveFlow = above.GetFlowValue();
				desiredFlow = aboveFlow > 0 ? LIQUID_INFLUENCE : 0;
				fedFromAbove = aboveFlow > 0;
				desiredBlockType = above.m_blockType;
			}
		}

		//----------------------------------------------------------------------
		// RULE 2: lateral flow if not fed from above
		//----------------------------------------------------------------------


		if (!fedFromAbove && !isSource)
		{
			uint8_t maxFlow = 0;

			auto gather = [&](int nIndex)
				{
					if(nIndex < 0 || nIndex >= NUM_BLOCKS_IN_CHUNK)
						return;

					Block& nb = m_blocks[nIndex];
					if (nb.IsSolid() || !nb.IsLiquid())
						return;

					uint8_t nf = nb.GetFlowValue();

					if (nf == oldFlow)
						return;

					BlockIterator n(this, nIndex);
					int downIndex = n.GetDownIndex();

					if (downIndex < 0 || downIndex >= NUM_BLOCKS_IN_CHUNK)
						return;

					Block& nbDown = m_blocks[downIndex];

					if (nbDown.IsAir() || nbDown.IsLiquid())
						return;

					if (nf > maxFlow)
					{
						desiredBlockType = nb.m_blockType;
						maxFlow = nf;
					}
				};

			gather(iter.GetNorthIndex());
			gather(iter.GetSouthIndex());
			gather(iter.GetEastIndex());
			gather(iter.GetWestIndex());

			if (maxFlow > 0)
				desiredFlow = maxFlow - 1;
			else
				desiredFlow = 0;
		}

		//----------------------------------------------------------------------
		// COMPUTE DRYING *BEFORE* CHANGING FLOW
		//----------------------------------------------------------------------
		bool drying = (desiredFlow < oldFlow) && !isSource;


		//----------------------------------------------------------------------
		// PROPAGATION: DOWN -> sideways if supported
		//----------------------------------------------------------------------
		if (desiredFlow > 0 && !drying)
		{
			int belowIndex = iter.GetDownIndex();
			bool spreadBelow = false;
			if (belowIndex >= 0 && belowIndex < NUM_BLOCKS_IN_CHUNK)
			{
				Block& bb = m_blocks[belowIndex];
				if (bb.IsAir() || bb.IsLiquid())
				{
					markDirty(belowIndex);
					spreadBelow = true;
				}
			}

			if(!spreadBelow)
			{
				// spread sideways if sitting on solid
				auto trySide = [&](int sideIndex)
					{
						if(sideIndex < 0 || sideIndex >= NUM_BLOCKS_IN_CHUNK)
							return;

						Block& sb = m_blocks[sideIndex];
						
						if (sb.IsAir())
							markDirty(sideIndex);

						else
						{
							uint8_t sbFlow = sb.GetFlowValue();
							if (sbFlow < desiredFlow - 1)
							{
								markDirty(sideIndex);
							}

						}
						

					};

				trySide(iter.GetNorthIndex());
				trySide(iter.GetEastIndex());
				trySide(iter.GetSouthIndex());
				trySide(iter.GetWestIndex());
			}
		}

		//----------------------------------------------------------------------
		// DRYING / RE-ADJUSTMENT PROPAGATION (lateral + vertical)
		//----------------------------------------------------------------------
		auto tryDry = [&](int nIndex, bool downNeighbor)
			{
				if(nIndex < 0 || nIndex >= NUM_BLOCKS_IN_CHUNK)
					return;

				Block& nb = m_blocks[nIndex];
				if (nb.IsSolid())
					return;

				uint8_t nf = nb.GetFlowValue();

				// skip empty air with zero flow
				if (!nb.IsLiquid())// && nf == 0)
					return;

				if (drying)
				{
					// drying wave: neighbor has more flow -> re-evaluate
					if (downNeighbor)
					{
						if (nf > desiredFlow)
							markDirty(nIndex);
					}

					else if (nf >= desiredFlow)
						markDirty(nIndex);
				}

				else
				{
					// increasing wave
					if (nf + 1 < desiredFlow)
						markDirty(nIndex);
				}
			};

		tryDry(iter.GetNorthIndex(), false);
		tryDry(iter.GetEastIndex(), false);
		tryDry(iter.GetSouthIndex(), false);
		tryDry(iter.GetWestIndex(), false);
		tryDry(iter.GetDownIndex(), true);


		//----------------------------------------------------------------------
		// FINALLY APPLY THE NEW FLOW AND TYPE
		//----------------------------------------------------------------------

		if (desiredFlow > 0)
		{
			if (!block.IsLiquid())
			{
				desiredBlockType = isSource ? block.m_blockType : desiredBlockType;
				block.m_blockType = desiredBlockType;
				block.SetIsLiquid(true);
				block.SetIsSolid(false);
				block.SetIsFullOpaque(desiredBlockType == LAVA_TYPE);
				block.SetIsVisible(true);
			}
		}

		else
		{
			// become air
			desiredBlockType = 0;
			block.m_blockType = desiredBlockType;
			block.SetIsLiquid(false);
			block.SetIsSolid(false);
			block.SetIsFullOpaque(false);
			block.SetIsVisible(false);
		}

		
		if (SOLIDIFY_LIQUID && (desiredBlockType == LAVA_TYPE || desiredBlockType == WATER_TYPE))
		{
			bool testWater = desiredBlockType == LAVA_TYPE;
			bool shouldSolidify = false;
			bool turnToObsidian = false;
			auto trySolidify = [&](int nIndex, bool downNeighbor)
				{
					if(nIndex < 0 || nIndex >= NUM_BLOCKS_IN_CHUNK)
						return;

					Block& nb = m_blocks[nIndex];

					if (testWater && nb.m_blockType == WATER_TYPE)
					{
						uint8_t nFlow = nb.GetFlowValue();

						if (downNeighbor && nFlow < LIQUID_INFLUENCE)
						{
							markDirty(nIndex);
							return;
						}

						shouldSolidify = true;
						if (nFlow == LIQUID_INFLUENCE)
						{
							turnToObsidian = true;
						}
					}

					else if (!testWater && nb.m_blockType == LAVA_TYPE)
					{
						uint8_t nFlow = nb.GetFlowValue();

						if (downNeighbor && nFlow < LIQUID_INFLUENCE)
						{
							markDirty(nIndex);
							return;
						}

						shouldSolidify = true;
						if (nb.IsLiquidSource())
						{
							turnToObsidian = true;
						}
					}

				};

			trySolidify(iter.GetUpIndex(), false);
			trySolidify(iter.GetNorthIndex(), false);
			trySolidify(iter.GetEastIndex(), false);
			trySolidify(iter.GetSouthIndex(), false);
			trySolidify(iter.GetWestIndex(), false);
			trySolidify(iter.GetDownIndex(), true);


			if (shouldSolidify)
			{
				block.m_blockType = turnToObsidian ? OBSIDIAN_TYPE : STONE_TYPE;
				block.SetIsLiquid(false);
				block.SetIsSolid(true);
				block.SetIsFullOpaque(true);
				block.SetIsVisible(true);
				block.SetIsLiquidSource(false);
				desiredFlow = 0;

				BlockIterator up = iter.GetIterator(BlockDirection::UP);
				int upIndex = up.m_blockIndex;
				if (upIndex >= 0 && upIndex < NUM_BLOCKS_IN_CHUNK)
				{
					markDirty(upIndex);

					markDirty(up.GetNorthIndex());
					markDirty(up.GetEastIndex());
					markDirty(up.GetSouthIndex());
					markDirty(up.GetWestIndex());
				}
			}

		}

		block.SetFlowValue(desiredFlow);

	}

}


//Job System Functions
//------------------------------------------------------------------------------------------------

bool Chunk::TryCreateBlocksFromFile()
{

	std::string fileName = Stringf("Saves/Chunk(%i,%i).chunk", m_chunkCoords.x, m_chunkCoords.y);
	std::vector<uint8_t> fileBuffer;
	int fileSize = FileReadToBuffer(fileBuffer, fileName, true);
	if(fileSize <= (int)sizeof(FileHeader))
		return false; //Could not find or read file, or file was too small to be consistent with file header

	FileHeader* header = reinterpret_cast<FileHeader*>(fileBuffer.data());
	if(header->m_version != CHUNK_VERSION)
		return false; //Chunk version has been updated so file is invalid

	size_t runsStart = sizeof(FileHeader);
	size_t runsBytes = fileBuffer.size() - runsStart;
	if (runsBytes % sizeof(FileRun) != 0)
		return false; // corrupted file
	
	size_t numRuns = runsBytes / sizeof(FileRun);
	FileRun* runs = reinterpret_cast<FileRun*>(fileBuffer.data() + runsStart);

	int blockIndex = 0;
	for (size_t i = 0; i < numRuns; ++i)
	{
		uint8_t type = runs[i].m_blockType;
		uint8_t length = runs[i].m_runLength;
		BlockDefinition* def = BlockDefinition::GetBlockDefinitionFromIndex(type);

		for (uint8_t j = 0; j < length; ++j)
		{
			if (blockIndex >= NUM_BLOCKS_IN_CHUNK)
			{
				// file corrupt: too many blocks
				return false;
			}
			m_blocks[blockIndex].m_blockType = type;
			m_blocks[blockIndex].SetIsFullOpaque(def->m_isOpaque);
			m_blocks[blockIndex].SetIsSolid(def->m_isSolid);
			m_blocks[blockIndex].SetIsVisible(def->m_isVisible);

			bool isLiquid = def->m_isLiquid;
			uint8_t flowValue = isLiquid ? LIQUID_INFLUENCE : 0;
			m_blocks[blockIndex].SetIsLiquid(isLiquid);
			m_blocks[blockIndex].SetIsLiquidSource(isLiquid);
			m_blocks[blockIndex].SetFlowValue(flowValue);

			blockIndex++;
		}
	}

	ThreadProcessLiquidData();
	ThreadProcessLightData();

	return true;
}

void Chunk::CreateProceduralBlocks()
{
	if(m_world->m_isShuttingDown)
		return;

	std::vector<NoiseValues> noiseValuesXY;
	noiseValuesXY.reserve(NOISE_SIZE_2D);

	std::vector<float> heightOffsetsXY;
	heightOffsetsXY.reserve(NOISE_SIZE_2D);

	std::vector<float> terrainSquashFactorsXY;
	terrainSquashFactorsXY.reserve(NOISE_SIZE_2D);

	std::vector<float> baseTerrainHeightsXY;
	baseTerrainHeightsXY.reserve(NOISE_SIZE_2D);

	IntVec3 globalNoiseCoordsStart;
	globalNoiseCoordsStart.x = (int)m_chunkBounds.m_mins.x - MAX_TREE_RADIUS;
	globalNoiseCoordsStart.y = (int)m_chunkBounds.m_mins.y - MAX_TREE_RADIUS;
	const int DEFAULT_TERRAIN_HEIGHT = m_world->m_worldDef->m_defaultTerrainHeight;
	const int WORLD_HALF_HEIGHT = (int)((float)CHUNK_SIZE_Z * 0.5f);

	//Initial Noise Values
	//---------------------------------------------------------------------------
	{
		for (int rowNum = 0; rowNum < NOISE_SIZE_Y; ++rowNum)
		{
			int yCoordGlobal = globalNoiseCoordsStart.y + rowNum;
			for (int columnNum = 0; columnNum < NOISE_SIZE_X; ++columnNum)
			{
				int xCoordGlobal = globalNoiseCoordsStart.x + columnNum;
			
				NoiseValues noiseValues = m_world->Get2DNoiseValuesAtPosition(Vec2(xCoordGlobal, yCoordGlobal));
				noiseValuesXY.push_back(noiseValues);

				float heightOffset = m_world->GetHeightOffsetFromNoiseValues(noiseValues);
				heightOffsetsXY.push_back(heightOffset);
				terrainSquashFactorsXY.push_back(m_world->GetSquashFactorFromNoiseValues(noiseValues));

				float baseTerrainHeightXY = DEFAULT_TERRAIN_HEIGHT + (heightOffset * WORLD_HALF_HEIGHT);
				baseTerrainHeightsXY.push_back(baseTerrainHeightXY);
			}
		}
	}

	
	if (m_world->m_isShuttingDown)
		return;
	

	//Base 3D noise
	//---------------------------------------------------------------------------
	std::vector<float> densityField;
	densityField.reserve(NOISE_SIZE_3D);
	{
		const float MIN_TERRAIN_HEIGHT = (float)m_world->m_worldDef->m_terrainMinHeight;
		const float BASE_SQUASH_STRENGTH = 1.f - m_world->m_worldDef->m_baseTerrainSquashFactor;
		const float FINAL_SQUASH_STRENGTH = 1.f - m_world->m_worldDef->m_finalTerrainSquashFactor;
		for (int layerNum = 0; layerNum < CHUNK_SIZE_Z; ++layerNum)
		{
			int zCoordsGlobal = layerNum;
			int idXY = 0;
			for (int rowNum = 0; rowNum < NOISE_SIZE_Y; ++rowNum)
			{
				int yCoordsGlobal = globalNoiseCoordsStart.y + rowNum;
				for (int columnNum = 0; columnNum < NOISE_SIZE_X; ++columnNum)
				{
					int xCoordsGlobal = globalNoiseCoordsStart.x + columnNum;
					Vec3 globalPos(xCoordsGlobal, yCoordsGlobal, zCoordsGlobal);
					float normalizedZ = GetClampedFractionWithinRange(globalPos.z, MIN_TERRAIN_HEIGHT, CHUNK_SIZE_Z);
					float squashCurve = 4.f * normalizedZ * (1.f - normalizedZ);
					float verticalBias = Lerp(1.f, -1.f, SmoothStep3(normalizedZ));

					float baseTerrainNoise = m_world->GetBaseTerrainDensityAtPosition(globalPos);
					float density = (verticalBias * (1.f - BASE_SQUASH_STRENGTH)) + (baseTerrainNoise * squashCurve * BASE_SQUASH_STRENGTH);


					float heightOffset = heightOffsetsXY[idXY];
					float squashFactor = terrainSquashFactorsXY[idXY];

					float baseTerrainHeightXY = baseTerrainHeightsXY[idXY];
					float terrainOffsetFromBaseHeight = (globalPos.z - baseTerrainHeightXY) /  GetMax(baseTerrainHeightXY, 0.1f);

					density += heightOffset;
					density -= squashFactor * terrainOffsetFromBaseHeight;

					//final squash
					density = (verticalBias * (1.f - FINAL_SQUASH_STRENGTH)) + (density * squashCurve * FINAL_SQUASH_STRENGTH);

					//Add to densityField
					densityField.push_back(density);

					idXY++;
				}
			}
		}
	}

	
	if (m_world->m_isShuttingDown)
		return;
	

	//Terrain Height
	//---------------------------------------------------------------------------
	const int SEA_HEIGHT = m_world->m_worldDef->m_seaHeight;
	std::vector<BiomeType> biomesXY;
	biomesXY.reserve(NOISE_SIZE_2D);
	std::vector<int> terrainHeightsXY;
	terrainHeightsXY.reserve(NOISE_SIZE_2D);
	{
		int idXY = 0;
		constexpr int TOP_LAYER_Z_OFFSET = CHUNK_MAX_Z * NOISE_SIZE_2D;
		for (int rowNum = 0; rowNum < NOISE_SIZE_Y; ++rowNum)
		{
			int yOffset = (rowNum * NOISE_SIZE_X);
			for (int columnNum = 0; columnNum < NOISE_SIZE_X; ++columnNum)
			{
				int densityIndex = TOP_LAYER_Z_OFFSET + yOffset + columnNum;
				int terrainHeight = CHUNK_MAX_Z;

				while (densityField[densityIndex] < 0.f && terrainHeight > 0)
				{
					terrainHeight--;
					int zOffset = terrainHeight * NOISE_SIZE_2D;
					densityIndex = zOffset + yOffset + columnNum;
				}

				terrainHeightsXY.push_back(terrainHeight);
				NoiseValues noiseValues = noiseValuesXY[idXY];
				BiomeType biome = m_world->m_worldDef->GetBiomeTypeFrom2DNoiseValues(noiseValues, terrainHeight, SEA_HEIGHT, noiseValues.m_noiseValues[(int)WorldNoiseType::NOISE_FLUCTUATION]);
				biomesXY.push_back(biome);

				idXY++;
			}
		}
	}
	

	uint8_t airType = BlockDefinition::GetBlockDefinitionIndexFromName("Air");
	uint8_t waterType = BlockDefinition::GetBlockDefinitionIndexFromName("Water");
	uint8_t lavaType = BlockDefinition::GetBlockDefinitionIndexFromName("Lava");
	//Fill blocks
	//---------------------------------------------------------------------------
	{
		uint8_t stoneType = BlockDefinition::GetBlockDefinitionIndexFromName("Stone");

		uint8_t diamondType = BlockDefinition::GetBlockDefinitionIndexFromName("Diamond");
		uint8_t goldType = BlockDefinition::GetBlockDefinitionIndexFromName("Gold");
		uint8_t ironType = BlockDefinition::GetBlockDefinitionIndexFromName("Iron");
		uint8_t coalType = BlockDefinition::GetBlockDefinitionIndexFromName("Coal");
		int idXYZ = 0;
		for (int layerNum = 0; layerNum < CHUNK_SIZE_Z; ++layerNum)
		{
			int zOffset = (layerNum * NOISE_SIZE_2D);
			int globalPosZ = layerNum;
			for (int rowNum = 0; rowNum < CHUNK_SIZE_Y; ++rowNum)
			{
				int globalPosY = rowNum + (int)m_chunkBounds.m_mins.y;
				int yOffset = (rowNum + MAX_TREE_RADIUS) * NOISE_SIZE_X;

				for (int columnNum = 0; columnNum < CHUNK_SIZE_X; ++columnNum)
				{
					int globalPosX = columnNum + (int)m_chunkBounds.m_mins.x;
					int xOffset = (columnNum + MAX_TREE_RADIUS);
					int noiseIndex2D = yOffset + xOffset;
					int noiseIndex3D = zOffset + noiseIndex2D;

					float density = densityField[noiseIndex3D];
					uint8_t blockType = airType;
					int terrainHeight = terrainHeightsXY[noiseIndex2D];
					NoiseValues noiseValues = noiseValuesXY[noiseIndex2D];
					float temperature = noiseValues.m_noiseValues[(int)WorldNoiseType::TEMPERATURE];
					BiomeType biome = biomesXY[noiseIndex2D];

					bool isVisible = false;
					bool isSolid = false;
					bool isFullOpaque = false;
					bool isLiquid = false;

					if (density >= 0.f)
					{
						isVisible = true;
						isSolid = true;
						isFullOpaque = true;
						//Grass / dirt
						if (layerNum == terrainHeight)
						{
							blockType = m_world->m_worldDef->GetSurfaceBlockTypeFromBiome(biome, SEA_HEIGHT, terrainHeight, temperature);

							std::vector<uint8_t> dirtTypes;
							std::vector<IntRange> dirtDepthRanges;

							//dirt layers
							m_world->m_worldDef->GetDirtLayersFromBiome(biome, dirtTypes, dirtDepthRanges);
							float dirtDepthNoise = 0;
							
							if (dirtTypes.size() > 0)
							{
								dirtDepthNoise = Get2dNoiseZeroToOne(globalPosX, globalPosY, m_world->m_worldDef->m_gameSeed);
								int dirtIndex = idXYZ;
								for (int i = 0; i < (int)dirtTypes.size(); ++i)
								{
									if (dirtIndex <= 0)
										break;

									int dirtDepth = RoundToNearestInt(Lerp((float)dirtDepthRanges[i].m_min, (float)dirtDepthRanges[i].m_max, dirtDepthNoise));
									for (int j = 0; j < dirtDepth; ++j)
									{
										BlockIterator iterator(this, dirtIndex);
										dirtIndex = iterator.GetDownIndex();
										if (dirtIndex >= 0 && m_blocks[dirtIndex].m_blockType != airType)
										{
											m_blocks[dirtIndex].SetIsVisible(true);
											m_blocks[dirtIndex].SetIsSolid(true);
											m_blocks[dirtIndex].SetIsFullOpaque(true);
											m_blocks[dirtIndex].SetIsLiquid(false);
											m_blocks[dirtIndex].SetIsLiquidSource(false);
											m_blocks[dirtIndex].m_blockType = dirtTypes[i];
										}
									}
								}
							}
						}

						//Stone / Ore
						else
						{
							blockType = stoneType;
							float oreNoise = Get3dNoiseZeroToOne(globalPosX, globalPosY, globalPosZ, m_world->m_worldDef->m_gameSeed);

							if (oreNoise < 0.000001f)
							{
								blockType = lavaType;
								isLiquid = true;
								isSolid = false;
								isFullOpaque = true;
								isVisible = true;
							}
							else if (oreNoise > 0.99999f)
							{
								blockType = lavaType;
								isLiquid = true;
								isSolid = false;
								isFullOpaque = true;
								isVisible = true;
							}

							else if (oreNoise > 0.999f)
								blockType = diamondType;
							else if (oreNoise > 0.995f)
								blockType = goldType;
							else if (oreNoise > 0.99f)
								blockType = ironType;
							else if (oreNoise > 0.97f)
								blockType = coalType;
						}

					}

					//Water
					else if (layerNum <= SEA_HEIGHT && m_world->m_worldDef->m_fillWater)
					{
						float iceMeltNoise = noiseValues.m_noiseValues[(int)WorldNoiseType::ICE_MELT];
						blockType = WorldDefinition::GetWaterBlockTypeFromBiome(biome, SEA_HEIGHT, layerNum, temperature, iceMeltNoise);
						bool isWater = blockType == waterType;
						isVisible = true;
						isSolid = !isWater;
						isFullOpaque = !isWater;
						isLiquid = isWater;
					}

					m_blocks[idXYZ].SetIsVisible(isVisible);
					m_blocks[idXYZ].SetIsSolid(isSolid);
					m_blocks[idXYZ].SetIsFullOpaque(isFullOpaque);
					m_blocks[idXYZ].SetIsLiquid(isLiquid);
					m_blocks[idXYZ].SetIsLiquidSource(isLiquid);
					m_blocks[idXYZ].m_blockType = blockType;
					idXYZ++;
				}
			}
		}
	}
	
	
	if (m_world->m_isShuttingDown)
		return;
	

	IntVec2 chunkOffset = IntVec2(m_chunkBounds.m_mins.GetXY());
	//Caves
	//---------------------------------------------------------------------------
	{
		int idXYZ = 0;
		for (int layerNum = 0; layerNum < CHUNK_SIZE_Z; ++layerNum)
		{
			int zCoordsGlobal = layerNum;
			for (int rowNum = 0; rowNum < CHUNK_SIZE_Y; ++rowNum)
			{
				int yOffset = (rowNum + MAX_TREE_RADIUS) * NOISE_SIZE_X;
				int yCoordsGlobal = rowNum + chunkOffset.y;
				for (int columNum = 0; columNum < CHUNK_SIZE_X; ++columNum)
				{
					if (m_blocks[idXYZ].m_blockType == waterType)
					{
						idXYZ++;
						continue;
					}
					int xCoordsGlobal = columNum + chunkOffset.x;
					int noiseIndex2D = yOffset + (columNum + MAX_TREE_RADIUS);
					int terrainHeight = terrainHeightsXY[noiseIndex2D];

					//get slope
					float heightCenter = (float)terrainHeight;
					float heightX = (float)terrainHeightsXY[noiseIndex2D + 1];
					float heightY = (float)terrainHeightsXY[noiseIndex2D + NOISE_SIZE_X];

					float dx = heightX - heightCenter;
					float dy = heightY - heightCenter;
					float slope = sqrtf(dx * dx + dy * dy); // magnitude of gradient
					slope = GetClampedZeroToOne(slope / 10.f); // normalize — tune denominator by terrain scale

					NoiseValues noiseValues = noiseValuesXY[noiseIndex2D];
					float tunnelSurfacePierce = noiseValues.m_noiseValues[(int)WorldNoiseType::TUNNEL_SURFACE_PIERCE];

					float caveNoise = m_world->GetCavesNoiseAtPosition(Vec3(xCoordsGlobal, yCoordsGlobal, zCoordsGlobal), terrainHeight, tunnelSurfacePierce, slope);
					if (caveNoise > 0.43f)
					{
						m_blocks[idXYZ].m_blockType = airType;
						m_blocks[idXYZ].SetIsVisible(false);
						m_blocks[idXYZ].SetIsSolid(false);
						m_blocks[idXYZ].SetIsFullOpaque(false);
						m_blocks[idXYZ].SetIsLiquid(false);
						m_blocks[idXYZ].SetIsLiquidSource(false);
					}

					idXYZ++;
				}
			}
		}
	}

	
	if (m_world->m_isShuttingDown)
		return;
	

	// Lava / Obsidian
	//---------------------------------------------------------------------------
	{
		uint8_t obsidianType = BlockDefinition::GetBlockDefinitionIndexFromName("Obsidian");
		int idXY = 0;
		for (int rowNum = 0; rowNum < CHUNK_SIZE_Y; ++rowNum)
		{
			IntVec2 posXY;
			posXY.y = (chunkOffset.y + rowNum);

			for (int columNum = 0; columNum < CHUNK_SIZE_X; ++columNum)
			{
				posXY.x = (chunkOffset.x + columNum);

				m_blocks[idXY].m_blockType = obsidianType;
				m_blocks[idXY].SetIsVisible(true);
				m_blocks[idXY].SetIsSolid(true);
				m_blocks[idXY].SetIsFullOpaque(true);
				m_blocks[idXY].SetIsLiquid(false);
				m_blocks[idXY].SetIsLiquidSource(false);

				float obsidianDepthNoise = Get2dNoiseZeroToOne(posXY.x, posXY.y, m_world->m_worldDef->m_gameSeed + 25);
				int obsidianDepth = RoundToNearestInt(obsidianDepthNoise);
				obsidianDepth += 2;

				int blockIndex = idXY;
				for (int i = 0; i < obsidianDepth; ++i)
				{
					BlockIterator upIt(this, blockIndex);
					int upIndex = upIt.GetUpIndex();

					if (upIndex >= 0 && upIndex < NUM_BLOCKS_IN_CHUNK)
					{
						if (i == 0)
						{
							m_blocks[upIndex].m_blockType = lavaType;
							m_blocks[upIndex].SetIsVisible(true);
							m_blocks[upIndex].SetIsSolid(false);
							m_blocks[upIndex].SetIsFullOpaque(true);
							m_blocks[upIndex].SetIsLiquid(true);
							m_blocks[upIndex].SetIsLiquidSource(true);
						}

						else
						{
							m_blocks[upIndex].m_blockType = obsidianType;
							m_blocks[upIndex].SetIsVisible(true);
							m_blocks[upIndex].SetIsSolid(true);
							m_blocks[upIndex].SetIsFullOpaque(true);
							m_blocks[upIndex].SetIsLiquid(false);
							m_blocks[upIndex].SetIsLiquidSource(false);
						}

						blockIndex = upIndex;
					}
				}

				idXY++;
			}
		}
	}

	
	
	//Trees
	//---------------------------------------------------------------------------
	if(m_world->m_worldDef->m_noiseInfos[(int)WorldNoiseType::VEGETATION].m_isActive)
	{
		int idXY = 0;
		for (int rowNum = 0; rowNum < NOISE_SIZE_Y; ++rowNum)
		{
			int globalCoordsY = globalNoiseCoordsStart.y + rowNum;
			for (int columnNum = 0; columnNum < NOISE_SIZE_X; ++columnNum)
			{
				int globalCoordsX = globalNoiseCoordsStart.x + columnNum;
				BiomeType biome = biomesXY[idXY];
				std::vector<TreeType> treeTypes;

				if (!WorldDefinition::DoesBiomeHaveTrees(biome, treeTypes))
				{
					idXY++;
					continue;
				}
				
				int terrainHeight = terrainHeightsXY[idXY];

				if (terrainHeight <= SEA_HEIGHT)
				{
					idXY++;
					continue;
				}

				float vegetationNoise = noiseValuesXY[idXY].m_noiseValues[(int)WorldNoiseType::VEGETATION];
				if (vegetationNoise > WorldDefinition::GetMinVegetationSpawnNoiseFromBiome(biome))
				{
					bool growTree = true;
					int northIndexXY = idXY + NOISE_SIZE_X;
					int eastIndexXY = idXY + 1;
					int southIndexXY = idXY - NOISE_SIZE_X;
					int westIndexXY = idXY - 1;

					constexpr int NUM_NEIGHBORS = 4;
					int neighborIndexes[NUM_NEIGHBORS] = {northIndexXY, eastIndexXY, southIndexXY, westIndexXY};
					for (int i = 0; i < NUM_NEIGHBORS; ++i)
					{
						if(neighborIndexes[i] <= 0 || neighborIndexes[i] >= NOISE_SIZE_2D)
							continue;

						float neighborVegetationNoise = noiseValuesXY[neighborIndexes[i]].m_noiseValues[(int)WorldNoiseType::VEGETATION];
						if (neighborVegetationNoise >= vegetationNoise)
						{
							growTree = false;
							break;
						}
					}

					if (growTree && treeTypes.size() >= 1)
					{
						TreeType treeType = treeTypes[0];

						if (treeTypes.size() >= 2)
						{
							float treeNoise = Get2dNoiseZeroToOne(globalCoordsX, globalCoordsY, m_world->m_worldDef->m_gameSeed);
							float treeTypeFrac = Lerp(0.f, (float)treeTypes.size() - 1.f, treeNoise);
							treeType = treeTypes[RoundToNearestInt(treeTypeFrac)];
						}

						IntVec3 treeCoords(globalCoordsX, globalCoordsY, terrainHeight);

						GrowTreeAtGlobalCoords(treeCoords, treeType);
					}
				}

				idXY++;
			}
		}
	}
	

	//Add Wild Grass
	{
		int idXYZ = 0;
		for (int layerNum = 0; layerNum < CHUNK_SIZE_Z; ++layerNum)
		{
			for (int rowNum = 0; rowNum < CHUNK_SIZE_Y; ++rowNum)
			{
				int yOffset = (rowNum + MAX_TREE_RADIUS) * NOISE_SIZE_X;
				int yCoordsGlobal = rowNum + chunkOffset.y;
				for (int columNum = 0; columNum < CHUNK_SIZE_X; ++columNum)
				{
					Block& block = m_blocks[idXYZ];
					BlockDefinition* def = BlockDefinition::GetBlockDefinitionFromIndex(block.m_blockType);

					if (!def->IsSurfaceBlock())
					{
						idXYZ++;
						continue;
					}

					int xCoordsGlobal = columNum + chunkOffset.x;
					int noiseIndex2D = yOffset + (columNum + MAX_TREE_RADIUS);

					NoiseValues noiseValues = noiseValuesXY[noiseIndex2D];
					BiomeType biome = biomesXY[noiseIndex2D];

					std::vector<uint8_t> wildGrassTypes = m_world->m_worldDef->GetWildGrassFromBiome(biome);
					if (wildGrassTypes.size() > 0)
					{
						float wildGrassNoise = noiseValues.m_noiseValues[(int)WorldNoiseType::WILD_GRASS];
						if (wildGrassNoise > m_world->m_worldDef->GetMinWildGrassSpawnNoiseFromBiome(biome))
						{
							float worldNoise = Get2dNoiseZeroToOne(xCoordsGlobal, yCoordsGlobal, m_world->m_worldDef->m_gameSeed);
							float wildGrassFrac = Lerp(0.f, (float)wildGrassTypes.size() - 1.f, worldNoise);
							uint8_t wildGrassType = wildGrassTypes[RoundToNearestInt(wildGrassFrac)];

							BlockIterator iterator(this, idXYZ);
							int wildGrassIndex = iterator.GetUpIndex();
							if (wildGrassIndex >= 0 && m_blocks[wildGrassIndex].m_blockType == airType)
							{
								m_blocks[wildGrassIndex].SetIsVisible(true);
								m_blocks[wildGrassIndex].SetIsSolid(false);
								m_blocks[wildGrassIndex].SetIsFullOpaque(false);
								m_blocks[wildGrassIndex].SetIsLiquid(false);
								m_blocks[wildGrassIndex].SetIsLiquidSource(false);
								m_blocks[wildGrassIndex].m_blockType = wildGrassType;
							}
						}
					}

					idXYZ++;
				}

			}
		}
	}

	
	if (m_world->m_isShuttingDown)
		return;
	

	//WaterFalls
	if(m_world->m_worldDef->m_noiseInfos[(int)WorldNoiseType::WATERFALLS].m_isActive)
	{

		const int WATER_FALL_MIN_HEIGHT = (int)(CHUNK_SIZE_Z * 0.3f);
		const int WATER_FALL_MIN_DROP = 7;
		for (int rowNum = 0; rowNum < CHUNK_SIZE_Y; ++rowNum)
		{
			int yOffset = (rowNum + MAX_TREE_RADIUS) * NOISE_SIZE_X;
			for (int columNum = 0; columNum < CHUNK_SIZE_X; ++columNum)
			{
				int noiseIndex2D = yOffset + (columNum + MAX_TREE_RADIUS);

				NoiseValues noiseValues = noiseValuesXY[noiseIndex2D];
				BiomeType biome = biomesXY[noiseIndex2D];
				int terrainHeight = terrainHeightsXY[noiseIndex2D];
				if (terrainHeight < WATER_FALL_MIN_HEIGHT)
					continue;

				float waterFallNoise = noiseValues.m_noiseValues[(int)WorldNoiseType::WATERFALLS];
				if(waterFallNoise < WorldDefinition::GetMinWaterFallSpawnNoiseFromBiome(biome))
					continue;

				int blockIndex = GetBlockIndexFromLocalCoords(IntVec3(columNum, rowNum, terrainHeight));

				bool spawnWaterFall = false;
				int northIndexXY = noiseIndex2D + NOISE_SIZE_X;
				int eastIndexXY = noiseIndex2D + 1;
				int southIndexXY = noiseIndex2D - NOISE_SIZE_X;
				int westIndexXY = noiseIndex2D - 1;
				int digIndex = -1;

				constexpr int NUM_NEIGHBORS = 4;
				int neighborIndexes[NUM_NEIGHBORS] = { northIndexXY, eastIndexXY, southIndexXY, westIndexXY };
				for (int i = 0; i < NUM_NEIGHBORS; ++i)
				{
					if (neighborIndexes[i] <= 0 || neighborIndexes[i] >= NOISE_SIZE_2D)
						continue;

					int neighborHeight = terrainHeightsXY[neighborIndexes[i]];
					if (terrainHeight - neighborHeight >= WATER_FALL_MIN_DROP)
					{
						spawnWaterFall = true;
					}

					else if (neighborHeight > terrainHeight)
					{
						BlockIterator iter(this, blockIndex);
						if (i == 0)
							digIndex = iter.GetNorthIndex();
						if (i == 1)
							digIndex = iter.GetEastIndex();
						if (i == 2)
							digIndex = iter.GetSouthIndex();
						if (i == 3)
							digIndex = iter.GetWestIndex();
					}
				}

				if (spawnWaterFall && digIndex >= 0 && digIndex < NUM_BLOCKS_IN_CHUNK && m_blocks[digIndex].IsSolid())
				{
					Block& block = m_blocks[digIndex];
					block.m_blockType = waterType;
					block.SetIsLiquid(true);
					block.SetIsLiquidSource(true);
					block.SetIsFullOpaque(false);
					block.SetIsVisible(true);
					block.SetIsSolid(false);
				}

			}

		}

	}

	ThreadProcessLiquidData();
	ThreadProcessLightData();
}

void Chunk::SaveChunkToDisk()
{
	FileHeader header;
	
	for (int i = 0; i < NUM_BLOCKS_IN_CHUNK; ++i)
	{
		Block& block = m_blocks[i];
		if (block.IsLiquid() && !block.IsLiquidSource())
		{
			block.m_blockType = 0; // revert flowing liquid back to air
		}
	}
	

	std::vector<FileRun> runs;
	uint8_t currentBlockType = m_blocks[0].m_blockType;
	uint8_t currentRunLength = 1;

	for (int i = 1; i < NUM_BLOCKS_IN_CHUNK; ++i)
	{
		if (m_blocks[i].m_blockType != currentBlockType || currentRunLength >= 255)
		{
			runs.push_back(FileRun{currentBlockType, currentRunLength});
			currentBlockType = m_blocks[i].m_blockType;
			currentRunLength = 1;
		}

		else
		{
			currentRunLength++;
		}
	}

	runs.push_back(FileRun{ currentBlockType, currentRunLength });

	std::vector<uint8_t> buffer;
	buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&header), reinterpret_cast<uint8_t*>(&header) + sizeof(header));

	buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(runs.data()), reinterpret_cast<uint8_t*>(runs.data()) + (runs.size() * sizeof(FileRun)));

	std::string fileName = Stringf("Saves/Chunk(%i,%i).chunk", m_chunkCoords.x, m_chunkCoords.y);
	FileWriteFromBuffer(buffer, fileName);

}


//Chunk Mesh
//------------------------------------------------------------------------------------------------

void Chunk::CreateTerrainMesh()
{
	// --- Reserve space ---
	Verts opaqueVerts;
	opaqueVerts.reserve(NUM_BLOCKS_IN_CHUNK * 1);
	IndexList opaqueIndices;
	opaqueIndices.reserve(NUM_BLOCKS_IN_CHUNK * 1);

	Verts transparentVerts; // water only
	transparentVerts.reserve(NUM_BLOCKS_IN_CHUNK / 2);
	IndexList transparentIndices;
	transparentIndices.reserve(NUM_BLOCKS_IN_CHUNK / 2);

	const uint8_t WATER_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Water");
	AABB3 blockBounds;
	IntVec3 local;
	const IntVec3 chunkOffset(m_chunkCoords.x * CHUNK_SIZE_X, m_chunkCoords.y * CHUNK_SIZE_Y, 0);
	const float BLOCK_SIZE = 1.0f;
	constexpr float CACTUS_NUDGE = 0.0625f;
	const uint8_t CACTUS_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("CactusLog");

	for (int levelNum = 0; levelNum < CHUNK_SIZE_Z; ++levelNum)
	{
		blockBounds.m_mins.z = (float)(chunkOffset.z + (levelNum * BLOCK_SIZE));
		blockBounds.m_maxs.z = blockBounds.m_mins.z + BLOCK_SIZE;
		local.z = levelNum;

		for (int rowNum = 0; rowNum < CHUNK_SIZE_Y; ++rowNum)
		{
			blockBounds.m_mins.y = (float)(chunkOffset.y + (rowNum * BLOCK_SIZE));
			blockBounds.m_maxs.y = blockBounds.m_mins.y + BLOCK_SIZE;
			local.y = rowNum;

			for (int columNum = 0; columNum < CHUNK_SIZE_X; ++columNum)
			{
				blockBounds.m_mins.x = (float)(chunkOffset.x + (columNum * BLOCK_SIZE));
				blockBounds.m_maxs.x = blockBounds.m_mins.x + BLOCK_SIZE;
				local.x = columNum;

				const int blockIndex = GetBlockIndexFromLocalCoords(local);
				Block& block = m_blocks[blockIndex];
				uint8_t blockType = block.m_blockType;
				if (blockType == 0)
					continue; // air

				BlockDefinition* def = BlockDefinition::GetBlockDefinitionFromIndex(blockType);
				if (!def->m_isVisible)
					continue;

				// --- Determine category ---
				const bool isWater = (blockType == WATER_TYPE);
				const bool isCactus = (blockType == CACTUS_TYPE);
				const bool isWildGrass = def->IsWildGrass();
				const bool isLiquid = block.IsLiquid();

				Verts& verts = isWater ? transparentVerts : opaqueVerts;
				IndexList& indices = isWater ? transparentIndices : opaqueIndices;
				unsigned char alpha = 255;

				AABB2 topUVs = m_world->m_worldDef->m_blockSpriteSheet->GetSpriteUVs(def->m_topSpriteCoords);
				AABB2 sideUVs = m_world->m_worldDef->m_blockSpriteSheet->GetSpriteUVs(def->m_sideSpriteCoords);
				AABB2 bottomUVs = m_world->m_worldDef->m_blockSpriteSheet->GetSpriteUVs(def->m_bottomSpriteCoords);
				AABB3 bounds = blockBounds;

				BlockIterator iter(this, blockIndex);
				float flowFraction = 1.0;
				float sideUVHeight = sideUVs.GetHeight();

				if (isLiquid)
				{
					if (isWater)
					{
						topUVs = AABB2::ZERO_TO_ONE;
						sideUVs = AABB2::ZERO_TO_ONE;
						sideUVs = AABB2::ZERO_TO_ONE;
						sideUVHeight = 1.0f;
					}

					flowFraction = GetClampedFractionWithinRange((float)block.GetFlowValue(), 0.75f, (float)LIQUID_INFLUENCE);
					sideUVs.m_maxs.y = sideUVs.m_mins.y + sideUVHeight * flowFraction;
				}

				BlockIterator upIter = iter.GetIterator(BlockDirection::UP);
				Block* upBlock = upIter.GetBlock();

				bool isTouchingWater = false;
				float outNeighborFlowFrac = 1.f;

				// north (+y)
				//-------------------------------------------------------------------------------------------------------------
				BlockIterator neighborIter = iter.GetIterator(BlockDirection::NORTH);
				if (ShouldRenderFace(def, block, neighborIter, isTouchingWater, outNeighborFlowFrac, true))
				{
					if (isCactus || isWildGrass)
						bounds.m_maxs.y -= CACTUS_NUDGE;

					AABB2 usedUvs = sideUVs;
					if (isLiquid)
					{
						bounds.m_maxs.z -= 1.0f - flowFraction;
						if (outNeighborFlowFrac < 1.f)
						{
							bounds.m_mins.z += outNeighborFlowFrac;
							usedUvs.m_mins.y = sideUVs.m_mins.y + sideUVHeight * outNeighborFlowFrac;
							usedUvs.m_maxs.y = sideUVs.m_mins.y + sideUVHeight * flowFraction;
						}
					}

					Vec3 bl = Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_mins.z);
					Vec3 br = Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_mins.z);
					Vec3 tr = Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_maxs.z);
					Vec3 tl = Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_maxs.z);

					alpha = isTouchingWater ? 255 : 0;
					Rgba8 color(0,0,200, alpha);

					Block* neighborBlock = neighborIter.GetBlock();
					if (neighborBlock && !isWildGrass)
					{
						float outdoorLight = GetFractionWithinRange((float)neighborBlock->GetOutdoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						float indoorLight = GetFractionWithinRange((float)neighborBlock->GetIndoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						color.r = DenormalizeByte(outdoorLight);
						color.g = DenormalizeByte(indoorLight);
					}

					else if (isWildGrass && upBlock)
					{
						float outdoorLight = GetFractionWithinRange((float)upBlock->GetOutdoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						float indoorLight = GetFractionWithinRange((float)upBlock->GetIndoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						color.r = DenormalizeByte(outdoorLight);
						color.g = DenormalizeByte(indoorLight);
						AddVertsForIndexedQuad3D(verts, indices, br, bl, tl, tr, color, usedUvs);
					}


					AddVertsForIndexedQuad3D(verts, indices, bl, br, tr, tl, color, usedUvs);
				}

				// east (+x)
				//-------------------------------------------------------------------------------------------------------------
				isTouchingWater = false;
				neighborIter = iter.GetIterator(BlockDirection::EAST);
				if (ShouldRenderFace(def, block, neighborIter, isTouchingWater, outNeighborFlowFrac, true))
				{
					bounds = blockBounds;
					if (isCactus || isWildGrass)
						bounds.m_maxs.x -= CACTUS_NUDGE;
					AABB2 usedUvs = sideUVs;
					if (isLiquid)
					{
						bounds.m_maxs.z -= 1.0f - flowFraction;
						if (outNeighborFlowFrac < 1.f)
						{
							bounds.m_mins.z += outNeighborFlowFrac;
							usedUvs.m_mins.y = sideUVs.m_mins.y + sideUVHeight * outNeighborFlowFrac;
							usedUvs.m_maxs.y = sideUVs.m_mins.y + sideUVHeight * flowFraction;
						}
					}

					Vec3 bl = Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_mins.z);
					Vec3 br = Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_mins.z);
					Vec3 tr = Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_maxs.z);
					Vec3 tl = Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_maxs.z);

					alpha = isTouchingWater ? 255 : 0;
					Rgba8 color(0, 0, 230, alpha);

					Block* neighborBlock = neighborIter.GetBlock();

					if (neighborBlock && !isWildGrass)
					{
						float outdoorLight = GetFractionWithinRange((float)neighborBlock->GetOutdoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						float indoorLight = GetFractionWithinRange((float)neighborBlock->GetIndoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						color.r = DenormalizeByte(outdoorLight);
						color.g = DenormalizeByte(indoorLight);
					}

					else if (isWildGrass && upBlock)
					{
						float outdoorLight = GetFractionWithinRange((float)upBlock->GetOutdoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						float indoorLight = GetFractionWithinRange((float)upBlock->GetIndoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						color.r = DenormalizeByte(outdoorLight);
						color.g = DenormalizeByte(indoorLight);
						AddVertsForIndexedQuad3D(verts, indices, br, bl, tl, tr, color, usedUvs);
					}


					AddVertsForIndexedQuad3D(verts, indices, bl, br, tr, tl, color, usedUvs);
					
				}

				// south (-y)
				//-------------------------------------------------------------------------------------------------------------
				isTouchingWater = false;
				neighborIter = iter.GetIterator(BlockDirection::SOUTH);
				if (ShouldRenderFace(def, block, neighborIter, isTouchingWater, outNeighborFlowFrac, true))
				{
					bounds = blockBounds;
					if (isCactus || isWildGrass)
						bounds.m_mins.y += CACTUS_NUDGE;
					AABB2 usedUvs = sideUVs;
					if (isLiquid)
					{
						bounds.m_maxs.z -= 1.0f - flowFraction;
						if (outNeighborFlowFrac < 1.f)
						{
							bounds.m_mins.z += outNeighborFlowFrac;
							usedUvs.m_mins.y = sideUVs.m_mins.y + sideUVHeight * outNeighborFlowFrac;
							usedUvs.m_maxs.y = sideUVs.m_mins.y + sideUVHeight * flowFraction;
						}
					}

					Vec3 bl = Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_mins.z);
					Vec3 br = Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_mins.z);
					Vec3 tr = Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_maxs.z);
					Vec3 tl = Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_maxs.z);

					alpha = isTouchingWater ? 255 : 0;
					Rgba8 color(0, 0, 230, alpha);

					Block* neighborBlock = neighborIter.GetBlock();

					if (neighborBlock && !isWildGrass)
					{
						float outdoorLight = GetFractionWithinRange((float)neighborBlock->GetOutdoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						float indoorLight = GetFractionWithinRange((float)neighborBlock->GetIndoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						color.r = DenormalizeByte(outdoorLight);
						color.g = DenormalizeByte(indoorLight);
					}

					else if (isWildGrass && upBlock)
					{
						float outdoorLight = GetFractionWithinRange((float)upBlock->GetOutdoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						float indoorLight = GetFractionWithinRange((float)upBlock->GetIndoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						color.r = DenormalizeByte(outdoorLight);
						color.g = DenormalizeByte(indoorLight);
						AddVertsForIndexedQuad3D(verts, indices, br, bl, tl, tr, color, usedUvs);
					}


					AddVertsForIndexedQuad3D(verts, indices, bl, br, tr, tl, color, usedUvs);
				}

				// west (-x)
				//-------------------------------------------------------------------------------------------------------------
				isTouchingWater = false;
				neighborIter = iter.GetIterator(BlockDirection::WEST);
				if (ShouldRenderFace(def, block, neighborIter, isTouchingWater, outNeighborFlowFrac, true))
				{
					bounds = blockBounds;
					if (isCactus || isWildGrass)
						bounds.m_mins.x += CACTUS_NUDGE;
					AABB2 usedUvs = sideUVs;
					if (isLiquid)
					{
						bounds.m_maxs.z -= 1.0f - flowFraction;
						if (outNeighborFlowFrac < 1.f)
						{
							bounds.m_mins.z += outNeighborFlowFrac;
							usedUvs.m_mins.y = sideUVs.m_mins.y + sideUVHeight * outNeighborFlowFrac;
							usedUvs.m_maxs.y = sideUVs.m_mins.y + sideUVHeight * flowFraction;
						}
					}

					Vec3 bl = Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_mins.z);
					Vec3 br = Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_mins.z);
					Vec3 tr = Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_maxs.z);
					Vec3 tl = Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_maxs.z);

					alpha = isTouchingWater ? 255 : 0;
					Rgba8 color(0, 0, 230, alpha);

					Block* neighborBlock = neighborIter.GetBlock();

					if (neighborBlock && !isWildGrass)
					{
						float outdoorLight = GetFractionWithinRange((float)neighborBlock->GetOutdoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						float indoorLight = GetFractionWithinRange((float)neighborBlock->GetIndoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						color.r = DenormalizeByte(outdoorLight);
						color.g = DenormalizeByte(indoorLight);
					}

					else if (isWildGrass && upBlock)
					{
						float outdoorLight = GetFractionWithinRange((float)upBlock->GetOutdoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						float indoorLight = GetFractionWithinRange((float)upBlock->GetIndoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
						color.r = DenormalizeByte(outdoorLight);
						color.g = DenormalizeByte(indoorLight);
						AddVertsForIndexedQuad3D(verts, indices, br, bl, tl, tr, color, usedUvs);
					}

					AddVertsForIndexedQuad3D(verts, indices, bl, br, tr, tl, color, usedUvs);
				}

				if (!isWildGrass)
				{
					bounds = blockBounds;
					// down (-z)
					//-------------------------------------------------------------------------------------------------------------
					isTouchingWater = false;
					neighborIter = iter.GetIterator(BlockDirection::DOWN);
					if (ShouldRenderFace(def, block, neighborIter, isTouchingWater, outNeighborFlowFrac))
					{
						alpha = isTouchingWater ? 255 : 0;
						Rgba8 color(0, 0, 255, alpha);

						Block* neighborBlock = neighborIter.GetBlock();
						if (neighborBlock)
						{
							float outdoorLight = GetFractionWithinRange((float)neighborBlock->GetOutdoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
							float indoorLight = GetFractionWithinRange((float)neighborBlock->GetIndoorLight(), 0.f, MAX_LIGHT_INFLUENCE);
							color.r = DenormalizeByte(outdoorLight);
							color.g = DenormalizeByte(indoorLight);
						}

						AddVertsForIndexedQuad3D(
							verts, indices,
							Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_mins.z),
							Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_mins.z),
							Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_mins.z),
							Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_mins.z),
							color,
							bottomUVs
						);
					}

					// up (+z)
					//-------------------------------------------------------------------------------------------------------------
					isTouchingWater = false;
					neighborIter = iter.GetIterator(BlockDirection::UP);
					if (ShouldRenderFace(def, block, neighborIter, isTouchingWater, outNeighborFlowFrac, false, true))
					{
						if (isLiquid)
							bounds.m_maxs.z -= 1.0f - flowFraction;

						alpha = isTouchingWater ? 255 : 0;
						Rgba8 color(255, 0, 255, alpha); //default to max outdoor light

						Block* neighborBlock = neighborIter.GetBlock();
						if (neighborBlock)
						{
							uint8_t neighborOutdoorLight = neighborBlock->GetOutdoorLight();
							uint8_t neighborIndoorLight = neighborBlock->GetIndoorLight();

							if (flowFraction < LIQUID_INFLUENCE && upBlock && upBlock->IsFullOpaque())
							{
								GetMaxIndoorAndOutdoorLightFromNeighbors(neighborIndoorLight, neighborOutdoorLight, iter);
							}

							float outdoorLight = GetFractionWithinRange((float)neighborOutdoorLight, 0.f, MAX_LIGHT_INFLUENCE);
							float indoorLight = GetFractionWithinRange((float)neighborIndoorLight, 0.f, MAX_LIGHT_INFLUENCE);
							color.r = DenormalizeByte(outdoorLight);
							color.g = DenormalizeByte(indoorLight);
						}

						AddVertsForIndexedQuad3D(
							verts, indices,
							Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_maxs.z),
							Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_maxs.z),
							Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_maxs.z),
							Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_maxs.z),
							color,
							topUVs
						);
					}
				}
			}
		}
	}

	// Upload to GPU
	if (!opaqueVerts.empty())
	{
		m_vbo = g_renderer->CreateVertexBuffer((unsigned int)(opaqueVerts.size() * sizeof(Vertex_PCU)), sizeof(Vertex_PCU));
		m_ibo = g_renderer->CreateIndexBuffer((unsigned int)(opaqueIndices.size() * sizeof(unsigned int)));
		g_renderer->CopyCPUToGPU(opaqueVerts.data(), (unsigned int)(opaqueVerts.size() * sizeof(Vertex_PCU)), m_vbo);
		g_renderer->CopyCPUToGPU(opaqueIndices.data(), (unsigned int)(opaqueIndices.size() * sizeof(unsigned int)), m_ibo);
	}

	else
		DebugAddMessage("Empty Mesh Trying to generate", 10.f);

	if (!transparentVerts.empty())
	{
		m_waterVbo = g_renderer->CreateVertexBuffer((unsigned int)(transparentVerts.size() * sizeof(Vertex_PCU)), sizeof(Vertex_PCU));
		m_waterIbo = g_renderer->CreateIndexBuffer((unsigned int)(transparentIndices.size() * sizeof(unsigned int)));
		g_renderer->CopyCPUToGPU(transparentVerts.data(), (unsigned int)(transparentVerts.size() * sizeof(Vertex_PCU)), m_waterVbo);
		g_renderer->CopyCPUToGPU(transparentIndices.data(), (unsigned int)(transparentIndices.size() * sizeof(unsigned int)), m_waterIbo);
	}

	// Optional: start animation in if needed
	if (!m_hasAnimatedIn && !m_isAnimatingIn)
	{
		m_animateTimer->Restart();
		m_isAnimatingIn = true;
	}
}

bool Chunk::AddBlock(int blockIndex, uint8_t blockType)
{
	if (m_state != ChunkState::ACTIVE)
		return false;

	m_blocks[blockIndex].m_blockType = blockType;
	BlockDefinition* blockDef = BlockDefinition::GetBlockDefinitionFromIndex(blockType);
	m_blocks[blockIndex].SetIsSolid(blockDef->m_isSolid);
	m_blocks[blockIndex].SetIsFullOpaque(blockDef->m_isOpaque);
	m_blocks[blockIndex].SetIsVisible(blockDef->m_isVisible);
	m_blocks[blockIndex].SetIsLiquid(blockDef->m_isLiquid);
	m_blocks[blockIndex].SetIsLiquidSource(blockDef->m_isLiquid);

	BlockIterator iter(this, blockIndex);
	Block* block = iter.GetBlock();
	if (block)
	{
		float timeAdded = (float)GetCurrentTimeSeconds();
		const uint8_t WATER_TYPE = BlockDefinition::GetBlockDefinitionIndexFromName("Water");
		if (block->IsLiquid())
		{
			const float FLOW_SPEED = block->m_blockType == WATER_TYPE ? WATER_FLOW_DELAY : LAVA_FLOW_DELAY;
			block->SetFlowValue(LIQUID_INFLUENCE);
			m_world->MarkLiquidDirty(iter);

			// Add this: mark neighbors
			BlockIterator n;

			n = iter.GetIterator(BlockDirection::DOWN);
			Block* nBlock = n.GetBlock();
			if (nBlock && nBlock->IsAir())
			{
				m_world->MarkLiquidDirty(n, timeAdded + FLOW_SPEED);

			}

			else
			{
				n = iter.GetIterator(BlockDirection::NORTH);
				m_world->MarkLiquidDirty(n, timeAdded + FLOW_SPEED);

				n = iter.GetIterator(BlockDirection::SOUTH);
				m_world->MarkLiquidDirty(n, timeAdded + FLOW_SPEED);

				n = iter.GetIterator(BlockDirection::EAST);
				m_world->MarkLiquidDirty(n, timeAdded + FLOW_SPEED);

				n = iter.GetIterator(BlockDirection::WEST);
				m_world->MarkLiquidDirty(n, timeAdded + FLOW_SPEED);
			}
		}

		else if (block->IsSolid())
		{
			block->SetIsLiquidSource(false);
			block->SetFlowValue(0);

			// 1. Spread downward fully
			BlockIterator d = iter.GetIterator(BlockDirection::DOWN);;
			while (true)
			{
				Block* db = d.GetBlock();
				if (!db || !db->IsLiquid())
					break;

				const float FLOW_SPEED = db->m_blockType == WATER_TYPE ? WATER_FLOW_DELAY : LAVA_FLOW_DELAY;

				timeAdded += FLOW_SPEED;
				m_world->MarkLiquidDirty(d, timeAdded);

				d = d.GetIterator(BlockDirection::DOWN);
				if (d.GetBlock() == nullptr)
					break;
			}

			// 2. Once we hit ground, spread outward
			auto markIfWater = [&](BlockDirection dir)
				{
					BlockIterator n = iter.GetIterator(dir);
					Block* nb = n.GetBlock();
					if (nb && nb->IsLiquid())
					{
						const float FLOW_SPEED = nb->m_blockType == WATER_TYPE ? WATER_FLOW_DELAY : LAVA_FLOW_DELAY;
						m_world->MarkLiquidDirty(n, timeAdded + FLOW_SPEED);
					}
				};

			markIfWater(BlockDirection::NORTH);
			markIfWater(BlockDirection::SOUTH);
			markIfWater(BlockDirection::EAST);
			markIfWater(BlockDirection::WEST);
		}
	}
	

	// Lighting logic unchanged
	m_world->MarkLightingDirty(iter);
	bool wasBlockSky = m_blocks[blockIndex].IsSky();
	if (wasBlockSky)
	{
		m_blocks[blockIndex].SetIsSky(false);
		iter = iter.GetIterator(BlockDirection::DOWN);
		block = iter.GetBlock();
		while (block && !block->IsFullOpaque())
		{
			m_world->MarkLightingDirty(iter);
			block->SetIsSky(false);
			iter = iter.GetIterator(BlockDirection::DOWN);
			block = iter.GetBlock();
		}
	}


	m_needsSaving = true;
	SetDirty();
	return true;
}

bool Chunk::RemoveBlock(int blockIndex)
{
	if (m_state != ChunkState::ACTIVE)
		return false;

	m_blocks[blockIndex].m_blockType = 0;
	m_blocks[blockIndex].SetIsSolid(false);
	m_blocks[blockIndex].SetIsFullOpaque(false);
	m_blocks[blockIndex].SetIsVisible(false);
	m_blocks[blockIndex].SetIsLiquid(false);
	m_blocks[blockIndex].SetIsLiquidSource(false);

	BlockIterator iter(this, blockIndex);

	m_world->MarkLightingDirty(iter);

	// sky logic unchanged
	BlockIterator upIter = iter.GetIterator(BlockDirection::UP);
	Block* upBlock = upIter.GetBlock();
	if (upBlock)
	{
		BlockDefinition* upDef = BlockDefinition::GetBlockDefinitionFromIndex(upBlock->m_blockType);
		if (upDef->IsWildGrass())
		{
			upBlock->m_blockType = 0;
		}

		if (upBlock->IsSky())
		{
			Block* block = iter.GetBlock();
			while (block && !block->IsFullOpaque())
			{
				block->SetIsSky(true);
				m_world->MarkLightingDirty(iter);
				iter = iter.GetIterator(BlockDirection::DOWN);
				block = iter.GetBlock();
			}
		}
	}


	iter = BlockIterator(this, blockIndex);
	m_world->MarkLiquidDirty(iter, (float)GetCurrentTimeSeconds());

	m_needsSaving = true;
	SetDirty();
	return true;
}

void Chunk::SetDirty()
{
	if(m_isDirty)
		return;

	m_world->m_dirtyChunks.insert({m_chunkCoords, this});
	m_isDirty = true;
	m_timeDirtied = GetCurrentTimeSeconds();
}

void Chunk::Regenerate()
{
	delete m_vbo;
	m_vbo = nullptr;
	delete m_ibo;
	m_ibo = nullptr;

	delete m_waterVbo;
	m_waterVbo = nullptr;
	delete m_waterIbo;
	m_waterIbo = nullptr;

	CreateTerrainMesh();
	m_isDirty = false;
}

//Helpers
//------------------------------------------------------------------------------------------------

bool Chunk::IsChunkInFrustum(Frustum frustum) const
{
	return IsSphereInViewFrustum(m_chunkBounds.GetCenterPos(), m_frustrumBoundsRadius, frustum);
}

Vec2 Chunk::GetCenterXYCoords() const
{
	return m_chunkBounds.GetCenterPos().GetXY();
}

int Chunk::GetBlockIndexFromLocalCoords(IntVec3 const& blockCoords) const
{
	return blockCoords.x | ((blockCoords.y << CHUNK_BITS_X) | (blockCoords.z << (CHUNK_BITS_X + CHUNK_BITS_Y)));
}

int Chunk::GetBlockIndexFromLocalCoords(Vec3 const& blockCoords) const
{
	return (int)blockCoords.x | ((int)blockCoords.y << CHUNK_BITS_X) | ((int)blockCoords.z << (CHUNK_BITS_X + CHUNK_BITS_Y));
}

int Chunk::GetBlockIndexFromGlobalCoords(Vec3 const& coords) const
{
	if(!m_chunkBounds.IsPointOnOrInside(coords))
		return -1;

	IntVec3 globalCoords(coords);
	IntVec3 localCoords = globalCoords - m_chunkBounds.m_mins;
	if(localCoords.x < CHUNK_SIZE_X && localCoords.y < CHUNK_SIZE_Y && localCoords.z < CHUNK_SIZE_Z)
		return GetBlockIndexFromLocalCoords(localCoords);

	return -1;

}

int Chunk::GetBlockIndexFromGlobalCoords(IntVec3 const& coords) const
{
	if (!m_chunkBounds.IsPointOnOrInside(Vec3(coords)))
		return -1;

	IntVec3 localCoords = coords - IntVec3(m_chunkBounds.m_mins);
	if (localCoords.x < CHUNK_SIZE_X && localCoords.y < CHUNK_SIZE_Y && localCoords.z < CHUNK_SIZE_Z)
		return GetBlockIndexFromLocalCoords(localCoords);

	return -1;
}

IntVec3 Chunk::GetBlockLocalCoordsFromIndex(int index) const
{
	IntVec3 localCoords;
	localCoords.x = index & CHUNK_MASK_X;
	localCoords.y = (index >> CHUNK_BITS_X) & (CHUNK_SIZE_Y - 1);
	localCoords.z = (index >> (CHUNK_BITS_X + CHUNK_BITS_Y)) & (CHUNK_SIZE_Z - 1);
	return localCoords;
}

IntVec3 Chunk::GetBlockGlobalCoordsFromIndex(int index) const
{
	IntVec3 localCoords = GetBlockLocalCoordsFromIndex(index);
	IntVec3 chunkMins = IntVec3(RoundToNearestInt(m_chunkBounds.m_mins.x), RoundToNearestInt(m_chunkBounds.m_mins.y), RoundToNearestInt(m_chunkBounds.m_mins.z));
	return localCoords + chunkMins;
}

bool Chunk::AreLocalCoordsInChunk(IntVec3 const& coords) const
{
	return (coords.x >= 0 && coords.x < CHUNK_SIZE_X) && (coords.y >= 0 && coords.y < CHUNK_SIZE_Y) && (coords.z >= 0 && coords.z < CHUNK_SIZE_Z);
}

bool Chunk::AreGlobalCoordsInChunk(IntVec3 const& coords) const
{
	IntVec3 chunkMins = IntVec3(RoundToNearestInt(m_chunkBounds.m_mins.x), RoundToNearestInt(m_chunkBounds.m_mins.y), RoundToNearestInt(m_chunkBounds.m_mins.z));
	IntVec3 localCoords = coords - chunkMins;
	return AreLocalCoordsInChunk(localCoords);
}

bool Chunk::ShouldRenderFace(BlockDefinition const* currentDef, Block const& block, BlockIterator const& neighborIterator, bool& out_faceIsTouchingWater, float& out_neighborFlowFrac, bool testFlowValue, bool isUpFace) const
{
	out_neighborFlowFrac = 1.0f;
	Block* neighbor = neighborIterator.GetBlock();
	if (neighbor == nullptr)
	{
		out_faceIsTouchingWater = false;
		return true;
	}

	if(isUpFace && block.IsLiquid() && block.GetFlowValue() < LIQUID_INFLUENCE && neighbor->IsFullOpaque())
		return true;

	uint8_t currentType = block.m_blockType;
	uint8_t neighborType = neighbor->m_blockType;
	BlockDefinition* neighborDef = BlockDefinition::GetBlockDefinitionFromIndex(neighborType);

	bool neighborIsWater = neighborDef->IsWater();
	if (neighborIsWater)
	{
		out_faceIsTouchingWater = true;
	}


	if (currentDef->IsLeaves() || neighborDef->IsLeaves())
		return true;

	bool neighborIsLiquid = neighborDef->m_isLiquid;

	if (block.IsLiquid())
	{
		if (neighborIsLiquid && testFlowValue)
		{
			uint8_t neighborFlow = neighbor->GetFlowValue();
			uint8_t flow = block.GetFlowValue();
			if (neighborFlow < flow)
			{
				out_neighborFlowFrac = GetClampedFractionWithinRange((float)neighborFlow, 0.75, (float)LIQUID_INFLUENCE);
				return true;
			}

			else
				return false;
		}

		else 
			return (!neighborDef->m_isOpaque && neighborType != currentType);
	}

	if (neighborIsLiquid)
	{
		if(!neighborDef->m_isOpaque)
			return true;

		if(isUpFace)
			return false;

		uint8_t neighborFlow = neighbor->GetFlowValue();
		if(neighborFlow >= LIQUID_INFLUENCE)
			return false;

		return true;
	}

	if (currentDef->IsCactus())
	{
		// Only hide cactus faces when touching another cactus block
		return (neighborType != currentType);
	}

	if (neighborDef->IsCactus())
		return true;

	return (!neighborDef->m_isOpaque);
}

void Chunk::GetMaxIndoorAndOutdoorLightFromNeighbors(uint8_t& out_indoorLight, uint8_t& out_outdoorLight, BlockIterator const& iter)
{
	auto getMaxLight = [&](BlockDirection direction)
		{
			BlockIterator n = iter.GetIterator(direction);
			Block* nb = n.GetBlock();
			if(!nb)
				return;

			out_outdoorLight = GetMax(out_outdoorLight, nb->GetOutdoorLight());
			out_indoorLight = GetMax(out_indoorLight, nb->GetIndoorLight());
		};

	getMaxLight(BlockDirection::NORTH);
	getMaxLight(BlockDirection::SOUTH);
	getMaxLight(BlockDirection::EAST);
	getMaxLight(BlockDirection::WEST);

}














