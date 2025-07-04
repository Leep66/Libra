#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Game/TileDefinition.hpp"

struct Tile {
	IntVec2 m_tileCoords = IntVec2(-1, -1);
	const TileDefinition* m_tileDef = nullptr;
	int m_health = -1;
};