#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/TileHeatMap.hpp"

class Map;


enum EntityType
{
	ENTITY_TYPE_UNKNOWN = -1,

	ENTITY_TYPE_PLAYERTANK,
	ENTITY_TYPE_SCORPIO,
	ENTITY_TYPE_LEO,
	ENTITY_TYPE_ARIES,

	ENTITY_TYPE_PROJECTILE_BOLT,
	ENTITY_TYPE_PROJECTILE_BULLET,
	ENTITY_TYPE_PROJECTILE_SHELL,
	ENTITY_TYPE_PROJECTILE_FIRE,
	
	NUM_ENTITY_TYPES
};

enum EntityFaction
{
	ENTITY_FACTION_UNKNOWN = -1,

	ENTITY_FACTION_GOOD,
	ENTITY_FACTION_NEUTRAL,
	ENTITY_FACTION_EVIL,

	NUM_ENTITY_FACTION
	
};



class Entity
{
	friend class Map;

protected:
	Entity(EntityType type, EntityFaction faction);
	virtual ~Entity();

	virtual void Update(float deltaSeconds) = 0;
	virtual void Render() const = 0;

	virtual void DebugRender() const;
	virtual void Die();

	Vec2 GetForwardNormal(float orientationDegrees);


public:
	
	
	Map* m_map = nullptr;
	EntityType m_entityType = ENTITY_TYPE_UNKNOWN;
	EntityFaction m_entityFaction = ENTITY_FACTION_UNKNOWN;

	Vec2	m_position;
	float	m_physicalRadius = 0.4f;
	float	m_cosmeticRadius = 0.6f;
	float	m_orientationDegrees = 0.f;

	int m_health = 10;
	int m_maxHealth = 10;
	bool m_isDead = false;
	bool m_isGarbage = false;
	

	Vec2 m_velocity;
	
	Vec2 m_turretSize;
	bool m_doesPushEntities = true;
	bool m_isPushedByEntity = true;
	bool m_isPushedByWall = true;
	bool m_isHittedByBullet = true;
	bool m_canSwim = false;

	TileHeatMap* m_heatMap = nullptr;

public:
	void UpdateHeatMap();
	bool CanSwim() const { return m_canSwim; }
};