#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Audio/AudioSystem.hpp"

#include "Game/Map.hpp"
#include "Engine/Renderer/Camera.hpp"

class Game 
{
public:

	Game(App* owner);	
	~Game();
	void Startup();

	void Update(float deltaSeconds);
	void Render() const;

	void Shutdown();

	void WinCurrentMap();
	
private:
	void AdjustForPauseAndTimeDistortion(float& deltaSeconds);
	void DebugRender() const;
	void UpdateCameras();
	void UpdateAttractMode(float deltaSeconds);
	void UpdateAttractRing(float deltaSeconds);
	void UpdateInput();
	void UpdateDeadScreen(float deltaSeconds);
	void UpdateTransition(float deltaSeconds);

	void EnterAttractMode();


	void RenderAttractMode() const;
	void RenderAttractBackground() const;
	void RenderAttractRing() const;
	void RenderAttractTestTexture() const;

	void RenderGame() const;
	void RenderUI() const;
	




public:
	App* m_App = nullptr;
	Map* m_currentMap = nullptr;
	int m_currentMapIndex = 0;
	RandomNumberGenerator* m_rng = nullptr;

	std::vector<Map*> m_maps;
	Texture* m_terrainTexture = nullptr;
	PlayerTank* m_playerTank = nullptr;
	bool m_isF1Active = false;
	bool m_isF2Active = false;
	bool m_isF3Active = false;
	bool m_isF4Active = false;
	bool m_isF6Active = true;
	int m_f6Index = 0;

	bool m_isAttractMode = true;
	float m_attractRingRadius = 150.f;
	float m_attractRingThickness = 20.f;
	float m_attractRingRadiusGrowSpeed = 5.f;
	float m_attractRingThinknessGrowSpeed = 20.f;

	Rgba8 m_startColor = Rgba8(0, 255, 0, 255);
	Camera m_screenCamera;
	Camera m_worldCamera;

	bool m_isPaused = false;
	bool m_pauseAfterUpdate = false;
	bool m_isSlowMo = false;
	bool m_isFastMo = false;
	bool m_isVictory = false;

	SoundID m_attractModeMusic;
	SoundPlaybackID m_attractModePlayback;

	SoundID m_gameModeMusic;
	SoundPlaybackID m_gameModePlayback;

	SoundID m_pauseSound;
	SoundID m_fireSound;
	SoundID m_hitSound;
	SoundID m_deadSound;
	SoundID m_victorySound;
	SoundID m_switchMapSound;
	SoundID m_findPlayerSound;
	float m_fpsoundTimer = 0.f;
	

	AABB2 m_deadScreenBounds;
	Texture* m_deadScreenTexture = nullptr;

	AABB2 m_victoryScreenBounds;
	Texture* m_victoryScreenTexture = nullptr;

	bool m_isDeadScreenAppear = false;
	float m_deadTimer = 0.f;
	float m_deadScreenTime = 3.f;

	AABB2 m_transitionScreenBounds;
	bool m_isIntro = false;
	bool m_isOutro = false;
	float m_introTimer = 0.f;
	float m_outroTimer = 0.f;
	float m_transitionTime = 1.f;
	float m_timelastFrame = (1.f / 60.f);

};