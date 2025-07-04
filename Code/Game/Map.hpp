#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/Entity.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Game/MapDefinition.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Game/Explosion.hpp"
#include <vector>
#include <string>

class Game;
struct Tile;
class PlayerTank;
class TileHeatMap;
class Explosion;


typedef std::vector<Entity*> EntityList;

class Map
{
public:
	Map(Game* game, std::string name);
	~Map();

	void Startup();
	void Update(float deltaSeconds);
	void Render() const;
	void DebugRender() const;


	IntVec2 GetValidPosition(TileHeatMap const& heatmap) const;
	Entity* GetPlayer() const;
	AABB2 GetTileBounds(IntVec2 const& tileCoords) const;
	AABB2 GetTileBounds(int tileIndex) const;
	IntVec2 GetTileCoordsForWorldPos(Vec2 const& worldPos) const;
	Vec2 GetTileCenterForCoords(IntVec2 const& tileCoords) const;

	bool IsInBounds(IntVec2 const& tileCoords) const;
	bool IsTileBorder(IntVec2 const& tileCoords) const;
	bool IsTileSolid(IntVec2 const& tileCoords) const;
	bool IsTileWater(IntVec2 const& tileCoords) const;
	bool IsTileInNest(IntVec2 const& tileCoords) const;
	bool IsPosInSolid(Vec2 worldPos) const;

	bool isBullet(Entity* entity) const;
	bool isAgent(Entity* entity) const;

	int GetTileIndexForCoords(IntVec2 const& tileCoords) const;

	Entity* SpawnNewEntity(EntityType type, EntityFaction faction, Vec2 position, float orientationDeg);
	void AddEntityToMap(Entity* entity, Vec2 const& position, float orientationDeg);
	void RemoveEntityToMap(Entity* entity);
	RaycastResult2D RaycastVsTiles(Ray2 const& ray) const;

	void CreateHeatMap();
	void PopulateDistanceField(TileHeatMap& out_distanceField, IntVec2 startCoords, float maxCost, bool treatWaterAsSolid, bool treatScorpioAsSolid);
	void RenderHeatMapMode1() const;
	void RenderHeatMapMode2() const;
	void RenderHeatMapMode3() const;
	void RenderHeatMapMode4() const;
	void FindNextEntity();
	
	void SpawnExplosion(Vec2 position, float size, float duration, SpriteAnimDefinition* animDef);

	void GenerateEntityPathToGoal(const IntVec2& start, const IntVec2& goal, std::vector<Vec2>& outPath);
	IntVec2 GetBestNeighborTile(const TileHeatMap& heatMap, const IntVec2& currentTile);
private:
	void UpdateCamera();

	void InitializeTiles();
	void SpawnInitialNPCs();
	Vec2 GetValidSpawnPosition();
	IntVec2 GetValidWormStartPosition();

	void UpdateEntities(float deltaSeconds);

	void UpdateExplosions(float deltaSeconds);
	Entity* CreateNewEntity(EntityType type, EntityFaction faction);
	void AddEntityToList(Entity* entityToAdd, EntityList& list);
	void RemoveEntityFromList(Entity* entityToRemove, EntityList& list);
	void CheckHealthAndSetGarbages();
	bool isGarbage(Entity* entity) const;
	void DeleteGarbageEntities();


	void RenderTiles() const;
	void AddVertsForTile(std::vector<Vertex_PCU>& verts, int tileIndex) const;
	void RenderEntities() const;
	void RenderExplosions() const;
	void RenderDebugRay() const;
	
	void PushEntitiesOutOfSolidOrWaterTiles();
	void PushEntityOutOfSolidOrWaterTiles(Entity& entity);
	void PushEntityOutOfTileIfSolidOrWater(Entity& entity, IntVec2 const& tileCoords) const;

	void HandleCollideAgents();
	void CollideAgents(Entity& a, Entity& b);
	void HandleBulletsVsTiles();
	
	void CheckBulletsVsEntities();
	void CheckPlayerBulletsVsEnemies();
	void CheckEnemyBulletsVsPlayer();
	void HandlePlayerBulletHitEnemy(Entity* bullet, Entity* enemy) const;
	void HandleEnemyBulletHitPlayer(Entity* bullet, Entity* player) const;

	bool HasScorpioOnTile(IntVec2 tileCoords, bool treatScorpioAsSolid);	
	void SetTileRegion(IntVec2 const& minCoords, IntVec2 const& maxCoords, std::string const& tileType);
	void PlaceWorms();
	void PlaceSpecificWorm(std::string const& tileType, int numWorms, int maxLength);
	void PlaceSpecificWormRecursive(IntVec2 currentCoords, std::string const& tileType, int wormLength, int currentLength);


	IntVec2 GetRandomDirection();
public:
	Game* m_game = nullptr;
	IntVec2 m_dimensions = IntVec2(-1, -1);
	std::vector<Tile> m_tiles;

	const MapDefinition* m_mapDef = nullptr;

	EntityList m_allEntities;

	EntityList m_entitiesByType[NUM_ENTITY_TYPES];
	EntityList m_bulletsByFaction[NUM_ENTITY_FACTION];
	EntityList m_agentsByFaction[NUM_ENTITY_FACTION];

	Vec2 m_endPos;

	TileHeatMap* m_heatMap1 = nullptr;
	TileHeatMap* m_heatMap2 = nullptr;
	TileHeatMap* m_heatMap3 = nullptr;

	int m_currentEntityIndex = -1;
	bool m_currentEntityLosing = false;
	std::vector<Explosion*> m_explosions;

	Texture* m_exploAnimTexture = nullptr;
	SpriteSheet* m_exploAnimSheet = nullptr;
	SpriteAnimDefinition* m_exploAnimDef = nullptr;
};