#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Input/NamedStrings.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/PlayerTank.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

extern App* g_theApp;
extern Renderer* g_theRenderer;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern NamedStrings g_gameConfigBlackboard;
extern BitmapFont* g_theFont;

SpriteSheet* g_terrainSpriteSheet = nullptr;



Game::Game(App* owner)
	: m_App(owner)
{
	Startup();
}

Game::~Game()
{
	g_theAudio->StopSound(m_attractModePlayback);
	g_theAudio->StopSound(m_gameModePlayback);

	m_currentMap = nullptr;
	m_maps.clear();
	for (int i = 0; i < (int)m_maps.size(); ++i)
	{
		delete m_maps[i];
	}
	
	delete m_playerTank;
	m_playerTank = nullptr;
	
}

void Game::Startup()
{
	m_terrainTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Terrain_8x8.png");
	g_terrainSpriteSheet = new SpriteSheet(*m_terrainTexture, IntVec2(8, 8));

	m_rng = new RandomNumberGenerator();

	TileDefinition::InitializeTileDefinitions();
	MapDefinition::InitializeMapDefinitions();
	
	m_maps.push_back(new Map(this, "First"));
	m_maps.push_back(new Map(this, "Second"));
	m_maps.push_back(new Map(this, "Third"));
	m_maps.push_back(new Map(this, "Fourth"));
	m_maps.push_back(new Map(this, "Fifth"));

	m_currentMap = m_maps[0];
	Entity* playerEntity = m_currentMap->SpawnNewEntity(ENTITY_TYPE_PLAYERTANK, ENTITY_FACTION_GOOD, Vec2(1.5f, 1.5f), 45.f);
	m_playerTank = dynamic_cast<PlayerTank*>(playerEntity);

	m_deadScreenTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/YouDiedScreen.png");
	m_victoryScreenTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/VictoryScreen.jpg");

	m_attractModeMusic = g_theAudio->CreateOrGetSound("Data/Audio/attractMusic.mp3");
	m_attractModePlayback = g_theAudio->StartSound(m_attractModeMusic, true, 0.1f);
	m_gameModeMusic = g_theAudio->CreateOrGetSound("Data/Audio/gameMusic.mp3");

	m_pauseSound = g_theAudio->CreateOrGetSound("Data/Audio/Pause.wav");
	m_fireSound = g_theAudio->CreateOrGetSound("Data/Audio/Fire.wav");
	m_hitSound = g_theAudio->CreateOrGetSound("Data/Audio/Hit.wav");
	m_deadSound = g_theAudio->CreateOrGetSound("Data/Audio/Dead.wav");
	m_victorySound = g_theAudio->CreateOrGetSound("Data/Audio/Victory.mp3");
	m_switchMapSound = g_theAudio->CreateOrGetSound("Data/Audio/SwitchMap.wav");
	m_findPlayerSound = g_theAudio->CreateOrGetSound("Data/Audio/FindPlayer.wav");

	m_deadScreenBounds = AABB2(0.f, 0.f, g_gameConfigBlackboard.GetValue("screenWidth", 1.f), g_gameConfigBlackboard.GetValue("screenHeight", 1.f));
	m_victoryScreenBounds = AABB2(0.f, 0.f, g_gameConfigBlackboard.GetValue("screenWidth", 1.f), g_gameConfigBlackboard.GetValue("screenHeight", 1.f));
	m_transitionScreenBounds = AABB2(0.f, 0.f, g_gameConfigBlackboard.GetValue("screenWidth", 1.f), g_gameConfigBlackboard.GetValue("screenHeight", 1.f));
	
	m_currentMap->CreateHeatMap();
}

void Game::Update(float deltaSeconds)
{	
	UpdateCameras();
	
	AdjustForPauseAndTimeDistortion(deltaSeconds);

	if (m_isAttractMode)
	{
		UpdateAttractMode(deltaSeconds);
		return;
	}
	UpdateInput();


	UpdateTransition(deltaSeconds);


	if (m_playerTank && !m_playerTank->m_isDead)
	{
		m_currentMap->Update(deltaSeconds);

		float distToEndPos = (m_playerTank->m_position - m_currentMap->m_endPos).GetLength();
		const float DIST_THRESHOLD = 0.3f;

		if (distToEndPos < DIST_THRESHOLD)
		{
			m_isOutro = true;
		}
	}
	else
	{
		UpdateDeadScreen(deltaSeconds);
	}

	m_fpsoundTimer += deltaSeconds;
}

void Game::UpdateInput()
{
	XboxController const& controller = g_theInput->GetController(0);

	if (g_theInput->IsKeyDown('T'))
	{
		m_isSlowMo = true;
		g_theAudio->SetSoundPlaybackSpeed(m_gameModePlayback, 0.5f);
	}
	if (g_theInput->WasKeyJustReleased('T'))
	{
		m_isSlowMo = false;
		g_theAudio->SetSoundPlaybackSpeed(m_gameModePlayback, 1.f);
	}

	if (g_theInput->IsKeyDown('Y'))
	{
		m_isFastMo = true;
		g_theAudio->SetSoundPlaybackSpeed(m_gameModePlayback, 2.f);
	}
	if (g_theInput->WasKeyJustReleased('Y'))
	{
		m_isFastMo = false;
		g_theAudio->SetSoundPlaybackSpeed(m_gameModePlayback, 1.f);
	}

	if (g_theInput->WasKeyJustPressed(' ') || g_theInput->WasKeyJustPressed(KEYCODE_ESC) ||
		controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		if (m_isVictory)
		{
			EnterAttractMode();
		}
	}

	if (g_theInput->WasKeyJustPressed('P') || g_theInput->WasKeyJustPressed(KEYCODE_ESC)
		|| controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		if (!m_isPaused) {
			if (!m_playerTank->m_isDead) {
				m_isPaused = true;
				g_theAudio->StartSound(m_pauseSound, false, 0.1f);
				g_theAudio->SetSoundPlaybackSpeed(m_gameModePlayback, 0.f);
			}
		}
		else {
			if (g_theInput->WasKeyJustPressed('P') && !m_isVictory && !m_isDeadScreenAppear) {
				m_isPaused = false;
				g_theAudio->StartSound(m_pauseSound, false, 0.1f);
				g_theAudio->SetSoundPlaybackSpeed(m_gameModePlayback, 1.f);
			}
			else if (!m_isVictory && (m_deadTimer == 0.f || m_deadTimer >= 3.0f) && !m_isIntro && !m_isOutro) {
				EnterAttractMode();
			}
			else if (m_isVictory) {
				EnterAttractMode();
			}
		}
	}

	if (g_theInput->WasKeyJustPressed('N') || controller.WasButtonJustPressed(XBOX_BUTTON_START))
	{
		if (m_isDeadScreenAppear)
		{
			m_playerTank->m_health = m_playerTank->m_maxHealth;
			m_playerTank->m_isDead = false;
			
			m_isPaused = false;
			m_isDeadScreenAppear = false;
			m_deadTimer = 0.f;
			g_theAudio->SetSoundPlaybackSpeed(m_gameModePlayback, 1.f);
		}
		
	}

	if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
	{
		if (m_isPaused)
		{
			m_isPaused = false;
			g_theAudio->SetSoundPlaybackSpeed(m_gameModePlayback, 1.f);
		}
	}

	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_pauseAfterUpdate = true;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		m_isF1Active = !m_isF1Active;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
	{
		m_isF2Active = !m_isF2Active;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F3))
	{
		m_isF3Active = !m_isF3Active;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F4))
	{
		m_isF4Active = !m_isF4Active;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
	{
		m_f6Index++;
	}

	if (m_f6Index % 5 == 4 && g_theInput->WasKeyJustPressed(KEYCODE_F7))
	{
		m_currentMap->FindNextEntity();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		m_isAttractMode = true;
		g_theApp->ResetGame();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F9))
	{
		if (!m_playerTank->m_isDead && !m_isVictory && !m_isIntro && !m_isOutro)
		{
			m_isOutro = true;
		}
	}

}

void Game::UpdateDeadScreen(float deltaSeconds)
{
	m_deadTimer += deltaSeconds;
	if (m_deadTimer < m_deadScreenTime && m_currentMap)
	{
		m_currentMap->Update(deltaSeconds);
	}
	else
	{
		m_isDeadScreenAppear = true;
		m_isPaused = true;
		g_theAudio->SetSoundPlaybackSpeed(m_gameModePlayback, 0.f);
	}
}

void Game::UpdateTransition(float deltaSeconds)
{
	UNUSED (deltaSeconds)
	float timeNow = static_cast<float>(GetCurrentTimeSeconds());
	float deltaTime = timeNow - m_timelastFrame;
	m_timelastFrame = timeNow;

	if (m_isIntro && !m_isOutro && m_playerTank)
	{
		m_isPaused = true;
		m_introTimer += deltaTime;

		float fraction = m_introTimer / m_transitionTime;
		float totalRotation = Interpolate(0.f, 720.f, fraction);
		float directionMultiplier = -1.f;

		m_playerTank->m_orientationDegrees = 45.f + totalRotation * directionMultiplier;
		dynamic_cast<PlayerTank*> (m_playerTank)->m_turretOrientationDegrees = 45.f + totalRotation * directionMultiplier;

		

		if (m_introTimer > m_transitionTime)
		{
			m_isPaused = false;
			m_isOutro = false;
			m_isIntro = false;
			m_introTimer = 0.f;
			m_outroTimer = 0.f;
		}
	}
	else if (m_isOutro && !m_isIntro)
	{
		m_outroTimer += deltaTime;
		m_isPaused = true;

		float fraction = m_outroTimer / m_transitionTime;
		float totalRotation = Interpolate(0.f, 720.f, fraction);
		float directionMultiplier = 1.f;

		m_playerTank->m_orientationDegrees = 45.f + totalRotation * directionMultiplier;
		dynamic_cast<PlayerTank*> (m_playerTank)->m_turretOrientationDegrees = 45.f + totalRotation * directionMultiplier;


		if (m_outroTimer > m_transitionTime)
		{
			m_isPaused = false;

			m_isOutro = false;
			m_isIntro = true;
			m_introTimer = 0.f;
			m_outroTimer = 0.f;
			WinCurrentMap();
		}
	}
}




void Game::EnterAttractMode() {
	m_isAttractMode = true;

	SoundID clickSound = g_theAudio->CreateOrGetSound("Data/Audio/click.wav");
	g_theAudio->StartSound(clickSound, false, 1.f, 0.f, 0.5f);
	g_theAudio->StopSound(m_gameModePlayback);
	g_theApp->ResetGame();
}


void Game::Render() const
{
	if (!m_isAttractMode)
	{
		RenderGame();
		RenderUI();

	}
	else
	{
		RenderAttractMode();
	}

}


void Game::RenderGame() const
{
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 255));
	
	g_theRenderer->BeginCamera(m_worldCamera);

	m_currentMap->Render();
	DebugRender();
	
	

	g_theRenderer->EndCamera(m_worldCamera);

}

void Game::RenderUI() const
{
	g_theRenderer->BeginCamera(m_screenCamera);

	if (m_isIntro)
	{
		std::vector <Vertex_PCU> verts;
		float alpha = RangeMap(m_introTimer, 0.f, m_transitionTime, 255.f, 0.f);

		

		AddVertsForAABB2D(verts, m_transitionScreenBounds, Rgba8(0, 0, 0, (unsigned char) alpha));
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(verts);
	}

	if (m_isOutro)
	{
		std::vector <Vertex_PCU> verts;
		float alpha = RangeMap(m_outroTimer, 0.f, m_transitionTime, 0.f, 255.f);

		AddVertsForAABB2D(verts, m_transitionScreenBounds, Rgba8(0, 0, 0, (unsigned char) alpha));
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(verts);
	}

	if (m_isDeadScreenAppear)
	{
		std::vector <Vertex_PCU> verts;

		AddVertsForAABB2D(verts, m_deadScreenBounds, Rgba8(255, 255, 255, 200));
		g_theRenderer->BindTexture(m_deadScreenTexture);
		g_theRenderer->DrawVertexArray(verts);
		g_theRenderer->BindTexture(nullptr);
	}

	if (m_isVictory)
	{
		std::vector <Vertex_PCU> verts;

		AddVertsForAABB2D(verts, m_deadScreenBounds, Rgba8(255, 255, 255, 255));
		g_theRenderer->BindTexture(m_victoryScreenTexture);
		g_theRenderer->DrawVertexArray(verts);
		g_theRenderer->BindTexture(nullptr);
	}

	

	if (m_f6Index % 5 == 1)
	{
		std::vector<Vertex_PCU> textVerts;
		g_theFont->AddVertsForText2D(textVerts, Vec2(10.f, 770.f), 20.f, "Heat map Debug: Distance Map from start (F6 for next mode)", Rgba8::GREEN, 0.8f);
		g_theRenderer->BindTexture(&g_theFont->GetTexture());
		g_theRenderer->DrawVertexArray(textVerts);

	}

	if (m_f6Index % 5 == 2)
	{
		std::vector<Vertex_PCU> textVerts;
		g_theFont->AddVertsForText2D(textVerts, Vec2(10.f, 770.f), 20.f, "Heat map Debug: Solid Map for amphibians (F6 for next mode)", Rgba8::GREEN, 0.8f);
		g_theRenderer->BindTexture(&g_theFont->GetTexture());
		g_theRenderer->DrawVertexArray(textVerts);

	}

	if (m_f6Index % 5 == 3)
	{
		std::vector<Vertex_PCU> textVerts;
		g_theFont->AddVertsForText2D(textVerts, Vec2(10.f, 770.f), 20.f, "Heat map Debug: Solid Map for land-based (F6 for next mode)", Rgba8::GREEN, 0.8f);
		g_theRenderer->BindTexture(&g_theFont->GetTexture());
		g_theRenderer->DrawVertexArray(textVerts);

	}

	if (m_f6Index % 5 == 4)
	{
		std::vector<Vertex_PCU> textVerts;
		g_theFont->AddVertsForText2D(textVerts, Vec2(10.f, 770.f), 20.f, "Heat map Debug: Distance Map to selected Entity's goal (F6 for next mode)", Rgba8::GREEN, 0.8f);
		g_theRenderer->BindTexture(&g_theFont->GetTexture());
		g_theRenderer->DrawVertexArray(textVerts);

	}

	g_theRenderer->EndCamera(m_screenCamera);

}

void Game::UpdateAttractMode(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	XboxController const& controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed('P') || controller.WasButtonJustPressed(XBOX_BUTTON_A)
		||g_theInput->WasKeyJustPressed(' ') || controller.WasButtonJustPressed(XBOX_BUTTON_START))
	{
		m_isAttractMode = false;
		g_theAudio->StopSound(m_attractModePlayback);
		m_gameModePlayback = g_theAudio->StartSound(m_gameModeMusic, true, 0.1f);
		SoundID clickSound = g_theAudio->CreateOrGetSound("Data/Audio/click.wav");
		g_theAudio->StartSound(clickSound, false, 1.f, 0.f, 0.5f);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_theApp->HandleQuitRequested();
	}

	UpdateAttractRing(deltaSeconds);
	
}

void Game::UpdateAttractRing(float deltaSeconds)
{
	
	float maxRadius = 200.f;
	float minRadius = 100.f;

	m_attractRingRadius += m_attractRingRadiusGrowSpeed * deltaSeconds;
	if (m_attractRingRadius >= maxRadius || m_attractRingRadius <= minRadius)
	{
		m_attractRingRadiusGrowSpeed = -m_attractRingRadiusGrowSpeed;
	}
	

	float maxThickness = 30.f;
	float minThickness = 10.f;
	m_attractRingThickness += m_attractRingThinknessGrowSpeed * deltaSeconds;
	if (m_attractRingThickness >= maxThickness || m_attractRingThickness <= minThickness)
	{
		m_attractRingThinknessGrowSpeed = -m_attractRingThinknessGrowSpeed;
	}
	
}

void Game::RenderAttractMode() const
{
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 255));
	g_theRenderer->BeginCamera(m_screenCamera);
	
	RenderAttractBackground();
	RenderAttractRing();



	/*RenderAttractTestTexture();
	g_theFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont"); // DO NOT SPECIFY FILE .EXTENSION!!  (Important later on.)

	std::vector<Vertex_PCU> textVerts;
	g_theFont->AddVertsForText2D(textVerts, Vec2(100.f, 200.f), 30.f, "Hello, world");
	g_theFont->AddVertsForText2D(textVerts, Vec2(250.f, 400.f), 15.f, "It's nice to have options!", Rgba8::RED, 0.6f);
	g_theRenderer->BindTexture(&g_theFont->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);*/

	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderAttractBackground() const
{
	Texture* backgroundTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/AttractScreen.png");
	std::vector<Vertex_PCU> verts;
	AABB2 texturedAABB2(0, 0, g_gameConfigBlackboard.GetValue("screenWidth", 1.f), g_gameConfigBlackboard.GetValue("screenHeight", 1.f));
	AddVertsForAABB2D(verts, texturedAABB2, Rgba8(255, 255, 255, 255));
	g_theRenderer->BindTexture(backgroundTexture);
	g_theRenderer->DrawVertexArray(verts);
	g_theRenderer->BindTexture(nullptr);
}

void Game::RenderAttractRing() const
{
	DebugDrawRing(Vec2(800.f, 400.f), m_attractRingRadius, m_attractRingThickness, Rgba8(255, 127, 0, 255));
	g_theRenderer->BindTexture(nullptr);
}

void Game::RenderAttractTestTexture() const
{
	Texture* testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
	std::vector<Vertex_PCU> verts;
	AABB2 texturedAABB2(500.f, 100.f, 900.f, 500.f);
	AddVertsForAABB2D(verts, texturedAABB2, Rgba8(255, 255, 255, 255));
	g_theRenderer->BindTexture(testTexture);
	g_theRenderer->DrawVertexArray(verts);
	g_theRenderer->BindTexture(nullptr);
}

void Game::Shutdown()
{

}

void Game::WinCurrentMap()
{
	m_currentMap->RemoveEntityToMap(m_playerTank);
	m_playerTank = nullptr;
	
	if (m_currentMapIndex < 5)
	{
		m_currentMapIndex++;
	}

	if (m_currentMapIndex == 5)
	{
		m_isVictory = true;
		m_isPaused = true;
		g_theAudio->StartSound(m_victorySound, false, 0.1f);
		g_theAudio->SetSoundPlaybackSpeed(m_gameModePlayback, 0.f);
	}
	else
	{
		m_currentMap = m_maps[m_currentMapIndex];
		g_theAudio->StartSound(m_switchMapSound, false, 0.1f);
	}
	
	Entity* playerEntity = m_currentMap->SpawnNewEntity(ENTITY_TYPE_PLAYERTANK, ENTITY_FACTION_GOOD, Vec2(1.5f, 1.5f), 45.f);
  	m_playerTank = dynamic_cast<PlayerTank*>(playerEntity);
}

void Game::DebugRender() const
{
	if (m_isF1Active)
	{
		m_currentMap->DebugRender();
	}

}

void Game::UpdateCameras()
{
	m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(g_gameConfigBlackboard.GetValue("screenWidth", 1.f), g_gameConfigBlackboard.GetValue("screenHeight", 1.f)));
	
}

void Game::AdjustForPauseAndTimeDistortion(float& deltaSeconds)
{
	if (m_isPaused && !m_pauseAfterUpdate)
	{
		deltaSeconds = 0.f;
	}

	if (m_isSlowMo)
	{
		deltaSeconds *= 0.10f;
	}

	if (m_pauseAfterUpdate)
	{
		m_isPaused = true;
		m_pauseAfterUpdate = false;
	}

	if (m_isFastMo)
	{
		deltaSeconds *= 4.f;
	}
}


