#include "App.h"
//#include "Melon.h"
//#include "Pyramid.h"
#include "Box.h"
#include "Cylinder.h"
#include "Pyramid.h"
//#include "Sheet.h"
#include "SkinnedBox.h"
#include "AssimpTest.h"
#include <memory>
#include <algorithm>
#include "Math.h"
#include "Surface.h"
#include "GDIPlusManager.h"
#include "imgui/imgui.h"


GDIPlusManager gdipm;


App::App()
	:
	window(800, 600, "DirectX Practice Window"),
	light(window.Gfx()) {

	class Factory {
	public:
		Factory( Graphics& gfx )
			:
			gfx( gfx )
		{}

		std::unique_ptr<Drawable> operator()() {
			const DirectX::XMFLOAT3 mat = { cdist( rng ),cdist( rng ),cdist( rng ) };
			switch( sdist( rng ) )
			{
			case 0:
				return std::make_unique<Box>(
					gfx,rng,adist,ddist,
					odist,rdist,bdist,mat
				);
			case 1:
				return std::make_unique<Cylinder>(
					gfx,rng,adist,ddist,odist,
					rdist,bdist,tdist
				);
			case 2:
				return std::make_unique<Pyramid>(
					gfx,rng,adist,ddist,odist,
					rdist,tdist
				);
			case 3:
				return std::make_unique<SkinnedBox>(
					gfx,rng,adist,ddist,
					odist,rdist
				);
			case 4:
				return std::make_unique<AssimpTest>(
					gfx,rng,adist,ddist,
					odist,rdist,mat,1.5f
				);
			default:
				assert( false && "impossible drawable option in factory" );
				return {};
			}
		}

	private:
		Graphics& gfx;
		std::mt19937 rng{ std::random_device{}() };
		std::uniform_int_distribution<int> sdist{ 0,4 };
		std::uniform_real_distribution<float> adist{0.0f, PI * 2.0f};
		std::uniform_real_distribution<float> ddist{0.0f, PI * 0.5f};
		std::uniform_real_distribution<float> odist{0.0f, PI * 0.08f};
		std::uniform_real_distribution<float> rdist{6.0f, 20.0f};
		std::uniform_real_distribution<float> bdist{0.4f, 3.0f};
		std::uniform_real_distribution<float> cdist{ 0.0f,1.0f };
		std::uniform_int_distribution<int> tdist{ 3,30 };
	};

	drawables.reserve( nDrawables );
	std::generate_n(std::back_inserter(drawables), nDrawables, Factory{ window.Gfx() });

	// init box pointers for editing instance parameters
	for (auto& pd : drawables) {
		if (auto pb = dynamic_cast<Box*>(pd.get())) {
			boxes.push_back( pb );
		}
	}

	window.Gfx().SetProjection( DirectX::XMMatrixPerspectiveLH( 1.0f,3.0f / 4.0f,0.5f,40.0f ) );
}

App::~App() {

}

int App::Run() {
	while(1) {
		// process all messages pending, but to not block for new messages
		if (const auto ecode = Window::ProcessMessages()) {
			// if return optional has value, means we're quitting so return exit code
			return *ecode;
		}

		DoFrame();
	}
}

void App::DoFrame() {
	const auto dt = timer.Mark() * speed_factor;
	window.Gfx().BeginFrame(0.07f, 0.0f, 0.12f);
	window.Gfx().SetCamera(cam.GetMatrix());
	light.Bind(window.Gfx(), cam.GetMatrix());

	// render geometry
	for (auto& d : drawables) {
		d->Update(window.keyboard.KeyIsPressed(VK_SPACE) ? 0.0f : dt);
		d->Draw(window.Gfx());
	}
	light.Draw(window.Gfx());

	// imgui windows
	SpawnSimulationWindow();
	cam.SpawnControlWindow();
	light.SpawnControlWindow();
	SpawnBoxWindowManagerWindow();
	SpawnBoxWindows();

	// present
	window.Gfx().EndFrame();
}

void App::SpawnSimulationWindow() noexcept {
	// imgui window to control simulation speed
	if (ImGui::Begin("Simulation Speed")) {
		ImGui::SliderFloat("Speed Factor", &speed_factor, 0.0f, 6.0f, "%.4f", 3.2f);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",1000.0f / ImGui::GetIO().Framerate,ImGui::GetIO().Framerate);
		ImGui::Text("Status: %s", window.keyboard.KeyIsPressed(VK_SPACE) ? "PAUSED" : "RUNNING");
	}
	ImGui::End();
}

void App::SpawnBoxWindowManagerWindow() noexcept {
	// imgui window to open box windows
	if (ImGui::Begin("Boxes")) {
		using namespace std::string_literals;
		const auto preview = comboBoxIndex ? std::to_string(*comboBoxIndex) : "Choose a box..."s;
		if (ImGui::BeginCombo("Box Number", preview.c_str())) {
			for (int i = 0; i < boxes.size(); i++) {
				const bool selected = comboBoxIndex && *comboBoxIndex == i;
				if (ImGui::Selectable(std::to_string(i).c_str(), selected)) {
					comboBoxIndex = i;
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("Spawn Control Window") && comboBoxIndex) {
			boxControlIds.insert(*comboBoxIndex);
			comboBoxIndex.reset();
		}
	}
	ImGui::End();
}

void App::SpawnBoxWindows() noexcept {
	// imgui box attribute control windows
	for (auto i = boxControlIds.begin(); i != boxControlIds.end();) {
		if (!boxes[*i]->SpawnControlWindow(*i, window.Gfx())) {
			i = boxControlIds.erase(i);
		} else {
			i++;
		}
	}
}