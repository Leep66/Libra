#pragma once
#include "Game/Entity.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Game/Game.hpp"


class PlayerTank : public Entity
{
public:
	PlayerTank(EntityType type, EntityFaction faction);
	virtual ~PlayerTank();
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	
	virtual void DebugRender() const override;
	virtual void Die() override;

	void SetRotationSpeed(float speed);
	

public:
	AABB2 m_tankLocalBounds = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);
	AABB2 m_turretLocalBounds = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);
	
	Texture* m_tankTexture = nullptr;
	Texture* m_turretTexture = nullptr;
	float m_turretOrientationDegrees = 45.f;
private:
	void UpdateFromKeyboard(float deltaSeconds);
	void UpdateFromController(float deltaSeconds);

	Vec2 GetVelocity() const { return m_velocity; }

	void RenderTank() const;
	void RenderTurret() const;
	void RenderHealthBar() const;


private:
	float m_speed;
	float m_targetDegrees = 0.f;
	float m_transitionSpeed = 10.f;

	float m_fireBoltCoolDown = 0.1f;
	float m_fireBoltTimer = -0.1f;

	float m_fireFlameCoolDown = 0.005f;
	float m_fireFlameTimer = -0.1f;

	float m_turretTargetDegrees = 0.f;
	float m_turretOffsetDegrees = 0.f;

	bool m_isMoving = false;
	bool m_isAiming = false;
	float m_timelastFrame = 1.f / 60.f;
};