#pragma once
#include "Game/Entity.hpp"
#include "Game/Game.hpp"

class Scorpio : public Entity
{
public:
	Scorpio(EntityType type, EntityFaction faction);
	virtual ~Scorpio();
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

	virtual void DebugRender() const override;

	virtual void Die() override;

public:
	AABB2 m_scorpioLocalBounds = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);
	AABB2 m_turretLocalBounds = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);

	Texture* m_scorpioBaseTexture = nullptr;
	Texture* m_scorpioTurretTexture = nullptr;

private:
	void UpdateTurret(float deltaSeconds);

	void RenderBase() const;
	void RenderTurret() const;
	void RenderLaser() const;
	void RenderHealthBar() const;

	void AddVertsForLaserTriangle(std::vector<Vertex_PCU>& verts, Vec2 const& startPos, Vec2 const& endPos, float thickness, Rgba8 const& startColor, Rgba8 const& endColor) const;

	bool IsPlayerVisible() const;


private:
	float m_turretTargetDegrees;

	Vec2 m_playerPos;

	Vec2 m_velocity;

	float m_fireCooldown = 0.3f;
	float m_fireTimer = 0.0f;
	bool m_fpSoundPlay = false;
};