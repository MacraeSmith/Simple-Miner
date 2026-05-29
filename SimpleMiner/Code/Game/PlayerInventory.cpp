#include "Game/PlayerInventory.hpp"
#include "Game/Game.hpp"
#include "Game/World.hpp"
#include "Game/WorldDefinition.hpp"
#include "Game/BlockDefinition.hpp"
#include "Engine/Renderer/RendererDX11.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Core/VertexUtils.hpp"

void PlayerInventory::Render() const
{
	AABB2 screenBounds = g_game->GetScreenBounds();
	screenBounds.AddPadding(Vec2(-0.21f, -0.02f), Vec2(-0.21f, -0.87f));
	Verts verts;
	AddVertsForAABB2D(verts, screenBounds);

	g_renderer->BindTexture(g_game->m_currentWorld->m_worldDef->m_hudTexture);
	g_renderer->BindShader(nullptr);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_renderer->SetModelConstants();
	g_renderer->DrawVertexArray(verts);

	std::vector<AABB2> slices = screenBounds.GetVerticalSlicedBoxesLeftToRight(NUM_BLOCKS_IN_INVENTORY);

	verts.clear();
	for (int i = 0; i < NUM_BLOCKS_IN_INVENTORY; ++i)
	{
		BlockDefinition* blockDef = BlockDefinition::GetBlockDefinitionFromIndex(m_blocks[i]);
		IntVec2 spriteCoords = blockDef->m_iconSpriteCoords;
		AABB2 uvs = g_game->m_currentWorld->m_worldDef->m_blockIconSpriteSheet->GetSpriteUVs(spriteCoords);
		Rgba8 color = Rgba8::WHITE;
		AddVertsForAABB2D(verts, slices[i], color, uvs);
	}

	g_renderer->BindTexture(&g_game->m_currentWorld->m_worldDef->m_blockIconSpriteSheet->GetTexture());
	g_renderer->BindShader(nullptr);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_renderer->SetModelConstants();
	g_renderer->DrawVertexArray(verts);


	verts.clear();
	AddVertsForAABB2D(verts, slices[m_selectedBlockIndex]);
	g_renderer->BindTexture(g_game->m_currentWorld->m_worldDef->m_selectedBlockTexture);
	g_renderer->BindShader(nullptr);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_renderer->SetModelConstants();
	g_renderer->DrawVertexArray(verts);
}
