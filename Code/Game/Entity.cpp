#include "Entity.hpp"
#include "Game/Map.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/TileHeatMap.hpp"

Entity::Entity(EntityType type, EntityFaction faction)
	:m_entityType(type)
	,m_entityFaction(faction)
{
	m_health = 10;
	m_maxHealth = 10;
}

Entity::~Entity()
{
	delete m_heatMap;
	m_heatMap = nullptr;
	
}

void Entity::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}

void Entity::Render() const
{
}

void Entity::DebugRender() const
{
}

void Entity::Die()
{
}

Vec2 Entity::GetForwardNormal(float orientationDegrees)
{
	return Vec2(CosDegrees(orientationDegrees), SinDegrees(orientationDegrees));
}

void Entity::UpdateHeatMap()
{
	if (m_heatMap != nullptr)
	{
		m_map->PopulateDistanceField(*m_heatMap, IntVec2((int)m_position.x, (int)m_position.y), 999.f, true, true);
	}
}
