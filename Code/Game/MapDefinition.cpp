#include "Game/MapDefinition.hpp"
#include "Engine/Core/EngineCommon.hpp"

std::vector<MapDefinition> MapDefinition::s_mapDefinitions;




void MapDefinition::SetMapDefinition(
	std::string name, IntVec2 dimensions, std::string fill, std::string edge, 
	std::string worm1Type, int worm1Count, int worm1MaxLength, 
	std::string worm2Type, int worm2Count, int worm2MaxLength, 
	std::string startFloor, std::string startBunker, 
	std::string endFloor, std::string endBunker, 
	int leoCount, int ariesCount, int scorpioCount, 
	int taurusCount, int capricornCount, int sagittariusCount)
{
	MapDefinition newDef;
	newDef.m_name = name;
	newDef.m_dimensions = dimensions;
	newDef.m_fillTileType = fill;
	newDef.m_edgeTileType = edge;
	newDef.m_worm1TileType = worm1Type;
	newDef.m_worm1Count = worm1Count;
	newDef.m_worm1MaxLength = worm1MaxLength;
	newDef.m_worm2TileType = worm2Type;
	newDef.m_worm2Count = worm2Count;
	newDef.m_worm2MaxLength = worm2MaxLength;
	newDef.m_startFloorTileType = startFloor;
	newDef.m_startBunkerTileType = startBunker;
	newDef.m_endFloorTileType = endFloor;
	newDef.m_endBunkerTileType = endBunker;
	newDef.m_leoCount = leoCount;
	newDef.m_ariesCount = ariesCount;
	newDef.m_scorpioCount = scorpioCount;
	newDef.m_taurusCount = taurusCount;
	newDef.m_capricornCount = capricornCount;
	newDef.m_sagittariusCount = sagittariusCount;

	s_mapDefinitions.push_back(newDef);
}

void MapDefinition::InitializeMapDefinitions()
{
	XmlDocument doc;
	XmlResult result = doc.LoadFile("Data/MapDef.xml");
	if (result != tinyxml2::XML_SUCCESS)
	{
		ERROR_AND_DIE("Failed to load Data/MapDef.xml");
	}

	XmlElement* root = doc.RootElement();
	if (root == nullptr)
	{
		ERROR_AND_DIE("MapDef.xml is missing a root element");
	}

	XmlElement const* mapDefElem = root->FirstChildElement("MapDefinition");
	while (mapDefElem)
	{
		std::string name = ParseXmlAttribute(*mapDefElem, "name", "Invalid Name");
		IntVec2 dimensions = ParseXmlAttribute(*mapDefElem, "dimensions", IntVec2(-1, -1));
		std::string fill = ParseXmlAttribute(*mapDefElem, "fillTileType", "Invalid");
		std::string edge = ParseXmlAttribute(*mapDefElem, "edgeTileType", "Invalid");
		std::string worm1Type = ParseXmlAttribute(*mapDefElem, "worm1TileType", "Invalid");
		int worm1Count = ParseXmlAttribute(*mapDefElem, "worm1Count", 0);
		int worm1MaxLength = ParseXmlAttribute(*mapDefElem, "worm1MaxLength", 0);
		std::string worm2Type = ParseXmlAttribute(*mapDefElem, "worm2TileType", "Invalid");
		int worm2Count = ParseXmlAttribute(*mapDefElem, "worm2Count", 0);
		int worm2MaxLength = ParseXmlAttribute(*mapDefElem, "worm2MaxLength", 0);
		std::string startFloor = ParseXmlAttribute(*mapDefElem, "startFloorTileType", "Invalid");
		std::string startBunker = ParseXmlAttribute(*mapDefElem, "startBunkerTileType", "Invalid");
		std::string endFloor = ParseXmlAttribute(*mapDefElem, "endFloorTileType", "Invalid");
		std::string endBunker = ParseXmlAttribute(*mapDefElem, "endBunkerTileType", "Invalid");
		int leoCount = ParseXmlAttribute(*mapDefElem, "leoCount", 0);
		int ariesCount = ParseXmlAttribute(*mapDefElem, "ariesCount", 0);
		int scorpioCount = ParseXmlAttribute(*mapDefElem, "scorpioCount", 0);
		int taurusCount = ParseXmlAttribute(*mapDefElem, "taurusCount", 0);
		int capricornCount = ParseXmlAttribute(*mapDefElem, "capricornCount", 0);
		int sagittariusCount = ParseXmlAttribute(*mapDefElem, "sagittariusCount", 0);

		SetMapDefinition(name, dimensions, fill, edge, worm1Type, worm1Count, worm1MaxLength,
			worm2Type, worm2Count, worm2MaxLength, startFloor, startBunker,
			endFloor, endBunker, leoCount, ariesCount, scorpioCount,
			taurusCount, capricornCount, sagittariusCount);

		mapDefElem = mapDefElem->NextSiblingElement("MapDefinition");
	}
}

MapDefinition const& MapDefinition::GetDefinition(int mapDefIndex)
{
	return s_mapDefinitions[mapDefIndex];
}

MapDefinition const& MapDefinition::GetDefinition(std::string mapName)
{
	for (MapDefinition const& def : s_mapDefinitions)
	{
		if (def.m_name == mapName)
		{
			return def;
		}
	}

	static MapDefinition invalidDef;
	return invalidDef;
}
