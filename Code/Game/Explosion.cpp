#include "Game/Explosion.hpp"
#include "Game/Game.hpp"
#include "Engine/Core/VertexUtils.hpp"

Explosion::Explosion(Map* map, Vec2 position, float size, float duration, SpriteAnimDefinition* animDef)
	: m_map(map)
	, m_position(position)
	, m_size(size)
	, m_duration(duration)
	, m_animDef(animDef)
{
}


void Explosion::Update(float deltaSeconds)
{
	m_age += deltaSeconds;
}

void Explosion::Render() const
{
	AABB2 explosionBounds;
	explosionBounds.SetCenter(m_position);
	explosionBounds.SetDimensions(Vec2(m_size, m_size));

	const SpriteDefinition& spriteDef = m_animDef->GetSpriteDefAtTime(m_age);
	Vec2 uvAtMins, uvAtMaxs;
	spriteDef.GetUVs(uvAtMins, uvAtMaxs);

	// Generate vertices
	std::vector<Vertex_PCU> explosionVerts;
	AddVertsForAABB2D(explosionVerts, explosionBounds, Rgba8(255, 255, 255, 255), uvAtMins, uvAtMaxs);

	// Bind texture, draw, and unbind
	g_theRenderer->BindTexture(m_map->m_exploAnimTexture);
	g_theRenderer->DrawVertexArray(explosionVerts);
	g_theRenderer->BindTexture(nullptr);
}

bool Explosion::IsFinished() const
{
	return m_age >= m_duration;
}

