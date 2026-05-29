#pragma once
#include "Game/Entity.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Rgba8.hpp"
#include <vector>

class Texture;
class Game;
class Prop : public Entity
{
public:
	explicit Prop(Game* owner);
	explicit Prop(Game* owner, std::vector<Vertex_PCU> const& vertexes, Vec3 const& position = Vec3::ZERO);
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
public:
	std::vector<Vertex_PCU> m_vertexes;
	Texture* m_texture = nullptr;
};

