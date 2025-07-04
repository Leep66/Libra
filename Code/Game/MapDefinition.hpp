#pragma once
#include <string>
#include <vector>
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/Rgba8.hpp"



struct MapDefinition
{
	std::string m_name = "Invalid Map Name";
	IntVec2 m_dimensions = IntVec2(-1, -1);
	std::string m_fillTileType = "Invalid Fill Tile Type";
	std::string m_edgeTileType = "Invalid Edge Tile Type";
	std::string m_worm1TileType = "Invalid Worm 1 Tile Type";
	int m_worm1Count = -1;
	int m_worm1MaxLength = -1;
	std::string m_worm2TileType = "Invalid Worm 2 Tile Type";
	int m_worm2Count = -1;
	int m_worm2MaxLength = -1;
	std::string m_startFloorTileType = "Invalid Start Floor";
	std::string m_startBunkerTileType = "Invalid Start Bunker";
	std::string m_endFloorTileType = "Invalid End Floor";
	std::string m_endBunkerTileType = "Invalid End Bunker";
	int m_leoCount = -1;
	int m_ariesCount = -1;
	int m_scorpioCount = -1;
	int m_taurusCount = -1;
	int m_capricornCount = -1;
	int m_sagittariusCount = -1;

	static std::vector<MapDefinition> s_mapDefinitions;
	static void SetMapDefinition(
		std::string name, IntVec2 dimensions,
		std::string fill, std::string edge,
		std::string worm1Type, int worm1Count, int worm1MaxLength,
		std::string worm2Type, int worm2Count, int worm2MaxLength,
		std::string startFloor, std::string startBunker,
		std::string endFloor, std::string endBunker,
		int leoCount, int ariesCount, int scorpioCount,
		int taurusCount, int capricornCount, int sagittariusCount);

	static void InitializeMapDefinitions();

	static MapDefinition const& GetDefinition(int mapDefIndex);
	static MapDefinition const& GetDefinition(std::string mapName);

};