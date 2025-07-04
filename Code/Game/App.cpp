#include "Game/App.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Window/Window.hpp"
#include "Game/Game.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Input/NamedStrings.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EventSystem.hpp"
#include <iostream>
#include <winuser.rh>


App*			g_theApp		= nullptr;
Renderer*		g_theRenderer	= nullptr;
InputSystem*	g_theInput		= nullptr;
AudioSystem*	g_theAudio		= nullptr;
Window*			g_theWindow		= nullptr;
BitmapFont* g_theFont = nullptr;
DevConsole* g_theDevConsole = nullptr;
EventSystem* g_theEventSystem = nullptr;

extern NamedStrings g_gameConfigBlackboard;

float m_timeLastFrameStart = (1.f / 60.f);

App::App()
{

}

App::~App() 
{

}

void App::Startup()
{
	XmlDocument gameDoc;
	XmlResult result = gameDoc.LoadFile("Data/GameConfig.xml");

	if (result != tinyxml2::XML_SUCCESS)
	{
		ERROR_AND_DIE("Failed to load Data/GameConfig.xml");
	}

	XmlElement* gameRoot = gameDoc.RootElement();
	if (gameRoot == nullptr)
	{
		ERROR_AND_DIE("GameConfig.xml is missing a root element");
	}

	g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*gameRoot);

	InputConfig inputConfig;
	g_theInput = new InputSystem(inputConfig);

	WindowConfig windowConfig;
	windowConfig.m_aspectRatio = g_gameConfigBlackboard.GetValue("windowAspect",1.0f);
	windowConfig.m_inputSystem = g_theInput;
	windowConfig.m_windowTitle = g_gameConfigBlackboard.GetValue("projectName", Stringf("Unnamed Project"));
	g_theWindow = new Window(windowConfig);

	RendererConfig rendererConfig;
	rendererConfig.m_window = g_theWindow;
	g_theRenderer = new Renderer(rendererConfig);

	AudioConfig audioConfig;
	g_theAudio = new AudioSystem(audioConfig);

	DevConsoleConfig devConsoleConfig;
	devConsoleConfig.m_renderer = g_theRenderer;
	g_theDevConsole = new DevConsole(devConsoleConfig);

	EventSystemConfig eventConfig;
	g_theEventSystem = new EventSystem(eventConfig);

	g_theEventSystem->SubscribeEventCallbackFunction("quit", App::Event_Quit);

	g_theInput->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	g_theAudio->Startup();
	g_theDevConsole->Startup();
	g_theFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	m_game = new Game(g_theApp);
}

void App::Update(float& deltaSeconds)
{
	m_game->Update(deltaSeconds);
}

void App::Shutdown()
{
	delete m_game;
	m_game = nullptr;
	g_theDevConsole->Shutdown();
	g_theAudio->Shutdown();
	g_theRenderer->Shutdown();
	g_theWindow->ShutDown();
	g_theInput->Shutdown();
	
	delete g_theEventSystem;
	g_theEventSystem = nullptr;
	delete g_theDevConsole;
	g_theDevConsole = nullptr;
	delete g_theAudio;
	g_theAudio = nullptr;
	delete g_theRenderer;
	g_theRenderer = nullptr;
	delete g_theWindow;
	g_theWindow = nullptr;
	delete g_theInput;
	g_theInput = nullptr;
	
}

bool App::HandleQuitRequested()
{
	m_isQuitting = true;
	return true;
}

void App::BeginFrame()
{
	g_theInput->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	g_theAudio->BeginFrame();
	
	g_theDevConsole->BeginFrame();
	g_theEventSystem->BeginFrame();
	//g_theNetwork->BeginFrame();
}



void App::ResetGame()
{
	m_game->Shutdown();
	m_game->m_isAttractMode = true;
	m_game->~Game();
	m_game = new Game(g_theApp);
}

bool App::Event_Quit(EventArgs& args)
{
	UNUSED(args);
	if (!g_theApp)
	{
		return false;
	}
	g_theApp->HandleQuitRequested();
	return true;
}

// Some simple OpenGL example drawing code.
// This is the graphical equivalent of printing "Hello, world."
void App::Render() const
{
	m_game->Render();
}

void App::EndFrame()
{
	g_theAudio->EndFrame();
	g_theRenderer->EndFrame();
	g_theWindow->EndFrame();
	g_theInput->EndFrame();
}

void App::RunFrame()
{
	float timeNow = static_cast<float>(GetCurrentTimeSeconds());
	float deltaSeconds = timeNow - m_timeLastFrameStart;
	m_timeLastFrameStart = timeNow;

	BeginFrame();
	Update(deltaSeconds);
	Render();
	EndFrame();
}
