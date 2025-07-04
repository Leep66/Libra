#include "Game/Leo.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerTank.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/TileHeatMap.hpp"


Leo::Leo(EntityType type, EntityFaction faction)
	:Entity(type, faction)
{
	m_physicalRadius = g_gameConfigBlackboard.GetValue("entityBaseRadius", 1.f);
	m_cosmeticRadius = g_gameConfigBlackboard.GetValue("entityTurretRadius", 1.f);

	m_health = g_gameConfigBlackboard.GetValue("leoHealth", 1);
	m_maxHealth = m_health;

	m_leoBaseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyTank3.png");
}

Leo::~Leo()
{
	m_pathPoints.clear();
}

void Leo::Update(float deltaSeconds)
{
	m_playerPos = m_map->GetTileCenterForCoords(m_map->GetTileCoordsForWorldPos(m_map->GetPlayer()->m_position));

	if (!m_heatMap) {
		m_heatMap = new TileHeatMap(m_map->m_dimensions);
		m_map->PopulateDistanceField(*m_heatMap, m_map->GetTileCoordsForWorldPos(m_position), 999.f, true, true);
	}

	if (m_targetPosition == Vec2::ZERO || 
		((m_position - m_targetPosition).GetLength() <= m_physicalRadius))
	{
		ChooseNewTargetPosition();
	}

	if (IsPlayerVisible() && !m_map->GetPlayer()->m_isDead)
	{
		m_targetPosition = m_playerPos;
		m_goalPosition = m_playerPos;
		GeneratePathToGoal();
		if (!m_fpSoundPlay)
		{
			if (m_map->m_game->m_fpsoundTimer > 0.5f)
			{
				g_theAudio->StartSound(m_map->m_game->m_findPlayerSound, false, 0.1f);

			}
			m_fpSoundPlay = true;
			m_map->m_game->m_fpsoundTimer = 0.f;
		}
	}
	else
	{
		m_goalPosition = m_targetPosition;
		m_fpSoundPlay = false;
	}

	

	if (m_pathPoints.size() >= 2)
	{
		FollowPath(deltaSeconds);
	}
	else
	{
		GeneratePathToGoal();
	}


	UpdateOrientation(deltaSeconds);
	MoveTowardTarget(deltaSeconds);
	HandleFiring(deltaSeconds);
}

void Leo::RenderHealthBar() const
{
	constexpr float healthBarWidth = 0.7f;
	constexpr float healthBarHeight = 0.05f;
	constexpr float borderThickness = 0.02f;

	Vec2 healthBarOffset(0.f, 0.5f);
	Vec2 healthBarPos = m_position + healthBarOffset;

	float healthRatio = static_cast<float>(m_health) / static_cast<float>(m_maxHealth);

	AABB2 healthBarBounds(healthBarPos.x - healthBarWidth * 0.5f,
		healthBarPos.y,
		healthBarPos.x + healthBarWidth * 0.5f,
		healthBarPos.y + healthBarHeight);

	AABB2 borderBounds(healthBarBounds.m_mins.x - borderThickness,
		healthBarBounds.m_mins.y - borderThickness,
		healthBarBounds.m_maxs.x + borderThickness,
		healthBarBounds.m_maxs.y + borderThickness);

	std::vector<Vertex_PCU> healthBarVerts;
	std::vector<Vertex_PCU> borderVerts;

	AddVertsForAABB2D(healthBarVerts, healthBarBounds, Rgba8::RED);
	AddVertsForAABB2D(borderVerts, borderBounds, Rgba8::BLACK);

	AABB2 filledBarBounds(healthBarBounds.m_mins.x,
		healthBarBounds.m_mins.y,
		healthBarBounds.m_mins.x + healthBarBounds.GetDimensions().x * healthRatio,
		healthBarBounds.m_maxs.y);

	std::vector<Vertex_PCU> filledBarVerts;
	AddVertsForAABB2D(filledBarVerts, filledBarBounds, Rgba8::GREEN);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray((int)borderVerts.size(), &borderVerts[0]);
	g_theRenderer->DrawVertexArray((int)healthBarVerts.size(), &healthBarVerts[0]);
	g_theRenderer->DrawVertexArray((int)filledBarVerts.size(), &filledBarVerts[0]);
}

void Leo::GeneratePathToGoal()
{
	m_pathPoints.clear();
	IntVec2 startTile = m_map->GetTileCoordsForWorldPos(m_position);
	IntVec2 goalTile = m_map->GetTileCoordsForWorldPos(m_goalPosition);

	m_map->GenerateEntityPathToGoal(startTile, goalTile, m_pathPoints);
}

void Leo::FollowPath(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	if (m_pathPoints.empty())
	{
		ChooseNewTargetPosition();
		GeneratePathToGoal();
		return;
	}

	Vec2 nextPathPoint = m_pathPoints.back();

	if ((m_position - nextPathPoint).GetLength() <= m_physicalRadius)
	{
		m_pathPoints.pop_back();
	}

	if (!m_pathPoints.empty())
	{
		m_nextWaypoint = m_pathPoints.back();
	}

}

bool Leo::IsPlayerVisible()
{
	if (m_map->m_game->m_playerTank->m_isDead)
	{
		return false;
	}
	Vec2 playerPos = m_playerPos;
	Vec2 scorpioPos = m_position;

	Vec2 direction = (playerPos - scorpioPos).GetNormalized();
	float maxDistance = g_gameConfigBlackboard.GetValue("enemyViewRange", 1.f);

	Ray2 ray;
	ray.m_startPos = scorpioPos;
	ray.m_fwdNormal = direction;
	ray.m_maxLength = maxDistance;

	RaycastResult2D tileHit = m_map->RaycastVsTiles(ray);

	RaycastResult2D playerHit = RaycastVsDisc2D(ray.m_startPos, ray.m_fwdNormal, ray.m_maxLength, playerPos, g_gameConfigBlackboard.GetValue("entityBaseRadius", 1.f));

	if (tileHit.m_didImpact && tileHit.m_impactDist < playerHit.m_impactDist)
	{
		return false;
	}
	else if (playerHit.m_didImpact)
	{
		return true;
	}

	return false;
}



void Leo::UpdateOrientation(float deltaSeconds)
{
	float distanceToPlayer = (m_position - m_playerPos).GetLength();

	if (distanceToPlayer < 2.f && IsPlayerVisible())
	{
		m_targetOrientationDeg = (m_playerPos - m_position).GetOrientationDegrees();
	}
	else
	{
		m_targetOrientationDeg = (m_nextWaypoint - m_position).GetOrientationDegrees();
	}

	float rotationSpeed = g_gameConfigBlackboard.GetValue("enemyTurnSpeed", 1.f) * deltaSeconds;
	m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, m_targetOrientationDeg, rotationSpeed);
}


void Leo::MoveTowardTarget(float deltaSeconds)
{
	float angleDifference = GetShortestAngularDispDegrees(m_orientationDegrees, m_targetOrientationDeg);

	if (abs(angleDifference) <= 45.f) {
		Vec2 forwardDirection = Vec2(CosDegrees(m_orientationDegrees), SinDegrees(m_orientationDegrees));
		m_velocity = forwardDirection * g_gameConfigBlackboard.GetValue("enemyMoveSpeed", 1.f);
		m_position += m_velocity * deltaSeconds;
	}
}

void Leo::HandleFiring(float deltaSeconds)
{
	m_fireTimer += deltaSeconds;


	float playerAngleDifference = GetShortestAngularDispDegrees(m_orientationDegrees, (m_playerPos - m_position).GetOrientationDegrees());
	if (m_fireTimer >= m_fireCooldown && abs(playerAngleDifference) <= 5.f && IsPlayerVisible()) {
		m_fireTimer = 0.f;
		Vec2 forwardNormal = GetForwardNormal(m_orientationDegrees);
		Vec2 nosePosition = m_position + (forwardNormal * 0.3f);
		g_theAudio->StartSound(m_map->m_game->m_fireSound, false, 0.1f);
		m_map->SpawnNewEntity(ENTITY_TYPE_PROJECTILE_BULLET, ENTITY_FACTION_EVIL, nosePosition, m_orientationDegrees);
	}
}


void Leo::ChooseNewTargetPosition()
{
	Vec2 newTarget;
	IntVec2 randomTile = m_map->GetValidPosition(*m_heatMap);
	if (!m_map->IsTileBorder(randomTile)) {
		newTarget = m_map->GetTileCenterForCoords(randomTile);
	}

	m_targetPosition = newTarget;
	m_map->PopulateDistanceField(*m_heatMap, m_map->GetTileCoordsForWorldPos(m_position), 999.f, true, true);
}

void Leo::Render() const
{
	std::vector<Vertex_PCU> baseVerts;

	AddVertsForAABB2D(baseVerts, m_baseLocalBounds, Rgba8(255, 255, 255, 255));

	TransformVertexArrayXY3D(static_cast<int>(baseVerts.size()), baseVerts.data(), 1.f, m_orientationDegrees, m_position);

	g_theRenderer->BindTexture(m_leoBaseTexture);
	g_theRenderer->DrawVertexArray(static_cast<int>(baseVerts.size()), baseVerts.data());
	g_theRenderer->BindTexture(nullptr);

	RenderHealthBar();

}

void Leo::DebugRender() const
{
	// Inner Ring
	DebugDrawRing(m_position, g_gameConfigBlackboard.GetValue("entityBaseRadius", 1.f), 0.05f, Rgba8(0, 200, 200, 255));
	// Outer Ring
	// DebugDrawRing(m_position, g_gameConfigBlackboard.GetValue("entityTurretRadius", 1.f), 0.05f, Rgba8(255, 0, 255, 255));

	// IBasis
	Vec2 fwd = Vec2::MakeFromPolarDegrees(m_orientationDegrees, g_gameConfigBlackboard.GetValue("entityTurretRadius", 1.f));
	Vec2 fwdPos = Vec2(m_position.x + fwd.x, m_position.y + fwd.y);
	DebugDrawLine(m_position, fwdPos, 0.02f, Rgba8(255, 0, 0, 255));

	// JBasis
	Vec2 left = Vec2::MakeFromPolarDegrees(m_orientationDegrees + 90.f, g_gameConfigBlackboard.GetValue("entityTurretRadius", 1.f));
	Vec2 leftPos = Vec2(m_position.x + left.x, m_position.y + left.y);
	DebugDrawLine(m_position, leftPos, 0.02f, Rgba8(0, 255, 0, 255));

	// Velocity
	/*Vec2 velCartPos = Vec2::MakeFromPolarDegrees(Atan2Degrees(m_velocity.y, m_velocity.x), m_velocity.GetLength());
	Vec2 velPos = Vec2(m_position.x + velCartPos.x, m_position.y + velCartPos.y);
	DebugDrawLine(m_position, velPos, 0.02f, Rgba8(255, 255, 0, 255));*/

	// Target
	/*Vec2 targetStart = Vec2::MakeFromPolarDegrees(m_targetOrientationDeg, g_gameConfigBlackboard.GetValue("entityTurretRadius", 1.f));
	Vec2 targetEnd = Vec2::MakeFromPolarDegrees(m_targetOrientationDeg, g_gameConfigBlackboard.GetValue("entityTurretRadius", 1.f) + 0.05f);
	Vec2 targetStartPos = Vec2(m_position.x + targetStart.x, m_position.y + targetStart.y);
	Vec2 targetEndPos = Vec2(m_position.x + targetEnd.x, m_position.y + targetEnd.y);
	DebugDrawLine(targetStartPos, targetEndPos, 0.1f, Rgba8(255, 0, 0, 255));
*/

	// Target Position
	if (m_targetPosition != Vec2::ZERO)
	{
		DebugDrawLine(m_position, m_targetPosition, 0.03f, Rgba8(255, 255, 255, 200));
		DebugDrawRing(m_targetPosition, 0.02f, 0.1f, Rgba8(255, 255, 255, 200));
	}

	// Next Tile Position

		DebugDrawLine(m_position, m_nextWaypoint, 0.03f, Rgba8(127, 127, 127, 200));
		DebugDrawRing(m_nextWaypoint, 0.02f, 0.1f, Rgba8(127, 127, 127, 200));


	// RaycastVsTiles Result
	/*Vec2 forwardDir = Vec2::MakeFromPolarDegrees(m_orientationDegrees);

	Ray2 ray;
	ray.m_startPos = m_position;
	ray.m_fwdNormal = forwardDir;
	ray.m_maxLength = g_gameConfigBlackboard.GetValue("enemyRaycastLength", 1.f);

	Vec2 rayEndPos = ray.m_startPos + (ray.m_fwdNormal * ray.m_maxLength);

	std::vector<Vertex_PCU> verts;

	float arrowThickness = 0.2f;

	RaycastResult2D rayResult = m_map->RaycastVsTiles(ray);
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

void Leo::Die()
{
}



