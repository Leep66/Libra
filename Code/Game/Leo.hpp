#pragma once
#include "Game/Entity.hpp"
#include "Game/Game.hpp"

class Leo : public Entity
{
public:
	Leo(EntityType type, EntityFaction faction);
	virtual ~Leo();
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

	virtual void DebugRender() const override;
	virtual void Die() override;

public:
	AABB2 m_baseLocalBounds = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);
	Texture* m_leoBaseTexture = nullptr;

private:
	void ChooseNewTargetPosition();
	bool IsPlayerVisible();
	void UpdateOrientation(float deltaSeconds);
	void MoveTowardTarget(float deltaSeconds);
	void HandleFiring(float deltaSeconds);

	void GeneratePathToGoal();
	void FollowPath(float deltaSeconds);

	void RenderHealthBar() const;

private:
	Vec2 m_playerPos;
	Vec2 m_targetPosition = Vec2::ZERO;
	Vec2 m_goalPosition;
	std::vector<Vec2> m_pathPoints;
	Vec2 m_nextWaypoint;
	IntVec2 m_previousWaypoint;
	float m_targetOrientationDeg = 0.f;	
	float m_fireCooldown = 1.f;
	float m_fireTimer = 0.f;
	float m_timeAtCurrentTarget = 0.f;
	IntVec2 m_lastTile;
	bool m_fpSoundPlay = false;
};