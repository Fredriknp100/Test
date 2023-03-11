#include "gui.h"


#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"




extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter

);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)

{

	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter); // set click points
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);

		}

	}return 0;

	}
	
	return DefWindowProcW(window, message, wideParameter, longParameter);
}


void gui::CreateHWindow(
	const char* windowName,
	const char* className) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEXA);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = className;
	windowClass.hIconSm = 0;

	RegisterClassExA(&windowClass);

	window = CreateWindowA(
		className,
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
	
}


void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

}


bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)

	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}

}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);

}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	// Start the ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);


	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();

	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();

}

void gui::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });



	// Create a new font
	ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF("Areal", 18.0f);
	// Push the new font
	ImGui::PushFont(font);
	ImGui::GetStyle().AntiAliasedLines;
	
	
	
	
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
	// Making main window
	ImGui::Begin(
		"OX BETA",
		&exit,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove);

	
	

	// Making all widgets in the menu have round cornors
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	

	// Coloring all buttons in the menu
	ImVec4 border_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	
	

	
	if (ImGui::BeginTabBar("Main"))
		ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(1.0f, 0.0f, 0.863f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.529f, 0.212f, 0.965f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(1.0f, 0.0f, 0.863f, 1.0f));
	{
		if (ImGui::BeginTabItem("Myself"))
		{
			if (ImGui::BeginTabBar("Tabs"))
				ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(1.0f, 0.0f, 0.863f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.529f, 0.212f, 0.965f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(1.0f, 0.0f, 0.863f, 1.0f));
			
				if (ImGui::BeginTabItem("Player"))
				{
					
					ImGui::Text("Modifiers");
					ImGui::Button("Heal");
					{

					}
					ImGui::SameLine();
					ImGui::Button("Armor");
					{

					}
					ImGui::SameLine();
					ImGui::Button("Revive");
					{

					}


					static bool toogle_godmode = false;
					ImGui::Checkbox("God Mode", &toogle_godmode);
					static bool toogle_infinitestamina = false;
					ImGui::Checkbox("Infinite Stamina", &toogle_infinitestamina);
					static bool toogle_invisibility = false;
					ImGui::Checkbox("Invisibility", &toogle_invisibility);
					static bool toogle_noragdoll = false;
					ImGui::Checkbox("No Ragdoll", &toogle_noragdoll);
					
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Teleport"))
				{
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Noclip"))
				{
					ImGui::EndTabItem();
				}


			ImGui::EndTabBar();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Online"))
		{
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Weapon"))
		{

			if (ImGui::BeginTabBar("Weapon"))
			{
				if (ImGui::BeginTabItem("Spawn"))
				{
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Cheats"))
				{
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Aimbot"))
				{

					static bool aimbot_active = false;

					ImGui::Text("Aimbot");
					ImGui::Checkbox("Enable", &aimbot_active);
					if (aimbot_active)
					{
						ImGui::SetNextItemWidth(170);
						ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 5.0f);
						static int aimbot_smooth = 0;
						ImGui::SliderInt("Smootning", &aimbot_smooth, 0, 100);

						// Lager combobox alternativer
						const char* items[] = { "Head", "Neck", "Kelvis", "Right Arm", "Left Arm", "Right Leg", "Left Leg" };
						int current_item = 0;

						// Gjør combobox bredde x
						ImGui::SetNextItemWidth(170);

						// Making a combobox for aimbot hit position
						if (ImGui::Combo("Menu", &current_item, items, IM_ARRAYSIZE(items)));
						{

						}
						
						
					}
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();


					static bool fov = false;
					static int size_fov = 30;
					
					ImGui::Checkbox("FOV", &fov);
					ImGui::SliderInt("FOV Size", &size_fov, 5, 100);

					ImVec2 windowSize = ImGui::GetWindowSize();
					float middleX = windowSize.x / 2;
					float middleY = windowSize.y / 2;

					if (fov)
					{
						// Draw circle outside window
						ImVec2 center = ImVec2(middleX, middleY); // x, y are the coordinates for the center of the circle
						float radius = 20.0f; // radius of the circle in pixels
						ImGui::GetWindowDrawList()->AddCircle(center, size_fov, ImColor(255, 255, 255));

					}

					
					ImGui::EndTabItem();
				}

			}

			ImGui::EndTabBar();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Vehicle"))
		{
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Visuals"))
		{
			constexpr auto name = "ESP SETTINGS";
			ImGui::Text(name);

			ImGui::SetNextItemWidth(170);
			static auto esp_dis = 0;
			ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 5.0f);
			ImGui::SliderInt("Distance", &esp_dis, 0, 1000);
			
			
			static float color_bone[] = { 0.0f / 255.0f, 8.0f / 255.0f, 255.0f / 255.0f, 1.0f };
			static float color_box[] = { 38.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 1.0f };
			static float color_health[] = { 255.0f / 255.0f, 203.0f / 255.0f, 0.0f / 255.0f, 1.0f };
			static float color_name[] = { 255.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 1.0f };
			static float color_line[] = { 240.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f, 1.0f };

			ImGuiColorEditFlags flag = ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoLabel;


			// Checkboxes for esp options
			
			static bool toogle_esp = false;
			ImGui::Checkbox("Enable ESP", &toogle_esp);

			static bool toogle_bone = false;
			ImGui::Checkbox("Bone", &toogle_bone);
			ImGui::SameLine();
			ImGui::ColorEdit3("Bone", color_bone, flag);

			static bool toogle_box = false;
			ImGui::Checkbox("Box", &toogle_box);
			ImGui::SameLine();
			ImGui::ColorEdit3("Box", color_box, flag);

			static bool toogle_health = false;
			ImGui::Checkbox("Health", &toogle_health);
			ImGui::SameLine();
			ImGui::ColorEdit3("Health", color_health, flag);
			
			static bool toogle_name = false;
			ImGui::Checkbox("Name", &toogle_name);
			ImGui::SameLine();
			ImGui::ColorEdit3("Name", color_name, flag);

			static bool toogle_line = false;
			ImGui::Checkbox("Line", &toogle_line);
			ImGui::SameLine();
			ImGui::ColorEdit3("Line", color_line, flag);

		

			ImGui::EndTabItem();

		}
		if (ImGui::BeginTabItem("Executor"))
		{	
			ImGui::Text("Lua Executor");
			// Making input field for execution code
			char myTextBuffer[200];
			ImGui::InputTextMultiline("", myTextBuffer,
			IM_ARRAYSIZE(myTextBuffer), ImVec2(485, 200), ImGuiInputTextFlags_None);

			ImGui::Button("Execute");
			{

			}
			ImGui::SameLine();
			ImGui::Button(".Load");
			{

			}


			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Config"))
		{	

			
			

			// Makes child window round in courners
			ImGuiStyle& style = ImGui::GetStyle();
			style.ChildRounding = 5.0f;
			style.FrameRounding = 5.0f;

			// Create the first child window
			ImGui::BeginChild("Child1", ImVec2(238, 200), true);
			ImGui::Text("This is the first child window");


			// Ending child window 1
			ImGui::EndChild();

			ImGui::SameLine();

			// Create the second child window
			ImGui::BeginChild("Child2", ImVec2(238, 200), true);
			
			ImVec2 size = ImGui::GetWindowSize();
			ImVec2 pos = ImGui::GetWindowPos();
			ImGui::SetCursorPos({ pos.x + size.x / 2.0f, pos.y });
			ImGui::Text("MISC");

			// Color edit for config tab(Changes the menu color)
			static float color_menu[4] = { 87.0f / 255.0f, 6.0f / 255.0f, 196.0f / 255.0f, 1.0f };

			ImGuiColorEditFlags flag = ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoOptions;
			ImGui::ColorEdit3("Menu Color", color_menu, flag);
			
		

			// Ending child window 2
			ImGui::EndChild();


			ImGui::EndTabItem();
		}

			
	}
		
	ImGui::EndTabBar();
	
	
	ImGui::PopClipRect();
	ImGui::PopStyleVar();
	ImGui::End();

	
	
}

