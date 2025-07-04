#pragma once
#include "Engine/Core/EngineCommon.hpp"

class Renderer;
class Window;
class InputSystem;
class AudioSystem;
class App;
struct Vec2;
struct Rgba8;

extern Renderer*		g_theRenderer;
extern Window*			g_theWindow;
extern InputSystem*		g_theInput;
extern AudioSystem*		g_theAudio;
extern App*				g_theApp;

extern bool g_isDebugDraw;


/*
constexpr float g_gameConfigBlackboard.GetValue("screenWidth", 1.f) = 1600;
constexpr float g_gameConfigBlackboard.GetValue("screenHeight", 1.f) = 800;
constexpr float WORLD_SIZE_X = 64.f;
constexpr float WORLD_SIZE_Y = 32.f;
constexpr float g_gameConfigBlackboard.GetValue("entityBaseRadius", 1.f) = 0.4f;
constexpr float g_gameConfigBlackboard.GetValue("entityTurretRadius", 1.f) = 0.6f;
constexpr float g_gameConfigBlackboard.GetValue("entityTurretSize", 1.f) = 1.4f;
constexpr float g_gameConfigBlackboard.GetValue("entityTurretSize", 1.f) = 1.4f;
constexpr float g_gameConfigBlackboard.GetValue("enemyViewRange", 1.f) = 10.f;
constexpr float g_gameConfigBlackboard.GetValue("enemyRaycastLength", 1.f) = 0.9f;

constexpr float PLAYER_TURN_SPEED = 180.f;
constexpr float g_gameConfigBlackboard.GetValue("enemyTurnSpeed", 1.f) = 120.f;
constexpr float g_gameConfigBlackboard.GetValue("entityBulletSpeed", 1.f) = 3.f;
constexpr float LIBRA_MOVE_SPEED = 1.f;
constexpr float g_gameConfigBlackboard.GetValue("enemyMoveSpeed", 1.f) = 0.5f;*/



void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);

void DebugDrawLine(Vec2 const& S, Vec2 const& E, float thickness, Rgba8 const& color);