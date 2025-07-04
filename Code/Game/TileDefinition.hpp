#pragma once
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <vector>
#include <string>


struct TileDefinition
{
	std::string m_name = "Invalid Tile Name";
	AABB2 m_uvCoords = AABB2::ZERO_TO_ONE;
	Rgba8 m_tintColor;
	IntVec2 m_spriteCoords;
	bool m_isSolid = false;
	bool m_isWater = false;

	// bool m_isOpaque = false;
	//float m_flammability;
	int m_health = -1;
	std::string m_tileNameToChangeToWhenDestroyed = "Invalid Tile Name to Change";
	//SoundID m_soundWhenDrivingOver;

	static std::vector<TileDefinition> s_tileDefinitions;
	static void SetTileDefinition(std::string name, IntVec2 spriteCoords, Rgba8 tintColor, bool isSolid, bool isWater, int health, std::string tileNameToChangeToWhenDestroyed);
	static void InitializeTileDefinitions();

	static TileDefinition const& GetDefinition(std::string const& name);
};

