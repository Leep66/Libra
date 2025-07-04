#include "Game/TileDefinition.hpp"
#include "Game/Tile.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Core/EngineCommon.hpp"

#include <vector>

std::vector<TileDefinition> TileDefinition::s_tileDefinitions;
extern SpriteSheet* g_terrainSpriteSheet;


void TileDefinition::SetTileDefinition(std::string name, IntVec2 spriteCoords, Rgba8 tintColor, bool isSolid, bool isWater, int health, std::string tileNameToChangeToWhenDestroyed) {
	TileDefinition newDef;
	newDef.m_name = name;
	newDef.m_spriteCoords = spriteCoords;
	newDef.m_tintColor = tintColor;
	newDef.m_isSolid = isSolid;
	newDef.m_isWater = isWater;
	newDef.m_health = health;
	newDef.m_tileNameToChangeToWhenDestroyed = tileNameToChangeToWhenDestroyed;

	int spriteIndex = spriteCoords.x + (spriteCoords.y * 8);
	newDef.m_uvCoords = g_terrainSpriteSheet->GetSpriteUVs(spriteIndex);

	s_tileDefinitions.push_back(newDef);
}

void TileDefinition::InitializeTileDefinitions() {
	XmlDocument doc;
	XmlResult result = doc.LoadFile("Data/TileDef.xml");
	if (result != tinyxml2::XML_SUCCESS)
	{
		ERROR_AND_DIE("Failed to load Data/TileDef.xml");
	}

	XmlElement* root = doc.RootElement();
	if (root == nullptr)
	{
		ERROR_AND_DIE("TileDef.xml is missing a root element");
	}

	/*for (XmlElement* element = gameRoot->FirstChildElement("TileDefinition"); element; element = element->NextSiblingElement("TileDefinition")) {
		std::string name = ParseXmlAttribute(*element, "name", "");
		GUARANTEE_OR_DIE(!name.empty(), "TileDefinition missing 'name' attribute!");

		IntVec2 spriteCoords = ParseXmlAttribute(*element, "spriteCoords", IntVec2(-1, -1));

		Rgba8 tintColor = ParseXmlAttribute(*element, "tint", Rgba8(255, 255, 255, 255));
		bool isSolid = ParseXmlAttribute(*element, "isSolid", false);
		bool isWater = ParseXmlAttribute(*element, "isWater", false);

		SetTileDefinition(name, spriteCoords, tintColor, isSolid, isWater);
	}*/

	XmlElement* tileElem = root->FirstChildElement();
	while (tileElem)
	{
		std::string name = ParseXmlAttribute(*tileElem, "name", "");
		GUARANTEE_OR_DIE(!name.empty(), "TileDefinition missing 'name' attribute!");

		IntVec2 spriteCoords = ParseXmlAttribute(*tileElem, "spriteCoords", IntVec2(-1, -1));

		Rgba8 tintColor = ParseXmlAttribute(*tileElem, "tint", Rgba8(255, 255, 255, 255));
		bool isSolid = ParseXmlAttribute(*tileElem, "isSolid", false);
		bool isWater = ParseXmlAttribute(*tileElem, "isWater", false);
		int health = ParseXmlAttribute(*tileElem, "health", -1);
		std::string tileNameToChangeToWhenDestroyed = ParseXmlAttribute(*tileElem, "tileNameToChangeToWhenDestroyed", "InvalidName");

		SetTileDefinition(name, spriteCoords, tintColor, isSolid, isWater, health, tileNameToChangeToWhenDestroyed);

		tileElem = tileElem->NextSiblingElement();
	}
}

TileDefinition const& TileDefinition::GetDefinition(std::string const& name) {
	for (TileDefinition const& def : s_tileDefinitions) {
		if (def.m_name == name) {
			return def;
		}
	}
	static TileDefinition invalidDef;
	return invalidDef;
}



