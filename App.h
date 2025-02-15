#pragma once

#include "Window.h"
#include "Timer.h"
#include "ImguiManager.h"
#include "Camera.h"
#include "PointLight.h"
#include <set>


class App {
public:
	App();
	~App();
	int Run();

private:
	void DoFrame();
	void SpawnSimulationWindow() noexcept;
	void SpawnBoxWindowManagerWindow() noexcept;
	void SpawnBoxWindows() noexcept;

private:
	Timer timer;
	ImguiManager imgui;
	Window window;
	std::vector<std::unique_ptr<class Drawable>> drawables;
	std::vector<class Box*> boxes;
	float speed_factor = 1.0f;
	Camera cam;
	PointLight light;
	static constexpr size_t nDrawables = 180;
	std::optional<int> comboBoxIndex;
	std::set<int> boxControlIds;
};
