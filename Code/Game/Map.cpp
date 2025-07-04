#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/Tile.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/PlayerTank.hpp"
#include "Game/Scorpio.hpp"
#include "Game/Leo.hpp"
#include "Game/Aries.hpp"
#include "Game/Bullet.hpp"
#include "Game/Explosion.hpp"

#include "Engine/Window/Window.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Input/NamedStrings.hpp"
#include "Engine/Core/TileHeatMap.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Math/FloatRange.hpp"

extern Renderer* g_theRenderer;
extern SpriteSheet* g_terrainSpriteSheet;
extern Window* g_theWindow;
extern NamedStrings g_gameConfigBlackboard;

Map::Map(Game* game, std::string name)
	:m_game(game)
{
	m_mapDef = &MapDefinition::GetDefinition(name);
	if (m_mapDef->m_name == "Invalid Map Name") {
		ERROR_AND_DIE("Invalid map definition name");
	}
	Startup();
}

Map::~Map()
{

	m_entitiesByType->clear();
	m_allEntities.clear();
	m_tiles.clear();
	m_bulletsByFaction->clear();
	m_agentsByFaction->clear();
	delete m_heatMap1;
	m_heatMap1 = nullptr;
	delete m_heatMap2;
	m_heatMap2 = nullptr;
	delete m_heatMap3;
	m_heatMap3 = nullptr;

	delete m_exploAnimSheet;
	m_exploAnimSheet = nullptr;

	delete m_exploAnimDef;
	m_exploAnimDef = nullptr;


}

void Map::Startup()
{
	m_exploAnimTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Explosion_5x5.png");
	m_exploAnimSheet = new SpriteSheet(*m_exploAnimTexture, IntVec2(5, 5));
	m_exploAnimDef = new SpriteAnimDefinition(*m_exploAnimSheet, 0, 24, 5.f, SpriteAnimPlaybackType::ONCE);

	InitializeTiles();
	CreateHeatMap();

	SpawnInitialNPCs();
	
	FindNextEntity();
}

void Map::Update(float deltaSeconds)
{
	UpdateCamera();
	if (!m_currentEntityLosing)
	{
		if (m_currentEntityIndex > (int)m_allEntities.size() - 1 || m_allEntities[m_currentEntityIndex] == nullptr)
		{
			int heatmapEntity = 0;
			for (Entity* entity : m_agentsByFaction[ENTITY_FACTION_EVIL])
			{
				if (entity)
				{
					if (entity->m_entityType != ENTITY_TYPE_SCORPIO)
					{
						heatmapEntity++;
					}
				}

			}
			if (heatmapEntity > 0)
			{
				FindNextEntity();

			}

		}
	}
	

	UpdateEntities(deltaSeconds);
	UpdateExplosions(deltaSeconds);


	PushEntitiesOutOfSolidOrWaterTiles();

	HandleCollideAgents();
	HandleBulletsVsTiles();

	CheckBulletsVsEntities();

	CheckHealthAndSetGarbages();
	DeleteGarbageEntities();
}

void Map::Render() const
{
	
	switch (m_game->m_f6Index % 5)
	{
	case 0:
		RenderTiles();
		break;
	case 1:
		RenderHeatMapMode1();
		break;
	case 2:
		RenderHeatMapMode2();
		break;
	case 3:
		RenderHeatMapMode3();
		break;
	case 4:
		RenderHeatMapMode4();
		break;
	default:
		break;
	}
	RenderEntities();
	RenderExplosions();
	

	if (m_game->m_f6Index % 5 == 4 && m_currentEntityIndex >= 0 && m_currentEntityIndex < static_cast<int>(m_allEntities.size()) - 1)
	{
		Entity* currentEntity = m_agentsByFaction[ENTITY_FACTION_EVIL][m_currentEntityIndex];
		if (currentEntity == nullptr) return;

		Vec2 entityPosition = currentEntity->m_position;

		Vec2 arrowStart = entityPosition + Vec2::MakeFromPolarDegrees(75.f, 100.f);
		Vec2 arrowEnd = entityPosition + Vec2::MakeFromPolarDegrees(75.f, 0.5f);

		std::vector<Vertex_PCU> arrowVerts;
		AddVertsForArrow2D(arrowVerts, arrowStart, arrowEnd, 0.3f, 0.1f, Rgba8::PINK);

		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(arrowVerts);
	}

	if (m_game->m_isPaused)
	{
		std::vector<Vertex_PCU> pauseScreenVerts;
		AABB2 pauseScreen = AABB2(0, 0, g_gameConfigBlackboard.GetValue("screenWidth", 1.f), g_gameConfigBlackboard.GetValue("screenHeight", 1.f));
		AddVertsForAABB2D(pauseScreenVerts, pauseScreen, Rgba8(0, 0, 0, 50));
		g_theRenderer->DrawVertexArray((int) pauseScreenVerts.size(), pauseScreenVerts.data());
		g_theRenderer->BindTexture(nullptr);
	}


	//RenderDebugRay();

}

void Map::DebugRender() const
{

	for (int entityType = 0; entityType < NUM_ENTITY_TYPES; ++entityType)
	{
		EntityList entitiesOfType = m_entitiesByType[entityType];
		for (int entityIndex = 0; entityIndex < (int)entitiesOfType.size(); ++entityIndex)
		{
			if (entitiesOfType[entityIndex])
			{
				entitiesOfType[entityIndex]->DebugRender();

			}
		}
	}
}

Entity* Map::GetPlayer() const
{
	return m_game->m_playerTank;
}

AABB2 Map::GetTileBounds(IntVec2 const& tileCoords) const
{
	Vec2 mins(static_cast<float>(tileCoords.x), static_cast<float>(tileCoords.y));
	Vec2 maxs = mins + Vec2(1.0f, 1.0f);  
	return AABB2(mins, maxs);
}

AABB2 Map::GetTileBounds(int tileIndex) const
{
	IntVec2 tileCoords = IntVec2(tileIndex % m_dimensions.x, tileIndex / m_dimensions.x);
	return GetTileBounds(tileCoords);
}

IntVec2 Map::GetTileCoordsForWorldPos(Vec2 const& worldPos) const
{
	int tileX = static_cast<int>(floorf(worldPos.x));
	int tileY = static_cast<int>(floorf(worldPos.y));
	return IntVec2(tileX, tileY);
}

Vec2 Map::GetTileCenterForCoords(IntVec2 const& tileCoords) const
{
	float centerX = static_cast<float>(tileCoords.x) + 0.5f;
	float centerY = static_cast<float>(tileCoords.y) + 0.5f;
	return Vec2(centerX, centerY);
}


bool Map::IsInBounds(IntVec2 const& tileCoords) const
{
	return	tileCoords.x >= 0 &&
			tileCoords.y >= 0 &&
			tileCoords.x < m_dimensions.x &&
			tileCoords.y < m_dimensions.y;
}

bool Map::IsTileBorder(IntVec2 const& tileCoords) const
{
	return tileCoords.x == 0 ||
		tileCoords.y == 0 ||
		tileCoords.x == m_dimensions.x - 1 ||
		tileCoords.y == m_dimensions.y - 1;
}

bool Map::IsTileSolid(IntVec2 const& tileCoords) const
{
	if (!IsInBounds(tileCoords))
	{
		return true; 
	}

	int tileIndex = GetTileIndexForCoords(tileCoords);
	if (tileIndex < 0 || tileIndex >= (int)m_tiles.size())
	{
		return false;
	}

	TileDefinition const* tileDef = m_tiles[tileIndex].m_tileDef;
	if (tileDef == nullptr)
	{
		return false;
	}

	return tileDef->m_isSolid;
}

bool Map::IsTileWater(IntVec2 const& tileCoords) const
{
	if (!IsInBounds(tileCoords)) {
		return false;
	}

	int tileIndex = GetTileIndexForCoords(tileCoords);
	TileDefinition const* tileDef = m_tiles[tileIndex].m_tileDef;
	if (tileDef == nullptr)
	{
		return false;
	}

	return tileDef->m_isWater;

}


bool Map::IsTileInNest(IntVec2 const& tileCoords) const
{
	const int NEST_1_MIN_X = 1;
	const int NEST_1_MAX_X = 6;
	const int NEST_1_MIN_Y = 1;
	const int NEST_1_MAX_Y = 6;

	const int NEST_2_MIN_X = m_dimensions.x - 8;
	const int NEST_2_MAX_X = m_dimensions.x - 2;
	const int NEST_2_MIN_Y = m_dimensions.y - 8;
	const int NEST_2_MAX_Y = m_dimensions.y - 2;

	if (tileCoords.x >= NEST_1_MIN_X && tileCoords.x <= NEST_1_MAX_X &&
		tileCoords.y >= NEST_1_MIN_Y && tileCoords.y <= NEST_1_MAX_Y)
	{
		return true;
	}

	if (tileCoords.x >= NEST_2_MIN_X && tileCoords.x <= NEST_2_MAX_X &&
		tileCoords.y >= NEST_2_MIN_Y && tileCoords.y <= NEST_2_MAX_Y)
	{
		return true;
	}

	return false;
}


bool Map::IsPosInSolid(Vec2 pos) const
{
	IntVec2 tileCoords = GetTileCoordsForWorldPos(pos);
	return IsTileSolid(tileCoords);
}

bool Map::isBullet(Entity* entity) const
{
	if (!entity)
	{
		return false;
	}

	switch (entity->m_entityType)
	{
	case ENTITY_TYPE_PROJECTILE_BOLT:
	case ENTITY_TYPE_PROJECTILE_BULLET:
	case ENTITY_TYPE_PROJECTILE_SHELL:
	case ENTITY_TYPE_PROJECTILE_FIRE:
		return true;

	case ENTITY_TYPE_PLAYERTANK:
	case ENTITY_TYPE_SCORPIO:
	case ENTITY_TYPE_LEO:
	case ENTITY_TYPE_ARIES:
		return false;

	default:
		ERROR_AND_DIE(Stringf("Unhandled entity type #%i", entity->m_entityType));
	}
}

bool Map::isAgent(Entity* entity) const
{
	if (!entity)
	{
		return false;
	}

	switch (entity->m_entityType)
	{
	case ENTITY_TYPE_PROJECTILE_BOLT:
	case ENTITY_TYPE_PROJECTILE_BULLET:
	case ENTITY_TYPE_PROJECTILE_SHELL:
	case ENTITY_TYPE_PROJECTILE_FIRE:
		return false;

	case ENTITY_TYPE_PLAYERTANK:
	case ENTITY_TYPE_SCORPIO:
	case ENTITY_TYPE_LEO:
	case ENTITY_TYPE_ARIES:
		return true;

	default:
		ERROR_AND_DIE(Stringf("Unhandled entity type #%i", entity->m_entityType));
	}
}


int Map::GetTileIndexForCoords(IntVec2 const& tileCoords) const
{
	if (tileCoords.x < 0 || tileCoords.x >= m_dimensions.x ||
		tileCoords.y < 0 || tileCoords.y >= m_dimensions.y) {
		return -1;
	}

	return tileCoords.y * m_dimensions.x + tileCoords.x;
}


void Map::UpdateEntities(float deltaSeconds)
{
	for (int entityType = 0; entityType < NUM_ENTITY_TYPES; ++entityType)
	{
		EntityList entitiesOfType = m_entitiesByType[entityType];
		for (int entityIndex = 0; entityIndex < (int)entitiesOfType.size(); ++entityIndex)
		{
			if (entitiesOfType[entityIndex])
			{
				entitiesOfType[entityIndex]->Update(deltaSeconds);
			}
		}
	}
}

RaycastResult2D Map::RaycastVsTiles(Ray2 const& ray) const {
	RaycastResult2D result;
	result.m_didImpact = false;

	Vec2 startPosition = ray.m_startPos;
	Vec2 forwardNormal = ray.m_fwdNormal.GetNormalized();
	float maxDistance = ray.m_maxLength;

	int tileX = static_cast<int>(floor(startPosition.x));
	int tileY = static_cast<int>(floor(startPosition.y));

	if (IsTileSolid(IntVec2(tileX, tileY))) {
		result.m_didImpact = true;
		result.m_impactPos = startPosition;
		result.m_impactNormal = Vec2(0.f, 0.f); 
		result.m_impactDist = 0.f;
		return result;
	}

	float fwdDistPerXCrossing = (forwardNormal.x == 0.f) ? FLT_MAX : 1.0f / fabsf(forwardNormal.x);
	float fwdDistPerYCrossing = (forwardNormal.y == 0.f) ? FLT_MAX : 1.0f / fabsf(forwardNormal.y);

	int tileStepDirectionX = (forwardNormal.x < 0) ? -1 : 1;
	int tileStepDirectionY = (forwardNormal.y < 0) ? -1 : 1;

	float xAtFirstXCrossing = tileX + (tileStepDirectionX + 1) / 2.0f;
	float xDistToFirstXCrossing = xAtFirstXCrossing - startPosition.x;
	float fwdDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwdDistPerXCrossing;

	float yAtFirstYCrossing = tileY + (tileStepDirectionY + 1) / 2.0f;
	float yDistToFirstYCrossing = yAtFirstYCrossing - startPosition.y;
	float fwdDistAtNextYCrossing = fabsf(yDistToFirstYCrossing) * fwdDistPerYCrossing;

	float currentFwdDist = 0.f;

	while (currentFwdDist <= maxDistance) {
		if (fwdDistAtNextXCrossing < fwdDistAtNextYCrossing) {
			if (fwdDistAtNextXCrossing > maxDistance) {
				break;
			}
			tileX += tileStepDirectionX;
			currentFwdDist = fwdDistAtNextXCrossing;
			fwdDistAtNextXCrossing += fwdDistPerXCrossing;

			if (IsTileSolid(IntVec2(tileX, tileY))) {
				result.m_didImpact = true;
				result.m_impactPos = startPosition + forwardNormal * currentFwdDist;
				result.m_impactNormal = Vec2( - (float) tileStepDirectionX, 0.f);
				result.m_impactDist = currentFwdDist;
				return result;
			}
		}
		else {
			if (fwdDistAtNextYCrossing > maxDistance) {
				break;
			}
			tileY += tileStepDirectionY;
			currentFwdDist = fwdDistAtNextYCrossing;
			fwdDistAtNextYCrossing += fwdDistPerYCrossing;

			if (IsTileSolid(IntVec2(tileX, tileY))) {
				result.m_didImpact = true;
				result.m_impactPos = startPosition + forwardNormal * currentFwdDist;
				result.m_impactNormal = Vec2(0.f, - (float) tileStepDirectionY);
				result.m_impactDist = currentFwdDist;
				return result;
			}
		}
	}

	result.m_didImpact = false;
	result.m_impactPos = startPosition + forwardNormal * maxDistance;
	result.m_impactNormal = Vec2(0.f, 0.f);
	result.m_impactDist = maxDistance;
	return result;
}




/*
RaycastResult2D Map::RaycastVsTiles(Ray2 const& ray) const
{
	RaycastResult2D result;
	constexpr int STEPS_PER_UNIT = 100;
	constexpr float DIST_PER_STEP = 1.f / static_cast<float>(STEPS_PER_UNIT);
	int numSteps = static_cast<int>(ray.m_maxLength * static_cast<float>(STEPS_PER_UNIT));

	IntVec2 prevTileCoords = GetTileCoordsForWorldPos(ray.m_startPos);
	for (int step = 0; step < numSteps; ++step)
	{
		float stepForwardDist = DIST_PER_STEP * static_cast<float>(step);
		Vec2 pos = ray.m_startPos + (ray.m_fwdNormal * stepForwardDist);
		IntVec2 currentTileCoords = GetTileCoordsForWorldPos(pos);

		if (IsPosInSolid(pos))
		{
			IntVec2 impactNormal = prevTileCoords - currentTileCoords;
			result.m_didImpact = true;
			result.m_impactPos = pos;
			result.m_impactDist = stepForwardDist;
			result.m_impactNormal = Vec2((float)impactNormal.x, (float)impactNormal.y);
			return result;

		}
		prevTileCoords = currentTileCoords;
	}

	result.m_impactDist = ray.m_maxLength;
	return result;
}*/

void Map::CreateHeatMap()
{
	m_heatMap1 = new TileHeatMap(m_dimensions, 999.f);
	PopulateDistanceField(*m_heatMap1, IntVec2(1, 1), 999.f, true, false);
	
	m_heatMap2 = new TileHeatMap(m_dimensions, 999.f);
	PopulateDistanceField(*m_heatMap2, IntVec2(1, 1), 999.f, false, true);

	m_heatMap3 = new TileHeatMap(m_dimensions, 999.f);
	PopulateDistanceField(*m_heatMap3, IntVec2(1, 1), 999.f, true, true);

}

void Map::PopulateDistanceField(TileHeatMap& out_distanceField, IntVec2 startCoords, float maxCost, bool treatWaterAsSolid, bool treatScorpioAsSolid)
{
	out_distanceField.SetValueForAllTiles(maxCost);
	int tileIndex = (int) GetClamped((float) startCoords.x + (float) m_dimensions.x * (float) startCoords.y, 0.f, (float) m_dimensions.x * (float) m_dimensions.y - 1.f);
	out_distanceField.SetValueAtIndex(tileIndex, 0.0f);
	float currentValue = 0.f;
	bool valueChanged = true;
	while (valueChanged)
	{
		valueChanged = false;

		for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
		{
			for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
			{
				int currentIndex = tileX + tileY * m_dimensions.x;
				float currentTileValue = out_distanceField.GetValueAtIndex(currentIndex);

				if (currentTileValue <= currentValue)
				{
					for (IntVec2 const& direction : { IntVec2(0, 1), IntVec2(0, -1), IntVec2(1, 0), IntVec2(-1, 0) })
					{
						IntVec2 neighborCoords = IntVec2(tileX, tileY) + direction;
						if (!IsInBounds(neighborCoords)) continue;
						if (treatWaterAsSolid && IsTileWater(neighborCoords)) continue;
						int neighborIndex = neighborCoords.x + neighborCoords.y * m_dimensions.x;
						float neighborValue = out_distanceField.GetValueAtIndex(neighborIndex);

						if (neighborValue > currentTileValue + 1.0f && !IsTileSolid(neighborCoords) && !HasScorpioOnTile(neighborCoords, treatScorpioAsSolid))
						{
							out_distanceField.SetValueAtIndex(neighborIndex, currentTileValue + 1.0f);
							valueChanged = true;
						}
					}
				}
			}
		}

		currentValue += 1.0f;
	}
}


void Map::RenderHeatMapMode1() const
{
	std::vector<Vertex_PCU> heatVerts;
	heatVerts.reserve(3 * 2 * m_dimensions.x * m_dimensions.y);
	m_heatMap1->AddHeatVertsForDebugDraw(
		heatVerts,
		AABB2(0.f, 0.f, static_cast<float>(m_dimensions.x), static_cast<float>(m_dimensions.y)),
		FloatRange(0.f, static_cast<float>(m_dimensions.x) + static_cast<float>(m_dimensions.y)),
		Rgba8(0, 0, 0),
		Rgba8(255, 255, 255),
		999.f,
		Rgba8(0, 0, 255)
	);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(heatVerts);
}

void Map::RenderHeatMapMode2() const
{
	std::vector<Vertex_PCU> heatVerts;
	heatVerts.reserve(3 * 2 * m_dimensions.x * m_dimensions.y);
	m_heatMap2->AddSolidVertsForDebugDraw(heatVerts, AABB2(0.f, 0.f, (float)m_dimensions.x, (float)m_dimensions.y), Rgba8(0, 0, 0), 999.f, Rgba8(255, 255, 255));
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(heatVerts);
}

void Map::RenderHeatMapMode3() const
{
	std::vector<Vertex_PCU> heatVerts;
	heatVerts.reserve(3 * 2 * m_dimensions.x * m_dimensions.y);
	m_heatMap3->AddSolidVertsForDebugDraw(heatVerts, AABB2(0.f, 0.f, (float)m_dimensions.x, (float)m_dimensions.y), Rgba8(0, 0, 0), 999.f, Rgba8(255, 255, 255));
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(heatVerts);
}

void Map::RenderHeatMapMode4() const
{
	if (m_currentEntityIndex >= 0 && m_currentEntityIndex < static_cast<int>(m_agentsByFaction[ENTITY_FACTION_EVIL].size()))
	{
		Entity* currentEntity = m_agentsByFaction[ENTITY_FACTION_EVIL][m_currentEntityIndex];
		if (currentEntity == nullptr || currentEntity->m_heatMap == nullptr)
		{
			RenderHeatMapMode1();
			return;
		}

		IntVec2 startCoords = IntVec2(static_cast<int>(currentEntity->m_position.x), static_cast<int>(currentEntity->m_position.y));

		float maxValue = currentEntity->m_heatMap->GetMaxValue(999.f);

		std::vector<Vertex_PCU> heatVerts;
		heatVerts.reserve(3 * 2 * m_dimensions.x * m_dimensions.y);
		currentEntity->m_heatMap->AddHeatVertsForDebugDraw(
			heatVerts,
			AABB2(0.f, 0.f, static_cast<float>(m_dimensions.x), static_cast<float>(m_dimensions.y)),
			FloatRange(0.f, maxValue),
			Rgba8(0, 0, 0),
			Rgba8(255, 255, 255),
			999.f,
			Rgba8(0, 0, 255)
		);

		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(heatVerts);
	}
	else
	{
		RenderHeatMapMode1();
	}
}


void Map::UpdateCamera()
{
	if (m_game->m_isF4Active)
	{
		float mapWidth = static_cast<float>(m_dimensions.x);
		float mapHeight = static_cast<float>(m_dimensions.y);
		float mapAspectRatio = mapWidth / mapHeight;

		float cameraAspectRatio = 2.f;

		float viewWidth, viewHeight;

		if (cameraAspectRatio > mapAspectRatio)
		{
			viewHeight = mapHeight;
			viewWidth = viewHeight * cameraAspectRatio;
		}
		else
		{
			viewWidth = mapWidth;
			viewHeight = viewWidth / cameraAspectRatio;
		}

		m_game->m_worldCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(viewWidth, viewHeight));
		return;
	}

	float xOffset = 8.f;
	float yOffset = 4.f;

	Vec2 worldCamMinsA(m_game->m_playerTank->m_position.x - xOffset, m_game->m_playerTank->m_position.y - yOffset);
	Vec2 worldCamMaxsA(m_game->m_playerTank->m_position.x + xOffset, m_game->m_playerTank->m_position.y + yOffset);

	if (worldCamMinsA.x < 0.f)
	{
		worldCamMinsA.x = 0.f;
		worldCamMaxsA.x = xOffset * 2;
	}
	else if (worldCamMaxsA.x > m_dimensions.x)
	{
		worldCamMinsA.x = (float) m_dimensions.x - (xOffset * 2);
		worldCamMaxsA.x = (float) m_dimensions.x;
	}

	if (worldCamMinsA.y < 0.f)
	{
		worldCamMinsA.y = 0.f;
		worldCamMaxsA.y = yOffset * 2;
	}
	else if (worldCamMaxsA.y > m_dimensions.y)
	{
		worldCamMinsA.y = (float) m_dimensions.y - (yOffset * 2);
		worldCamMaxsA.y = (float) m_dimensions.y;
	}


	m_game->m_worldCamera.SetOrthographicView(worldCamMinsA, worldCamMaxsA);
}

void Map::InitializeTiles()
{
	m_dimensions = m_mapDef->m_dimensions;
	int numTiles = m_dimensions.x * m_dimensions.y;
	m_tiles.resize(numTiles);

	for (int tileY = 0; tileY < m_dimensions.y; ++tileY) {
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX) {
			int tileIndex = tileX + (tileY * m_dimensions.x);
			Tile& tile = m_tiles[tileIndex];
			tile.m_tileCoords = IntVec2(tileX, tileY);

			tile.m_tileDef = &TileDefinition::GetDefinition(m_mapDef->m_fillTileType);
			

			if (tileX == 0 || tileY == 0 || tileX == m_dimensions.x - 1 || tileY == m_dimensions.y - 1) {
				tile.m_tileDef = &TileDefinition::GetDefinition(m_mapDef->m_edgeTileType);
			}
		}
	}

	PlaceWorms();


	SetTileRegion(IntVec2(1, 1), IntVec2(5, 5), m_mapDef->m_startFloorTileType);
	m_tiles[2 + 4 * m_dimensions.x].m_tileDef = &TileDefinition::GetDefinition(m_mapDef->m_startBunkerTileType);
	m_tiles[3 + 4 * m_dimensions.x].m_tileDef = &TileDefinition::GetDefinition(m_mapDef->m_startBunkerTileType);
	m_tiles[4 + 4 * m_dimensions.x].m_tileDef = &TileDefinition::GetDefinition(m_mapDef->m_startBunkerTileType);
	m_tiles[4 + 3 * m_dimensions.x].m_tileDef = &TileDefinition::GetDefinition(m_mapDef->m_startBunkerTileType);
	m_tiles[4 + 2 * m_dimensions.x].m_tileDef = &TileDefinition::GetDefinition(m_mapDef->m_startBunkerTileType);
	m_tiles[1 + 1 * m_dimensions.x].m_tileDef = &TileDefinition::GetDefinition("Start");

	int startX = m_dimensions.x - 7;
	int startY = m_dimensions.y - 7;
	SetTileRegion(IntVec2(startX, startY), IntVec2(startX + 5, startY + 5), m_mapDef->m_endFloorTileType);

	for (int offset = 0; offset < 4; ++offset)
	{
		m_tiles[(startX + 1) + offset + (startY + 1) * m_dimensions.x].m_tileDef = &TileDefinition::GetDefinition(m_mapDef->m_startBunkerTileType);
		m_tiles[(startX + 1) + ((startY + 1) + offset) * m_dimensions.x].m_tileDef = &TileDefinition::GetDefinition(m_mapDef->m_startBunkerTileType);
	}

	int endPortalX = m_dimensions.x - 2;
	int endPortalY = m_dimensions.y - 2;
	m_tiles[endPortalX + endPortalY * m_dimensions.x].m_tileDef = &TileDefinition::GetDefinition("End");

	m_endPos = Vec2(endPortalX + 0.5f, endPortalY + 0.5f);

	for (Tile& tile : m_tiles)
	{
		tile.m_health = tile.m_tileDef->m_health;
	}
}

void Map::SetTileRegion(IntVec2 const& minCoords, IntVec2 const& maxCoords, std::string const& tileType)
{
	for (int y = minCoords.y; y <= maxCoords.y; ++y) {
		for (int x = minCoords.x; x <= maxCoords.x; ++x) {
			int tileIndex = x + y * m_dimensions.x;
			m_tiles[tileIndex].m_tileDef = &TileDefinition::GetDefinition(tileType);
		}
	}
}

void Map::PlaceWorms()
{
	PlaceSpecificWorm(m_mapDef->m_worm1TileType, m_mapDef->m_worm1Count, m_mapDef->m_worm1MaxLength);
	PlaceSpecificWorm(m_mapDef->m_worm2TileType, m_mapDef->m_worm2Count, m_mapDef->m_worm2MaxLength);
}

void Map::PlaceSpecificWormRecursive(IntVec2 currentCoords, std::string const& tileType, int wormLength, int currentLength)
{
	if (currentLength >= wormLength) {
		return;
	}

	if (IsInBounds(currentCoords) && !IsTileInNest(currentCoords) && !IsTileBorder(currentCoords)) {
		int tileIndex = currentCoords.x + currentCoords.y * m_dimensions.x;
		m_tiles[tileIndex].m_tileDef = &TileDefinition::GetDefinition(tileType);
	}
	else {
		return;
	}

	IntVec2 direction = GetRandomDirection();

	PlaceSpecificWormRecursive(currentCoords + direction, tileType, wormLength, currentLength + 1);
}

void Map::PlaceSpecificWorm(std::string const& tileType, int numWorms, int maxLength)
{
	for (int i = 0; i < numWorms; ++i) {
		IntVec2 startCoords = GetValidWormStartPosition();

		int wormLength = m_game->m_rng->RollRandomIntInRange(1, maxLength);

		PlaceSpecificWormRecursive(startCoords, tileType, wormLength, 0);
	}
}



IntVec2 Map::GetRandomDirection()
{
	int direction = m_game->m_rng->RollRandomIntInRange(0, 3);
	IntVec2 result;
	switch (direction)
	{
	case 0:
		result = IntVec2(1, 0);
		break;
	case 1:
		result = IntVec2(0, 1);
		break;
	case 2:
		result = IntVec2(-1, 0);
		break;
	case 3:
		result = IntVec2(0, -1);
		break;
	default:
		break;
	}

	return result;
}



void Map::SpawnInitialNPCs()
{
	int numScorpios = m_mapDef->m_scorpioCount;
	int numLeos = m_mapDef->m_leoCount;
	int numAries = m_mapDef->m_ariesCount;

	for (int i = 0; i < numScorpios; ++i)
	{
		Vec2 spawnPosition = GetValidSpawnPosition();
		float randomAngle = m_game->m_rng->RollRandomFloatInRange(0.f, 360.f);
		SpawnNewEntity(ENTITY_TYPE_SCORPIO, ENTITY_FACTION_EVIL, spawnPosition, randomAngle);
	}

	for (int i = 0; i < numLeos; ++i)
	{
		Vec2 spawnPosition = GetValidSpawnPosition();
		float randomAngle = m_game->m_rng->RollRandomFloatInRange(0.f, 360.f);
		SpawnNewEntity(ENTITY_TYPE_LEO, ENTITY_FACTION_EVIL, spawnPosition, randomAngle);
	}

	for (int i = 0; i < numAries; ++i)
	{
		Vec2 spawnPosition = GetValidSpawnPosition();
		float randomAngle = m_game->m_rng->RollRandomFloatInRange(0.f, 360.f);
		SpawnNewEntity(ENTITY_TYPE_ARIES, ENTITY_FACTION_EVIL, spawnPosition, randomAngle);
	}
}

Vec2 Map::GetValidSpawnPosition()
{
	IntVec2 position;
	bool validPositionFound = false;

	while (!validPositionFound)
	{
		int x = m_game->m_rng->RollRandomIntInRange(0, m_dimensions.x - 1);
		int y = m_game->m_rng->RollRandomIntInRange(0, m_dimensions.y - 1);
		position = IntVec2(x, y);

		if (!IsTileSolid(position) && !IsTileInNest(position) && !IsTileWater(position))
		{
			validPositionFound = true;
		}
	}
	Vec2 returnPos = Vec2(position.x + 0.5f, position.y + 0.5f);
	return returnPos;
}

IntVec2 Map::GetValidWormStartPosition()
{
	IntVec2 position;
	bool validPositionFound = false;

	while (!validPositionFound)
	{
		int x = m_game->m_rng->RollRandomIntInRange(0, m_dimensions.x - 1);
		int y = m_game->m_rng->RollRandomIntInRange(0, m_dimensions.y - 1);
		position = IntVec2(x, y);

		if (!IsTileBorder(position))
		{
			validPositionFound = true;
		}
	}
	return position;
}

IntVec2 Map::GetValidPosition(TileHeatMap const& heatmap) const
{
	IntVec2 position;
	bool validPositionFound = false;

	while (!validPositionFound)
	{
		int x = m_game->m_rng->RollRandomIntInRange(0, m_dimensions.x - 1);
		int y = m_game->m_rng->RollRandomIntInRange(0, m_dimensions.y - 1);
		position = IntVec2(x, y);

		int tileIndex = x + y * m_dimensions.x;
		float heatmapValue = heatmap.GetValueAtIndex(tileIndex);

		if (!IsTileSolid(position) && !IsTileBorder(position) && !IsTileWater(position) && heatmapValue != 999.0f)
		{
			validPositionFound = true;
		}
	}
	return position;
}


void Map::RenderTiles() const
{
	std::vector<Vertex_PCU> tileVerts;

	int numTiles = m_dimensions.x * m_dimensions.y;
	int numTris = numTiles * 2;
	int numVerts = numTris * 3;
	tileVerts.reserve(numVerts);

	for (int tileIndex = 0; tileIndex < (int)m_tiles.size(); ++tileIndex)
	{
		AddVertsForTile(tileVerts, tileIndex);
	}

	g_theRenderer->DrawVertexArray(tileVerts);
	g_theRenderer->BindTexture(nullptr);                            
}

void Map::AddVertsForTile(std::vector<Vertex_PCU>& verts, int tileIndex) const
{
	Tile const& tile = m_tiles[tileIndex];
	AABB2 bounds = GetTileBounds(tileIndex);

	TileDefinition const* tileDef = tile.m_tileDef;
	if (tileDef == nullptr) {
		return;
	}

	const SpriteDefinition& spriteDef = g_terrainSpriteSheet->GetSpriteDef(tileDef->m_spriteCoords);

	Rgba8 tileColor = tileDef->m_tintColor;
	Vec2 uvAtMins, uvAtMaxs;
	spriteDef.GetUVs(uvAtMins, uvAtMaxs);

	AddVertsForAABB2D(verts, bounds, tileColor, uvAtMins, uvAtMaxs);

	g_theRenderer->BindTexture(&spriteDef.GetTexture());
}




void Map::RenderEntities() const
{
	for (int entityType = 0; entityType < NUM_ENTITY_TYPES; ++entityType)
	{
		EntityList entitiesOfType = m_entitiesByType[entityType];
		for (int entityIndex = 0; entityIndex < (int)entitiesOfType.size(); ++entityIndex)
		{
			Entity* entity = entitiesOfType[entityIndex];
			if (entity != nullptr)
			{
				entity->Render();
			}
		}
	}
}

void Map::RenderExplosions() const
{
	g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
	for (auto it = m_explosions.begin(); it != m_explosions.end();)
	{
		Explosion* explosion = *it;

		if (explosion)
		{
			explosion->Render();
			++it; 
		}
	}
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
}


void Map::RenderDebugRay() const
{
	/*Vec2 mouseUV = g_theWindow->GetNormalizedMouseUV();
	Vec2 camMins = m_game->m_worldCamera.GetOrthographicBottomLeft();
	Vec2 camMaxs = m_game->m_worldCamera.GetOrthographicTopRight();
	AABB2 camBounds(camMins, camMaxs);
	Vec2 mouseWorldPos = camBounds.GetPointAtUV(mouseUV);


	Ray2 ray;
	ray.m_startPos = mouseWorldPos;
	ray.m_fwdNormal = Vec2::MakeFromPolarDegrees(20.f);
	ray.m_maxLength = 3.5f;

	Vec2 rayEndPos = ray.m_startPos + (ray.m_fwdNormal * ray.m_maxLength);

	std::vector<Vertex_PCU> verts;
	float arrowThickness = 0.2f;
	RaycastResult2D rayResult = RaycastVsTiles(ray);
	AddVertsForArrow2D(verts, ray.m_startPos, rayEndPos, arrowThickness, 0.05f, Rgba8(0, 0, 0));
	if (rayResult.m_didImpact)
	{
		Vec2 impactNormalEnd = rayResult.m_impactPos + (rayResult.m_impactNormal * 0.5f);
		AddVertsForDisc2D(verts, rayResult.m_impactPos, 0.07f, Rgba8::BLACK);
		AddVertsForArrow2D(verts, rayResult.m_impactPos, impactNormalEnd, 0.1f, 0.02f, Rgba8::CYAN);
	}

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(verts);*/
}

void Map::PushEntitiesOutOfSolidOrWaterTiles()
{
	for (int entityType = 0; entityType < NUM_ENTITY_TYPES; ++entityType)
	{
		EntityList entitiesOfType = m_entitiesByType[entityType];
		for (int entityIndex = 0; entityIndex < (int)entitiesOfType.size(); ++entityIndex)
		{
			if (entitiesOfType[entityIndex])
			{
				if (isBullet(entitiesOfType[entityIndex]))
				{
					continue;
				}
				if (m_game->m_isF3Active)
				{
					if (entitiesOfType[entityIndex]->m_entityType == ENTITY_TYPE_PLAYERTANK)
					{
						continue;
					}
					PushEntityOutOfSolidOrWaterTiles(*entitiesOfType[entityIndex]);
				}
				else
				{
					PushEntityOutOfSolidOrWaterTiles(*entitiesOfType[entityIndex]);
				}
			}
			
		}
	}
}

void Map::PushEntityOutOfSolidOrWaterTiles(Entity& entity)
{
	IntVec2 entityTileCoords = GetTileCoordsForWorldPos(entity.m_position);

	for (int offsetY = -1; offsetY <= 1; ++offsetY) {
		for (int offsetX = -1; offsetX <= 1; ++offsetX) {
			IntVec2 checkCoords = IntVec2(entityTileCoords.x + offsetX, entityTileCoords.y + offsetY);
			AABB2 tileBounds = GetTileBounds(checkCoords);
			if (!IsPointInsideAABB2D(entity.m_position, tileBounds))
			{
				PushEntityOutOfTileIfSolidOrWater(entity, checkCoords);
			}
		}
	}
}

void Map::PushEntityOutOfTileIfSolidOrWater(Entity& entity, IntVec2 const& tileCoords) const
{
	if (IsTileSolid(tileCoords)) {
		AABB2 tileBounds = GetTileBounds(tileCoords);
		PushDiscOutOfAABB2D(entity.m_position, entity.m_physicalRadius, tileBounds);
	}
	else if (IsTileWater(tileCoords)) {
		if (!entity.CanSwim()) {
			AABB2 tileBounds = GetTileBounds(tileCoords);
			PushDiscOutOfAABB2D(entity.m_position, entity.m_physicalRadius, tileBounds);
		}
	}
}

void Map::HandleCollideAgents()
{
	for (Entity* entityA : m_allEntities)
	{
		for (Entity* entityB : m_allEntities)
		{
			if (isAgent(entityA) && isAgent(entityB) && (entityA != entityB))
			{
				CollideAgents(*entityA, *entityB);
			}
		}
	}
}

void Map::CollideAgents(Entity& a, Entity& b)
{
	bool aPushesB = a.m_doesPushEntities && b.m_isPushedByEntity;
	bool bPushesA = b.m_doesPushEntities && a.m_isPushedByEntity;

	if (aPushesB && bPushesA)
	{
		PushDiscsOutOfEachOther2D(a.m_position, a.m_physicalRadius, b.m_position, b.m_physicalRadius);
	}
	else if (aPushesB)
	{
		PushDiscOutOfDisc2D(b.m_position, b.m_physicalRadius, a.m_position, a.m_physicalRadius);
	}
	else if (bPushesA)
	{
		PushDiscOutOfDisc2D(a.m_position, a.m_physicalRadius, b.m_position, b.m_physicalRadius);
	}
}

void Map::HandleBulletsVsTiles()
{
	for (int bulletFactionIndex = 0; bulletFactionIndex < NUM_ENTITY_FACTION; ++bulletFactionIndex)
	{
		EntityList entitiesOfType = m_bulletsByFaction[bulletFactionIndex];
		for (int entityIndex = 0; entityIndex < (int)entitiesOfType.size(); ++entityIndex)
		{
			Bullet* bullet = dynamic_cast<Bullet*>(entitiesOfType[entityIndex]);

			if (bullet != nullptr)
			{
				Ray2 bulletRay;
				bulletRay.m_startPos = bullet->m_position;
				bulletRay.m_fwdNormal = Vec2::MakeFromPolarDegrees(bullet->m_orientationDegrees);
				bulletRay.m_maxLength = 0.1f;

				RaycastResult2D result = RaycastVsTiles(bulletRay);
				if (result.m_didImpact)
				{
					Vec2 impactPos = result.m_impactPos;
					Vec2 normal = result.m_impactNormal;

					IntVec2 impactTile = GetTileCoordsForWorldPos(impactPos);

					Vec2 extendedImpactPos = impactPos + bulletRay.m_fwdNormal * 0.1f; 
					IntVec2 extendedTileCoords = GetTileCoordsForWorldPos(extendedImpactPos);

					int tileIndex = GetTileIndexForCoords(extendedTileCoords);

					if (tileIndex >= 0 && tileIndex < (int) m_tiles.size())
					{
						Tile* hitTile = &m_tiles[tileIndex];
						if (hitTile->m_health > 0 && bullet->m_entityType != ENTITY_TYPE_PROJECTILE_FIRE)
						{
							hitTile->m_health--;
							if (hitTile->m_health <= 0&& !IsTileInNest(extendedTileCoords) && !IsTileBorder(extendedTileCoords))
							{
								hitTile->m_tileDef = &TileDefinition::GetDefinition(hitTile->m_tileDef->m_tileNameToChangeToWhenDestroyed);
							}
						}
					}

					bullet->Bounce(result);

					bullet->m_health--;
				}
			}
		}
	}
}



void Map::CheckBulletsVsEntities()
{
	CheckPlayerBulletsVsEnemies();
	CheckEnemyBulletsVsPlayer();
}

void Map::CheckPlayerBulletsVsEnemies()
{
	for (int goodBulletIndex = 0; goodBulletIndex < (int) m_bulletsByFaction[ENTITY_FACTION_GOOD].size(); ++goodBulletIndex)
	{
		for (int evilEntityIndex = 0; evilEntityIndex < (int) m_agentsByFaction[ENTITY_FACTION_EVIL].size(); ++evilEntityIndex)
		{
			Entity* goodBullet = m_bulletsByFaction[ENTITY_FACTION_GOOD][goodBulletIndex];
			Entity* evilAgent = m_agentsByFaction[ENTITY_FACTION_EVIL][evilEntityIndex];
			if (!goodBullet || !evilAgent)
			{
				continue;
			}

			if (evilAgent->m_entityType == ENTITY_TYPE_ARIES)
			{
				Ray2 bulletRay;
				bulletRay.m_startPos = goodBullet->m_position;
				bulletRay.m_fwdNormal = Vec2::MakeFromPolarDegrees(goodBullet->m_orientationDegrees);
				bulletRay.m_maxLength = 0.001f;

				Vec2 bulletRayEnd = bulletRay.m_startPos + bulletRay.m_fwdNormal * bulletRay.m_maxLength;

				RaycastResult2D result = RaycastVsDisc2D(bulletRay.m_startPos, bulletRay.m_fwdNormal, bulletRay.m_maxLength,
					evilAgent->m_position, evilAgent->m_physicalRadius);

				if (dynamic_cast<Aries*>(evilAgent)->IsShieldWork(result))
				{
					dynamic_cast<Bullet*>(goodBullet)->Bounce(result);
					goodBullet->m_health--;
				}
				else
				{
					if (DoDiscsOverlap(goodBullet->m_position, goodBullet->m_physicalRadius, evilAgent->m_position, evilAgent->m_physicalRadius - 0.1f))
					{
						HandlePlayerBulletHitEnemy(goodBullet, evilAgent);
					}
				}
				

			}
			else
			{
				if (DoDiscsOverlap(goodBullet->m_position, goodBullet->m_physicalRadius, evilAgent->m_position, evilAgent->m_physicalRadius))
				{
					HandlePlayerBulletHitEnemy(goodBullet, evilAgent);
				}
			}
		}
	}
}

void Map::CheckEnemyBulletsVsPlayer()
{
	for (int evilBulletIndex = 0; evilBulletIndex < (int)m_bulletsByFaction[ENTITY_FACTION_EVIL].size(); ++evilBulletIndex)
	{
		Entity* evilBullet = m_bulletsByFaction[ENTITY_FACTION_EVIL][evilBulletIndex];
		Entity* player = GetPlayer();

		if (evilBullet && !player->m_isDead)
		{
			if (DoDiscsOverlap(evilBullet->m_position, evilBullet->m_physicalRadius, player->m_position, player->m_physicalRadius))
			{
				HandleEnemyBulletHitPlayer(evilBullet, player);
			}
		}
	}
}

void Map::HandlePlayerBulletHitEnemy(Entity* bullet, Entity* enemy) const
{
	bullet->m_health = 0;
	bullet->m_isDead = true;
	bullet->m_isGarbage = true;

	enemy->m_health--;
	g_theAudio->StartSound(m_game->m_hitSound, false, 0.1f);
}

void Map::HandleEnemyBulletHitPlayer(Entity* bullet, Entity* player) const
{
	bullet->m_health = 0;
	bullet->m_isDead = true;
	bullet->m_isGarbage = true;

	if (!m_game->m_isF2Active)
	{
		player->m_health--;
		g_theAudio->StartSound(m_game->m_hitSound, false, 0.1f);
	}
}

bool Map::HasScorpioOnTile(IntVec2 tileCoords, bool treatScorpioAsSolid)
{
	for (Entity* entity : m_entitiesByType[ENTITY_TYPE_SCORPIO])
	{
		if (entity)
		{
			Scorpio* scorpio = dynamic_cast<Scorpio*> (entity);

			IntVec2 scorpioTileCoords = IntVec2(static_cast<int>(scorpio->m_position.x), static_cast<int>(scorpio->m_position.y));
			if (scorpioTileCoords == tileCoords && treatScorpioAsSolid)
			{
				return true;
			}
		}
		
	}
	return false;
}

void Map::FindNextEntity()
{
	if (m_currentEntityIndex >= static_cast<int>(m_agentsByFaction[ENTITY_FACTION_EVIL].size()) - 1)
	{
		m_currentEntityIndex = -1;
		m_currentEntityLosing = true;
		return;
	}

	for (int i = (int) GetClamped(m_currentEntityIndex + 1.f, 0.f, (float)m_agentsByFaction[ENTITY_FACTION_EVIL].size() - 1.f); i < static_cast<int>(m_agentsByFaction[ENTITY_FACTION_EVIL].size()); ++i)
	{
		Entity* entity = m_agentsByFaction[ENTITY_FACTION_EVIL][i];

		if (entity != nullptr && isAgent(entity) && entity->m_entityType != ENTITY_TYPE_SCORPIO)
		{
			m_currentEntityIndex = i;
			m_currentEntityLosing = false;
			return;
		}
	}

	m_currentEntityIndex = -1;
	m_currentEntityLosing = true;
}

void Map::SpawnExplosion(Vec2 position, float size, float duration, SpriteAnimDefinition* animDef) {
	Explosion* newExplosion = new Explosion(this, position, size, duration, animDef);
	m_explosions.push_back(newExplosion);
}

void Map::GenerateEntityPathToGoal(const IntVec2& start, const IntVec2& goal, std::vector<Vec2>& outPath)
{
	TileHeatMap heatMap(m_dimensions);

	PopulateDistanceField(heatMap, start, 999.f, true, true);

	IntVec2 currentTile = goal;
	while (currentTile != start)
	{
		outPath.push_back(GetTileCenterForCoords(currentTile));

		IntVec2 nextTile = GetBestNeighborTile(heatMap, currentTile);
		currentTile = nextTile;
	}

	
}

IntVec2 Map::GetBestNeighborTile(const TileHeatMap& heatMap, const IntVec2& currentTile)
{
	IntVec2 directions[] = {
		IntVec2(1, 0), IntVec2(-1, 0), IntVec2(0, 1), IntVec2(0, -1)
	};

	IntVec2 bestTile = currentTile;
	float minHeatValue = 99.f;

	for (const IntVec2& direction : directions)
	{
		IntVec2 neighborTile = currentTile + direction;

		if (IsTileSolid(neighborTile)) {
			continue;
		}

		float heatValue = heatMap.GetValueAtIndex(GetTileIndexForCoords(neighborTile));

		if (heatValue < minHeatValue)
		{
			minHeatValue = heatValue;
			bestTile = neighborTile;
		}
	}

	return bestTile;
}



void Map::UpdateExplosions(float deltaSeconds)
{
	for (auto it = m_explosions.begin(); it != m_explosions.end();)
	{
		Explosion* explosion = *it;
		if (explosion)
		{
			explosion->Update(deltaSeconds);

			if (explosion->IsFinished())
			{
				delete explosion;
				it = m_explosions.erase(it);
			}
			else
			{
				++it;
			}
		}
		else
		{
			++it;
		}
	}
}


Entity* Map::CreateNewEntity(EntityType type, EntityFaction faction)
{
	switch (type)
	{
	case ENTITY_TYPE_PLAYERTANK: return new PlayerTank(type, faction);
	case ENTITY_TYPE_SCORPIO: return new Scorpio(type, faction);
	case ENTITY_TYPE_LEO: return new Leo(type, faction);
	case ENTITY_TYPE_ARIES: return new Aries(type, faction);

	case ENTITY_TYPE_PROJECTILE_BOLT:
	case ENTITY_TYPE_PROJECTILE_BULLET:
	case ENTITY_TYPE_PROJECTILE_SHELL:
	case ENTITY_TYPE_PROJECTILE_FIRE:
		return new Bullet(type, faction);
	default: ERROR_AND_DIE(Stringf("Unknown entity type"));
	}
}



Entity* Map::SpawnNewEntity(EntityType type, EntityFaction faction, Vec2 position, float orientationDegrees)
{

	Entity* newEntity = CreateNewEntity(type, faction);
	
	AddEntityToMap(newEntity, position, orientationDegrees);
	return newEntity;
}

void Map::AddEntityToMap(Entity* entity, Vec2 const& position, float orientationDegrees)
{
	entity->m_map = this;
	entity->m_position = position;
	entity->m_orientationDegrees = orientationDegrees;

	AddEntityToList(entity, m_allEntities);
	AddEntityToList(entity, m_entitiesByType[entity->m_entityType]);

	if (isBullet(entity))
	{
		AddEntityToList(entity, m_bulletsByFaction[entity->m_entityFaction]);
	}

	if (isAgent(entity))
	{
		AddEntityToList(entity, m_agentsByFaction[entity->m_entityFaction]);
	}
}

void Map::RemoveEntityToMap(Entity* entity)
{
	RemoveEntityFromList(entity, m_allEntities);
	RemoveEntityFromList(entity, m_entitiesByType[entity->m_entityType]);

	if (isBullet(entity))
	{
		RemoveEntityFromList(entity, m_bulletsByFaction[entity->m_entityFaction]);
	}

	if (isAgent(entity))
	{
		RemoveEntityFromList(entity, m_agentsByFaction[entity->m_entityFaction]);
		PopulateDistanceField(*m_heatMap1, IntVec2(1, 1), 999.f, true, false);
		PopulateDistanceField(*m_heatMap2, IntVec2(1, 1), 999.f, false, true);
		PopulateDistanceField(*m_heatMap3, IntVec2(1, 1), 999.f, true, true);
	}
}

void Map::AddEntityToList(Entity* entityToAdd, EntityList& list)
{
	for (int i = 0; i < (int)list.size(); ++i)
	{
		if (list[i] == nullptr)
		{
			list[i] = entityToAdd;
			return;
		}
	}
	list.push_back(entityToAdd);
}

void Map::RemoveEntityFromList(Entity* entityToRemove, EntityList& list)
{
	
	for (int i = 0; i < (int)list.size(); ++i)
	{
		if (list[i] == entityToRemove)
		{
			list[i] = nullptr;
			
			return;
		}
	}

	ERROR_AND_DIE(Stringf("Entity of type was not in list!"));
}

void Map::CheckHealthAndSetGarbages()
{
	for (Entity* entity : m_allEntities)
	{
		if (entity)
		{
			if (entity->m_health <= 0)
			{
				if (isAgent(entity) && !entity->m_isDead)
				{
					g_theAudio->StartSound(m_game->m_deadSound, false, 0.1f);
					SpawnExplosion(entity->m_position, 1.5f, 1.f, m_exploAnimDef);
				}
				else if (isBullet(entity))
				{
					SpawnExplosion(entity->m_position, 0.5f, 0.2f, m_exploAnimDef);

				}

				if (entity->m_entityType == ENTITY_TYPE_PLAYERTANK)
				{
					entity->m_isDead = true;

				} 
				else
				{
					entity->m_isGarbage = true;
					entity->m_isDead = true;
				}

				
			}
		}
		
	}
}

bool Map::isGarbage(Entity* entity) const
{
	return entity->m_isGarbage;
}

void Map::DeleteGarbageEntities()
{
	for (Entity* entity : m_allEntities)
	{
		if (entity)
		{
			if (isGarbage(entity))
			{

				RemoveEntityToMap(entity);
			}
		}
		
	}
}
