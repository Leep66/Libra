#include "Game/PlayerTank.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/XboxController.hpp"
#include "Engine/Input/NamedStrings.hpp"
#include "Engine/Core/Time.hpp"

extern InputSystem* g_theInput;
extern NamedStrings g_gameConfigBlackboard;

PlayerTank::PlayerTank(EntityType type, EntityFaction faction)
	:Entity(type, faction)
{
	m_speed = g_gameConfigBlackboard.GetValue("playerMoveSpeed", 1.0f);
	m_transitionSpeed = 10.f;
	m_physicalRadius = g_gameConfigBlackboard.GetValue("entityBaseRadius", 1.0f);
	m_cosmeticRadius = g_gameConfigBlackboard.GetValue("entityTurretRadius", 1.0f);
	m_turretSize = Vec2(g_gameConfigBlackboard.GetValue("entityTurretSize", 1.0f), g_gameConfigBlackboard.GetValue("entityTurretSize", 1.0f));

	m_health = g_gameConfigBlackboard.GetValue("playerHealth", 1);
	m_maxHealth = m_health;

	m_tankTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PlayerTankBase.png");
	m_turretTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PlayerTankTop.png");
}

PlayerTank::~PlayerTank()
{

}

void PlayerTank::Update(float deltaSeconds)
{
	m_isMoving = false;
	m_isAiming = false;

	if (!m_isDead)
	{
		UpdateFromKeyboard(deltaSeconds);
		UpdateFromController(deltaSeconds);

		m_position += m_velocity * deltaSeconds;
		m_turretOffsetDegrees = m_orientationDegrees - m_turretOrientationDegrees;
	}
}



void PlayerTank::UpdateFromKeyboard(float deltaSeconds)
{
	// Tank Moving
	if (g_theInput->IsKeyDown('E')) {
		m_targetDegrees = 90.f;
		m_isMoving = true;
	}
	if (g_theInput->IsKeyDown('D')) {
		m_targetDegrees = 270.f;
		m_isMoving = true;
	}
	if (g_theInput->IsKeyDown('F')) {
		m_targetDegrees = 0.f;
		m_isMoving = true;
	}
	if (g_theInput->IsKeyDown('S')) {
		m_targetDegrees = 180.f;
		m_isMoving = true;
	}

	if (g_theInput->IsKeyDown('E') && g_theInput->IsKeyDown('F')) {
		m_targetDegrees = 45.f;
		m_isMoving = true;
	}
	if (g_theInput->IsKeyDown('E') && g_theInput->IsKeyDown('S')) {
		m_targetDegrees = 135.f;
		m_isMoving = true;
	}
	if (g_theInput->IsKeyDown('D') && g_theInput->IsKeyDown('F')) {
		m_targetDegrees = 315.f;
		m_isMoving = true;
	}
	if (g_theInput->IsKeyDown('D') && g_theInput->IsKeyDown('S')) {
		m_targetDegrees = 225.f;
		m_isMoving = true;
	}

	if (m_isMoving)
	{
		m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, m_targetDegrees, g_gameConfigBlackboard.GetValue("playerTurnSpeed", 1.f) * deltaSeconds);
		Vec2 forwardDirection = Vec2(CosDegrees(m_orientationDegrees), SinDegrees(m_orientationDegrees));
		m_velocity = forwardDirection * m_speed;
		m_turretOrientationDegrees = GetTurnedTowardDegrees(m_turretOrientationDegrees, m_targetDegrees - m_turretOffsetDegrees, g_gameConfigBlackboard.GetValue("playerTurnSpeed", 1.f) * deltaSeconds);
	}
	else
	{
		m_velocity = Vec2(0.f, 0.f);
	}

	// Turret Aiming
	if (g_theInput->IsKeyDown('I')) {
		m_turretTargetDegrees = 90.f;
		m_isAiming = true;
	}
	if (g_theInput->IsKeyDown('J')) {
		m_turretTargetDegrees = 180.f;
		m_isAiming = true;
	}
	if (g_theInput->IsKeyDown('K')) {
		m_turretTargetDegrees = 270.f;
		m_isAiming = true;
	}
	if (g_theInput->IsKeyDown('L')) {
		m_turretTargetDegrees = 0.f;
		m_isAiming = true;
	}

	if (g_theInput->IsKeyDown('I') && g_theInput->IsKeyDown('J'))
	{
		m_turretTargetDegrees = 135.f;
	}

	if (g_theInput->IsKeyDown('I') && g_theInput->IsKeyDown('L'))
	{
		m_turretTargetDegrees = 45.f;
	}

	if (g_theInput->IsKeyDown('K') && g_theInput->IsKeyDown('J'))
	{
		m_turretTargetDegrees = 225.f;
	}

	if (g_theInput->IsKeyDown('K') && g_theInput->IsKeyDown('L'))
	{
		m_turretTargetDegrees = 315.f;
	}

	if (m_isAiming) {
		m_turretOrientationDegrees = GetTurnedTowardDegrees(m_turretOrientationDegrees, m_turretTargetDegrees, g_gameConfigBlackboard.GetValue("playerTurnSpeed", 1.f) * deltaSeconds);
	}

	m_fireBoltTimer += deltaSeconds;
	m_fireFlameTimer += deltaSeconds;

	// Tank Firing
	if (g_theInput->IsKeyDown(' '))
	{
		if (m_fireBoltTimer >= m_fireBoltCoolDown && !m_map->m_game->m_isPaused)
		{
			m_fireBoltTimer = 0.f;
			Vec2 forwardNormal = GetForwardNormal(m_turretOrientationDegrees);
			Vec2 nosePosition = m_position + (forwardNormal * 0.5f);
			g_theAudio->StartSound(m_map->m_game->m_fireSound, false, 0.1f);
			m_map->SpawnNewEntity(ENTITY_TYPE_PROJECTILE_BOLT, ENTITY_FACTION_GOOD, nosePosition, m_turretOrientationDegrees);
		}
	}

	if (g_theInput->IsKeyDown('H'))
	{
		if (!m_map->m_game->m_isPaused)
		{
			if (m_fireFlameTimer >= m_fireFlameCoolDown && !m_map->m_game->m_isPaused)
			{
				m_fireFlameTimer = 0.f;
				Vec2 forwardNormal = GetForwardNormal(m_turretOrientationDegrees);
				Vec2 nosePosition = m_position + (forwardNormal * 0.5f);

				float randomOffset = m_map->m_game->m_rng->RollRandomFloatInRange(-30.f, 30.f);
				float flameOrientationDegrees = m_turretOrientationDegrees + randomOffset;
				Vec2 flameForwardNormal = GetForwardNormal(flameOrientationDegrees);

				//g_theAudio->StartSound(m_map->m_game->m_fireSound, false, 0.05f);
				m_map->SpawnNewEntity(ENTITY_TYPE_PROJECTILE_FIRE, ENTITY_FACTION_GOOD, nosePosition, flameOrientationDegrees);
			}
		}
	}



	
}

void PlayerTank::UpdateFromController(float deltaSeconds)
{
	XboxController const& controller = g_theInput->GetController(0);
	float leftStickMagnitude = controller.GetLeftStick().GetMagnitude();
	if (leftStickMagnitude > 0.f)
	{
		float thrustFraction = leftStickMagnitude;
		m_targetDegrees = controller.GetLeftStick().GetOrientationDegrees();

		Vec2 forwardDirection = Vec2(CosDegrees(m_orientationDegrees), SinDegrees(m_orientationDegrees));
		m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, m_targetDegrees, g_gameConfigBlackboard.GetValue("playerTurnSpeed", 1.f) * deltaSeconds);
		m_velocity = forwardDirection *  thrustFraction * m_speed;
		m_turretOrientationDegrees = GetTurnedTowardDegrees(m_turretOrientationDegrees, m_targetDegrees - m_turretOffsetDegrees, g_gameConfigBlackboard.GetValue("playerTurnSpeed", 1.f) * deltaSeconds);
	}

	float rightStickMagnitude = controller.GetRightStick().GetMagnitude();
	if (rightStickMagnitude > 0.f)
	{
		m_turretTargetDegrees = controller.GetRightStick().GetOrientationDegrees();
		m_turretOrientationDegrees = GetTurnedTowardDegrees(m_turretOrientationDegrees, m_turretTargetDegrees, g_gameConfigBlackboard.GetValue("playerTurnSpeed", 1.f) * deltaSeconds);
	}

	if (controller.WasButtonJustPressed(XBOX_BUTTON_A))
	{
		if (m_fireBoltTimer >= m_fireBoltCoolDown && !m_map->m_game->m_isPaused)
		{
			m_fireBoltTimer = 0.f;
			Vec2 forwardNormal = GetForwardNormal(m_turretOrientationDegrees);
			Vec2 nosePosition = m_position + (forwardNormal * 0.5f);
			g_theAudio->StartSound(m_map->m_game->m_fireSound, false, 0.1f);
			m_map->SpawnNewEntity(ENTITY_TYPE_PROJECTILE_BOLT, ENTITY_FACTION_GOOD, nosePosition, m_turretOrientationDegrees);
		}
	}

	if (controller.WasButtonJustPressed(XBOX_BUTTON_B))
	{
		if (!m_map->m_game->m_isPaused)
		{
			if (m_fireFlameTimer >= m_fireFlameCoolDown && !m_map->m_game->m_isPaused)
			{
				m_fireFlameTimer = 0.f;
				Vec2 forwardNormal = GetForwardNormal(m_turretOrientationDegrees);
				Vec2 nosePosition = m_position + (forwardNormal * 0.5f);

				float randomOffset = m_map->m_game->m_rng->RollRandomFloatInRange(-30.f, 30.f);
				float flameOrientationDegrees = m_turretOrientationDegrees + randomOffset;
				Vec2 flameForwardNormal = GetForwardNormal(flameOrientationDegrees);
				m_map->SpawnNewEntity(ENTITY_TYPE_PROJECTILE_FIRE, ENTITY_FACTION_GOOD, nosePosition, flameOrientationDegrees);
			}
		}
	}

}

void PlayerTank::RenderTank() const
{
	float newScale = 1.f;
	if (m_map->m_game->m_isIntro)
	{
		float fraction = m_map->m_game->m_introTimer / m_map->m_game->m_transitionTime;
		newScale = Interpolate(0.f, 1.f, fraction);
	}
	else if (m_map->m_game->m_isOutro)
	{
		float fraction = m_map->m_game->m_outroTimer / m_map->m_game->m_transitionTime;
		newScale = Interpolate(1.f, 0.f, fraction);
	}


	std::vector<Vertex_PCU> tankVerts;
	
	AddVertsForAABB2D(tankVerts, m_tankLocalBounds, Rgba8(255, 255, 255, 255));

	TransformVertexArrayXY3D(static_cast<int>(tankVerts.size()), tankVerts.data(), newScale, m_orientationDegrees, m_position);

	g_theRenderer->BindTexture(m_tankTexture);
	g_theRenderer->DrawVertexArray(static_cast<int>(tankVerts.size()), tankVerts.data());
	g_theRenderer->BindTexture(nullptr);
}

void PlayerTank::RenderTurret() const
{
	float newScale = 1.f;
	if (m_map->m_game->m_isIntro)
	{
		float fraction = m_map->m_game->m_introTimer / m_map->m_game->m_transitionTime;
		newScale = Interpolate(0.f, 1.f, fraction);
	}
	else if (m_map->m_game->m_isOutro)
	{
		float fraction = m_map->m_game->m_outroTimer / m_map->m_game->m_transitionTime;
		newScale = Interpolate(1.f, 0.f, fraction);
	}

	std::vector<Vertex_PCU> turretVerts;
	if (m_turretSize.x > 0.f && m_turretSize.y > 0.f) {
		Vec2 turretPos = Vec2::MakeFromPolarDegrees(m_turretTargetDegrees, m_turretSize.x);
		
		AddVertsForAABB2D(turretVerts, m_turretLocalBounds, Rgba8(255, 255, 255, 255));
	}

	TransformVertexArrayXY3D(static_cast<int>(turretVerts.size()), turretVerts.data(), newScale, m_turretOrientationDegrees, m_position);

	g_theRenderer->BindTexture(m_turretTexture);
	g_theRenderer->DrawVertexArray(static_cast<int>(turretVerts.size()), turretVerts.data());
	g_theRenderer->BindTexture(nullptr);
}

void PlayerTank::RenderHealthBar() const
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


void PlayerTank::Render() const
{

	if (!m_isDead)
	{
		RenderTank();
		RenderTurret();
		RenderHealthBar();

		// F2 Active
		if (m_map->m_game->m_isF2Active)
		{
			DebugDrawRing(m_position, m_physicalRadius - 0.05f, 0.05f, Rgba8::WHITE);
		}

		// F3 Active
		if (m_map->m_game->m_isF3Active)
		{
			DebugDrawRing(m_position, m_physicalRadius - 0.1f, 0.05f, Rgba8::BLACK);
		}
	}
	
}

void PlayerTank::DebugRender() const
{
	// Inner Ring
	DebugDrawRing(m_position, m_physicalRadius, 0.05f, Rgba8(0, 200, 200, 255));
	// Outer Ring
	// DebugDrawRing(m_position, m_cosmeticRadius, 0.05f, Rgba8(255, 0, 255, 255));

	// IBasis
	Vec2 fwd = Vec2::MakeFromPolarDegrees(m_orientationDegrees, m_cosmeticRadius);
	Vec2 fwdPos = Vec2(m_position.x + fwd.x, m_position.y + fwd.y);
	DebugDrawLine(m_position, fwdPos, 0.02f, Rgba8(255, 0, 0, 255));

	// JBasis
	Vec2 left = Vec2::MakeFromPolarDegrees(m_orientationDegrees + 90.f, m_physicalRadius);
	Vec2 leftPos = Vec2(m_position.x + left.x, m_position.y + left.y);
	DebugDrawLine(m_position, leftPos, 0.02f, Rgba8(0, 255, 0, 255));

	// Velocity
	Vec2 velCartPos = Vec2::MakeFromPolarDegrees(Atan2Degrees(m_velocity.y, m_velocity.x), m_velocity.GetLength());
	Vec2 velPos = Vec2(m_position.x + velCartPos.x, m_position.y + velCartPos.y);
	DebugDrawLine(m_position, velPos, 0.02f, Rgba8(255, 255, 0, 255));

	// Target
	Vec2 targetStart = Vec2::MakeFromPolarDegrees(m_targetDegrees, m_physicalRadius);
	Vec2 targetEnd = Vec2::MakeFromPolarDegrees(m_targetDegrees, m_physicalRadius);
	Vec2 targetStartPos = Vec2(m_position.x + targetStart.x, m_position.y + targetStart.y);
	Vec2 targetEndPos = Vec2(m_position.x + targetEnd.x, m_position.y + targetEnd.y);
	DebugDrawLine(targetStartPos, targetEndPos, 0.1f, Rgba8(255, 0, 0, 255));

	// Turret
	Vec2 turretPos = m_position + Vec2::MakeFromPolarDegrees(m_turretOrientationDegrees, m_turretSize.x / 3);
	// DebugDrawLine(m_position, turretPos, 0.15f, Rgba8(0, 0, 255, 255));

	Vec2 turretTargetStart = Vec2::MakeFromPolarDegrees(m_turretTargetDegrees, m_physicalRadius);
	Vec2 turretTargetEnd = Vec2::MakeFromPolarDegrees(m_turretTargetDegrees, m_physicalRadius + 0.05f);
	Vec2 turretTargetStartPos = Vec2(m_position.x + turretTargetStart.x, m_position.y + turretTargetStart.y);
	Vec2 turretTargetEndPos = Vec2(m_position.x + turretTargetEnd.x, m_position.y + turretTargetEnd.y);

	// DebugDrawLine(turretTargetStartPos, turretTargetEndPos, 0.2f, Rgba8(0, 0, 255, 255));

	
}

void PlayerTank::Die()
{
}

void PlayerTank::SetRotationSpeed(float speed)
{
	m_transitionSpeed = speed;
}
