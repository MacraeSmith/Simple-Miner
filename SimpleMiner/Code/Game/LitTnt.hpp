#pragma once
#include "Game/Entity.hpp"
class Timer;
class World;
class LitTnt : public Entity
{
public:
	explicit LitTnt(Game* owner);
	virtual ~LitTnt();

	virtual void Update() override;
	virtual void Render() const override;
	void Explode();

private:
	uint8_t m_blockIndex = 41;
	Timer* m_timer = nullptr;
	World* m_world = nullptr;
};

