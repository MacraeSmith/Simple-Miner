#include "Game/LitTnt.hpp"
#include "Game/Game.hpp"
#include "Game/World.hpp"
#include "Game/BlockDefinition.hpp"
#include "Game/WorldDefinition.hpp"
#include "Game/BlockIterator.hpp"
#include "Engine/Renderer/RendererDX11.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

LitTnt::LitTnt(Game* owner)
	:Entity(owner)
	,m_timer(new Timer(g_rng->RollRandomFloatInRange(TNT_TIMER_RANGE.m_min, TNT_TIMER_RANGE.m_max), &Clock::GetSystemClock()))
{
	m_world = m_game->m_currentWorld;

	m_halfHeight = TNT_BOX_RADIUS;
	m_physicsBounds = AABB3(-TNT_BOX_RADIUS, -TNT_BOX_RADIUS, 0.f, TNT_BOX_RADIUS, TNT_BOX_RADIUS, TNT_BOX_RADIUS * 2.f);

}

LitTnt::~LitTnt()
{
	delete m_timer;
	m_timer = nullptr;
}

void LitTnt::Update()
{
	Entity::Update();
	if (m_timer->Tick())
	{
		Explode();
		m_isGarbage = true;
	}

	else
	{
		float t = sin(m_game->m_gameClock->GetTotalSeconds() * 15.f);
		t = RangeMap(t, -1.f, 1.f, 0.f, 1.f);
		m_color = Rgba8::ColorLerp(Rgba8::WHITE, Rgba8::BLACK, t);

	}
}

void LitTnt::Render() const
{
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_renderer->SetModelConstants(GetModelToWorldTransform(), m_color);
	g_renderer->BindShader(m_world->m_worldDef->m_chunkShader);
	g_renderer->BindTexture(&m_world->m_worldDef->m_blockSpriteSheet->GetTexture());
	g_renderer->DrawIndexedVertexBuffer(m_world->m_worldDef->m_tntVbo, m_world->m_worldDef->m_tntIbo, m_world->m_worldDef->m_tntIbo->GetNumIndexes());
}

void LitTnt::Explode()
{
	BlockIterator centerIter = m_world->GetBlockIteratorAtPosition(m_position);
	Block* centerBlock = centerIter.GetBlock();
	if(!centerBlock)
		return;

	if(centerBlock->IsLiquid())
		return;

	int radius = g_rng->RollRandomIntInRange(TNT_EXPLOSION_RADIUS_RANGE.m_min, TNT_EXPLOSION_RADIUS_RANGE.m_max);

	std::vector<BlockIterator> surroundingIters = centerIter.GetAllValidBlockIteratorsInRadius(radius);


	for (int i = 0; i < (int)surroundingIters.size(); ++i)
	{
		Block* nB = surroundingIters[i].GetBlock();
		if(!nB || nB->IsLiquid())
			continue;

		uint8_t blockType = nB->m_blockType;

		m_world->RemoveBlock(surroundingIters[i]);
		if (blockType == m_blockIndex) // both equal tnt
		{
			Vec3 pos = surroundingIters[i].GetPosition();
			m_game->SpawnTNT(pos + Vec3(0.5f, 0.5f, 0.5f));
		}
	}

	EventArgs args;
	args.SetValue("Center", m_position.GetAsText());
	args.SetValue("Radius", Stringf("%i", radius));
	args.SetValue("Force", Stringf("%f", TNT_EXPLOSION_FORCE));

	FireEvent("ExplosionFired", args);

	m_world->RemoveBlock(centerIter);

}
