#include "Game/Bullet.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerTank.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"

Bullet::Bullet(EntityType type, EntityFaction faction)
	:Entity(type, faction)
{
	m_doesPushEntities = false;
	m_isPushedByEntity = false;
	m_isPushedByWall = false;
	m_isHittedByBullet = false;

	m_physicalRadius = 0.1f;
	m_cosmeticRadius = 0.12f;

	
	Startup();
	m_maxHealth = m_health;
}

Bullet::~Bullet()
{
}

void Bullet::Startup()
{
	char const* bulletName = "1";
	if (m_entityType == ENTITY_TYPE_PROJECTILE_BOLT && m_entityFaction == ENTITY_FACTION_GOOD)
	{
		bulletName = "FriendlyBolt";
		m_health = 3;
	}
	else if (m_entityType == ENTITY_TYPE_PROJECTILE_BULLET && m_entityFaction == ENTITY_FACTION_GOOD)
	{
		bulletName = "FriendlyBullet";
		m_health = 1;
	}
	else if (m_entityType == ENTITY_TYPE_PROJECTILE_SHELL && m_entityFaction == ENTITY_FACTION_GOOD)
	{
		bulletName = "FriendlyShell";
		m_health = 1;
	}
	else if (m_entityType == ENTITY_TYPE_PROJECTILE_FIRE && m_entityFaction == ENTITY_FACTION_GOOD)
	{
		bulletName = "FriendlyFire";
		m_health = 1;
		m_duration = 0.5f;
		m_haveDuration = true;
	}
	else if (m_entityType == ENTITY_TYPE_PROJECTILE_BOLT && m_entityFaction == ENTITY_FACTION_EVIL)
	{
		bulletName = "EnemyBolt";
		m_health = 2;
	}
	else if (m_entityType == ENTITY_TYPE_PROJECTILE_BULLET && m_entityFaction == ENTITY_FACTION_EVIL)
	{
		bulletName = "EnemyBullet";
		m_health = 1;
	}
	else if (m_entityType == ENTITY_TYPE_PROJECTILE_SHELL && m_entityFaction == ENTITY_FACTION_EVIL)
	{
		bulletName = "EnemyShell";
		m_health = 1;
	}
	
	else if (m_entityType == ENTITY_TYPE_PROJECTILE_FIRE && m_entityFaction == ENTITY_FACTION_EVIL)
	{
		bulletName = "FriendlyFire";
		m_health = 1;
		m_duration = 1.f;
		m_haveDuration = true;
	}


	std::string bulletTextureFilePath = Stringf("Data/Images/%s.png", bulletName);
	m_BulletTexture = g_theRenderer->CreateOrGetTextureFromFile(bulletTextureFilePath.c_str());
}

void Bullet::Update(float deltaSeconds)
{
	if (!m_isBounced)
	{
		float bulletSpeed = g_gameConfigBlackboard.GetValue("entityBulletSpeed", 1.f);
		Vec2 bulletDirection = Vec2::MakeFromPolarDegrees(m_orientationDegrees).GetNormalized();
		m_velocity = bulletDirection * bulletSpeed;
	}
	

	m_position += (m_velocity * deltaSeconds);

	if (m_duration > 0)
	{
		m_duration -= deltaSeconds;
	}

	if (m_duration <= 0.f && m_haveDuration)
	{
		m_isDead = true;
		m_isGarbage = true;
	}
}

void Bullet::Render() const
{
	std::vector<Vertex_PCU> bulletVerts;

	AddVertsForAABB2D(bulletVerts, m_baseLocalBounds, Rgba8(255, 255, 255, 255));

	TransformVertexArrayXY3D(static_cast<int>(bulletVerts.size()), bulletVerts.data(), 1.f, m_orientationDegrees, m_position);

	g_theRenderer->BindTexture(m_BulletTexture);

	if (m_entityType == ENTITY_TYPE_PROJECTILE_FIRE)
	{
		g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
	}
	g_theRenderer->DrawVertexArray(static_cast<int>(bulletVerts.size()), bulletVerts.data());

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);

	g_theRenderer->BindTexture(nullptr);
}

void Bullet::DebugRender() const
{
	// Inner Ring
	DebugDrawRing(m_position, m_physicalRadius, 0.05f, Rgba8(0, 200, 200, 255));
	// Outer Ring
	DebugDrawRing(m_position, m_cosmeticRadius, 0.05f, Rgba8(255, 0, 255, 255));

	// IBasis
	Vec2 fwd = Vec2::MakeFromPolarDegrees(m_orientationDegrees, m_cosmeticRadius);
	Vec2 fwdPos = Vec2(m_position.x + fwd.x, m_position.y + fwd.y);
	DebugDrawLine(m_position, fwdPos, 0.02f, Rgba8(255, 0, 0, 255));

	// JBasis
	Vec2 left = Vec2::MakeFromPolarDegrees(m_orientationDegrees + 90.f, m_cosmeticRadius);
	Vec2 leftPos = Vec2(m_position.x + left.x, m_position.y + left.y);
	DebugDrawLine(m_position, leftPos, 0.02f, Rgba8(0, 255, 0, 255));

	// Velocity
	Vec2 velCartPos = Vec2::MakeFromPolarDegrees(Atan2Degrees(m_velocity.y, m_velocity.x), m_velocity.GetLength());
	Vec2 velPos = Vec2(m_position.x + velCartPos.x, m_position.y + velCartPos.y);
	DebugDrawLine(m_position, velPos, 0.02f, Rgba8(255, 255, 0, 255));
}

void Bullet::Die()
{
}

void Bullet::Bounce(RaycastResult2D result)
{
	Vec2 normal = result.m_impactNormal.GetNormalized();
	m_velocity = m_velocity.GetReflected(normal);

	m_orientationDegrees = m_velocity.GetOrientationDegrees();
}



