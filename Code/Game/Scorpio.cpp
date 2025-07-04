#include "Game/Scorpio.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerTank.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"

Scorpio::Scorpio(EntityType type, EntityFaction faction)
	:Entity(type, faction)
{
	m_physicalRadius = g_gameConfigBlackboard.GetValue("entityBaseRadius", 1.f);
	m_cosmeticRadius = g_gameConfigBlackboard.GetValue("entityTurretRadius", 1.f);
	m_turretSize = Vec2(g_gameConfigBlackboard.GetValue("entityTurretSize", 1.f), g_gameConfigBlackboard.GetValue("entityTurretSize", 1.f));

	m_health = g_gameConfigBlackboard.GetValue("scorpioHealth", 1);;
	m_maxHealth = m_health;

	m_isPushedByEntity = false;
	m_turretTargetDegrees = 0.f;

	m_scorpioBaseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyTurretBase.png");
	m_scorpioTurretTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyCannon.png");
}

Scorpio::~Scorpio()
{
}

void Scorpio::Update(float deltaSeconds)
{
	m_playerPos = m_map->GetPlayer()->m_position;

	UpdateTurret(deltaSeconds);
}


void Scorpio::UpdateTurret(float deltaSeconds)
{
	Vec2 directionToPlayer = m_playerPos - m_position;
	float targetOrientation = directionToPlayer.GetOrientationDegrees();

	bool isPlayerVisible = IsPlayerVisible();

	float rotationSpeed;

	if (isPlayerVisible && !m_map->GetPlayer()->m_isDead) {
		rotationSpeed = 60.f;
		float angleDifference = GetShortestAngularDispDegrees(m_orientationDegrees, targetOrientation);

		if (abs(angleDifference) < rotationSpeed * deltaSeconds) {
			m_orientationDegrees = targetOrientation;
		}
		else {
			m_orientationDegrees += rotationSpeed * deltaSeconds * (angleDifference > 0 ? 1.0f : -1.0f);
		}

		m_fireTimer += deltaSeconds;
		if (m_fireTimer >= m_fireCooldown && abs(angleDifference) <= 5.f)
		{
			m_fireTimer = 0.f;
			Vec2 forwardNormal = GetForwardNormal(m_orientationDegrees);
			Vec2 nosePosition = m_position + (forwardNormal * 0.5f);
			g_theAudio->StartSound(m_map->m_game->m_fireSound, false, 0.1f);
			m_map->SpawnNewEntity(ENTITY_TYPE_PROJECTILE_BOLT, ENTITY_FACTION_EVIL, nosePosition, m_orientationDegrees);
		}

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
	else {
		m_fpSoundPlay = false;
		rotationSpeed = 30.f;
		m_orientationDegrees -= rotationSpeed * deltaSeconds;
	}

	if (m_orientationDegrees >= 360.0f) {
		m_orientationDegrees -= 360.0f;
	}
	else if (m_orientationDegrees < 0.0f) {
		m_orientationDegrees += 360.0f;
	}
}

bool Scorpio::IsPlayerVisible() const
{
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




void Scorpio::Render() const
{
	RenderBase();
	RenderTurret();
	RenderLaser();
	RenderHealthBar();
}



void Scorpio::DebugRender() const
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
	Vec2 velCartPos = Vec2::MakeFromPolarDegrees(Atan2Degrees(m_velocity.y, m_velocity.x), m_velocity.GetLength());
	Vec2 velPos = Vec2(m_position.x + velCartPos.x, m_position.y + velCartPos.y);
	DebugDrawLine(m_position, velPos, 0.02f, Rgba8(255, 255, 0, 255));
}

void Scorpio::Die()
{
}

void Scorpio::RenderBase() const
{
	std::vector<Vertex_PCU> baseVerts;

	AddVertsForAABB2D(baseVerts, m_scorpioLocalBounds, Rgba8(255, 255, 255, 255));

	TransformVertexArrayXY3D(static_cast<int>(baseVerts.size()), baseVerts.data(), 1.f, m_orientationDegrees, m_position);

	g_theRenderer->BindTexture(m_scorpioBaseTexture);
	g_theRenderer->DrawVertexArray(static_cast<int>(baseVerts.size()), baseVerts.data());
	g_theRenderer->BindTexture(nullptr);
}

void Scorpio::RenderTurret() const
{
	std::vector<Vertex_PCU> turretVerts;

	AddVertsForAABB2D(turretVerts, m_turretLocalBounds, Rgba8(255, 255, 255, 255));

	TransformVertexArrayXY3D(static_cast<int>(turretVerts.size()), turretVerts.data(), 1.f, m_orientationDegrees, m_position);

	g_theRenderer->BindTexture(m_scorpioTurretTexture);
	g_theRenderer->DrawVertexArray(static_cast<int>(turretVerts.size()), turretVerts.data());
	g_theRenderer->BindTexture(nullptr);
}

void Scorpio::RenderLaser() const
{
	float maxRange = g_gameConfigBlackboard.GetValue("enemyViewRange", 1.f);
	Vec2 laserStart = m_position + Vec2::MakeFromPolarDegrees(m_orientationDegrees, m_turretSize.x / 3.0f);

	Vec2 laserEnd = laserStart + Vec2::MakeFromPolarDegrees(m_orientationDegrees, maxRange);

	Rgba8 startColor = Rgba8(255, 0, 0, 255);
	Rgba8 endColor = Rgba8(255, 0, 0, 0);

	Vec2 scorpioPos = m_position;

	Vec2 direction = Vec2::MakeFromPolarDegrees(m_orientationDegrees);

	Ray2 ray;
	ray.m_startPos = scorpioPos;
	ray.m_fwdNormal = direction;
	ray.m_maxLength = maxRange;

	RaycastResult2D tileHit = m_map->RaycastVsTiles(ray);
	Vec2 laserImpact = tileHit.m_impactPos;
	
	std::vector<Vertex_PCU> laserVerts;

	if (tileHit.m_didImpact)
	{
		AddVertsForLineSegment2D(laserVerts, laserStart, laserImpact, 0.05f, startColor);
	}
	else
	{
		AddVertsForLaserTriangle(laserVerts, laserStart, laserEnd, 0.05f, startColor, endColor);
	}
	g_theRenderer->BindTexture(nullptr); 
	g_theRenderer->DrawVertexArray(laserVerts);
}

void Scorpio::RenderHealthBar() const
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


void Scorpio::AddVertsForLaserTriangle(std::vector<Vertex_PCU>& verts, Vec2 const& startPos, Vec2 const& endPos, float thickness, Rgba8 const& startColor, Rgba8 const& endColor) const
{
	Vec2 forward = (endPos - startPos).GetNormalized();
	Vec2 left = forward.GetRotated90Degrees();

	Vec2 offset = left * (thickness * 0.5f);

	Vec3 p0 = Vec3(startPos.x, startPos.y, 0.0f);
	Vec3 p1 = Vec3(startPos.x - offset.x, startPos.y - offset.y, 0.0f);
	Vec3 p2 = Vec3(startPos.x + offset.x, startPos.y + offset.y, 0.0f);
	Vec3 p3 = Vec3(endPos.x, endPos.y, 0.0f);

	verts.push_back(Vertex_PCU(p1, startColor, Vec2(0, 0)));
	verts.push_back(Vertex_PCU(p2, startColor, Vec2(0, 0)));
	verts.push_back(Vertex_PCU(p3, endColor, Vec2(0, 0)));
}
