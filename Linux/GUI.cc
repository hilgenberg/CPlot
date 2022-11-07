#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include "PlotWindow.h"

GUI::GUI(SDL_Window* window, SDL_GLContext context, PlotWindow &w)
: w(w)
, visible(false)
, need_redraw(1)
, enabled(true)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigInputTextCursorBlink = false;
	io.ConfigInputTrickleEventQueue = false;
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	ImGui_ImplSDL2_InitForOpenGL(window, context);
	ImGui_ImplOpenGL2_Init();

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);
}

GUI::~GUI()
{
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

bool GUI::handle_event(const SDL_Event &event)
{
	if (!visible) return false;
	if (ImGui_ImplSDL2_ProcessEvent(&event)) redraw();
	ImGuiIO &io = ImGui::GetIO();

	switch (event.type)
	{
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			if (io.WantCaptureKeyboard) return true;
			break;

		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
			if (io.WantCaptureMouse) return true;
			break;
	}
	return false;
}

void GUI::update()
{
	if (ImGui::GetIO().MouseDrawCursor != visible)
	{
		ImGui::GetIO().MouseDrawCursor = visible;

		if (!visible)
		{
			ImGui_ImplOpenGL2_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			ImGui::Render();
			ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
			ImGui::EndFrame();

			return;
		}
	}
	else if (!visible) return;

	bool dark = 
		w.plot.axis_type() == Axis::Invalid ||
		w.plot.axis.options.background_color.lightness() < 0.55;
	if (dark) ImGui::StyleColorsDark(); else ImGui::StyleColorsLight();
	/*ImVec4* colors = ImGui::GetStyle().Colors;
	float w0 = colors[ImGuiCol_WindowBg].w;
	colors[ImGuiCol_WindowBg].w = 0.7f;*/

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	ImGui::GetStyle().FrameBorderSize = dark ? 0.0f : 1.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	main_menu();
	main_panel_height = 0.0f;
	if (show_main_panel) main_panel();
	if (show_settings_panel) settings_panel();
	ImGui::PopStyleVar(); // draw other windows with border
	//colors[ImGuiCol_WindowBg].w = w0;

	if (param_edit) param_editor();
	if (def_edit)   def_editor();
	error_panel();
	confirmation_panel();

	if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

	//ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);

	//ImGui::Text("counter = %d", counter);
	//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
}

void GUI::draw()
{
	if (!visible) return;
	ImGui::Render();
	//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	if (need_redraw > 0) --need_redraw;
}

void GUI::enable(bool e)
{
	if (e) {
		if (!enabled) ImGui::EndDisabled();
	} else {
		if (enabled) ImGui::BeginDisabled();
	}
	enabled = e;
}

void GUI::error(const std::string &msg)
{
	error_msg = msg;
	redraw();
	// cannot call OpenPopup here! would cause the IDs to mismatch.
}

void GUI::error_panel()
{
	static bool showing = false;
	if (error_msg.empty()) return;
	if (!showing)
	{
		showing = true;
		ImGui::OpenPopup("Error");
		need_redraw = 20; // imgui wants to animate dimming the background
	}
	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (!ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) return;
	ImGui::TextWrapped(error_msg.c_str());
	ImGui::Separator();
	if (ImGui::Button("OK", ImVec2(120, 0)))
	{
		error_msg.clear();
		ImGui::CloseCurrentPopup();
		showing = false;
	}
	ImGui::EndPopup();
}

void GUI::confirm(bool c, const std::string &msg, std::function<void(void)> action)
{
	assert(action);
	if (!c)
	{
		try
		{
			action();
		}
		catch (std::exception &e)
		{
			error(e.what());
		}
		return;
	}
	confirm_msg = msg;
	confirm_action = action;
}
void GUI::confirmation_panel()
{
	static bool showing = false;
	if (!confirm_action) return;
	if (!showing)
	{
		showing = true;
		ImGui::OpenPopup("Confirmation");
		need_redraw = 20; // imgui wants to animate dimming the background
	}
	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (!ImGui::BeginPopupModal("Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) return;
	ImGui::TextWrapped(confirm_msg.c_str());
	ImGui::Separator();
	bool close = false;
	if (ImGui::Button("Cancel", ImVec2(120, 0))) close = true;
	ImGui::SameLine();
	if (ImGui::Button("OK", ImVec2(120, 0)))
	{
		try
		{
			confirm_action();
		}
		catch (std::exception &e)
		{
			error(e.what());
		}
		close = true;
	}
	if (close)
	{
		confirm_msg.clear();
		confirm_action = nullptr;
		ImGui::CloseCurrentPopup();
		showing = false;
	}
	ImGui::EndPopup();
}
