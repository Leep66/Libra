#pragma once
#include "Game/Game.hpp"
#include "Game/Entity.hpp"

class Bullet : public Entity
{
public:
	Bullet(EntityType type, EntityFaction faction);
	virtual ~Bullet();
	void Startup();
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

	virtual void DebugRender() const override;
	virtual void Die() override;

	void Bounce(RaycastResult2D result);


public:
	AABB2 m_baseLocalBounds = AABB2(-0.1f, -0.05f, 0.1f, 0.05f);
	Texture* m_BulletTexture = nullptr;
	bool m_isBounced = false;
	float m_duration = -99.f;
	bool m_haveDuration = false;
private:
	
};