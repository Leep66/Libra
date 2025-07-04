#include <Engine/Math/Vec2.hpp>
#include <Engine/Core/Rgba8.hpp>
#include <Engine/Renderer/Camera.hpp>
#include <Engine/Core/Vertex_PCU.hpp>
#include "Game/Game.hpp"

class App 
{
public:
	App();
	~App();
	void Startup();
	void Shutdown();
	void RunFrame();

	bool HandleQuitRequested();
	bool IsQuitting() const { return m_isQuitting; }
	
	void ResetGame();

	static bool Event_Quit(EventArgs& args);


private:
	void BeginFrame();
	void Update(float& deltaSeconds);
	void Render() const;
	void EndFrame();
	
	

private:
	Game* m_game = nullptr;
	bool m_isQuitting			= false;

};