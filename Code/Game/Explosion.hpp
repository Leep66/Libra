#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Game/Map.hpp"

class Explosion {
public:
	Explosion(Map* map, Vec2 position, float size, float duration, SpriteAnimDefinition* animDef);

	void Update(float deltaSeconds);
	void Render() const;
	bool IsFinished() const;

private:
	Map* m_map;
	Vec2 m_position;
	float m_size;
	float m_duration;
	float m_age = 0.0f;
	SpriteAnimDefinition* m_animDef;
	float m_orientationDegrees = 0.f;
};