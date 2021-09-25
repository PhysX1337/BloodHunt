#include "function.h"
#include "overlay.h"
#include "cfg.h"
#include "Mhyprot/mhyprot.hpp"
#include <intrin.h>
#include <corecrt_math_defines.h>"
#include <vector>


bool ShowMenu = true;
namespace OverlayWindow

{
	WNDCLASSEX WindowClass;
	HWND Hwnd;
	LPCSTR Name;
}

namespace DirectX9Interface
{
	IDirect3D9Ex* Direct3D9 = NULL;
	IDirect3DDevice9Ex* pDevice = NULL;
	D3DPRESENT_PARAMETERS pParams = { NULL };
	MARGINS Margin = { -1 };
	MSG Message = { NULL };
}
typedef struct _EntityList
{
	uintptr_t actor_pawn;
	uintptr_t actor_mesh;
	uintptr_t actor_state;
	uintptr_t actor_root;
	int actor_id;
}EntityList;
std::vector<EntityList> entityList;

auto GameCache()->VOID
{
	while (true)
	{
		std::vector<EntityList> tmpList;

		GameVars.u_world = read<DWORD_PTR>(GameVars.dwProcess_Base + GameOffset.offset_uworld);
		GameVars.game_instance = read<DWORD_PTR>(GameVars.u_world + 0x180); //OwningGameInstance
		GameVars.local_player_array = read<DWORD_PTR>(GameVars.game_instance + 0x38);  // LocalPlayers
		GameVars.local_player = read<DWORD_PTR>(GameVars.local_player_array);
		GameVars.local_player_controller = read<DWORD_PTR>(GameVars.local_player + 0x30); // AcknowledgedPawn
		GameVars.local_player_pawn = read<DWORD_PTR>(GameVars.local_player_controller + 0x2A8); // AcknowledgedPawn
		GameVars.local_player_root = read<DWORD_PTR>(GameVars.local_player_pawn + 0x138); // RootComponent
		GameVars.local_player_state = read<DWORD_PTR>(GameVars.local_player_pawn + 0x230); // PlayerState
		GameVars.persistent_level = read<DWORD_PTR>(GameVars.u_world + 0x30); // PersistentLevel
		GameVars.actor_count = read<int>(GameVars.persistent_level + 0xA0); // MaxPacket
		GameVars.actors = read<DWORD_PTR>(GameVars.persistent_level + 0x98); // OwningActor

		for (int index = 0; index < GameVars.actor_count; ++index)
		{

			auto actor_pawn = read<uintptr_t>(GameVars.actors + index * 0x8);
			if (!actor_pawn)continue;

			auto actor_id = read<int>(actor_pawn + 0x18);
			if (!actor_id)continue;

			auto actor_mesh = read<uintptr_t>(actor_pawn + 0x288); // Mesh
			if (!actor_mesh)continue;

			auto actor_state = read<uintptr_t>(actor_pawn + 0x230); //PlayerState
			if (!actor_state)continue;

			auto actor_root = read<uintptr_t>(actor_pawn + 0x138); // RootComponent
			if (!actor_root)continue;

			auto name = BloodHunt::GetNameFromFName(actor_id);

			if (actor_pawn == GameVars.local_player_pawn) // Ignore Self?
				continue;

			EntityList Entity{ };
			Entity.actor_pawn = actor_pawn;

			if (name == "TBP_ElysiumPlayer_C" || name == "TBP_Player_C" || name == "TBP_NPC_Primogen_C" || name == "TBP_NPC_C")
			{
				if (actor_mesh != NULL)
				{
					Entity.actor_id = actor_id;
					Entity.actor_state = actor_state;
					Entity.actor_root = actor_root;
					Entity.actor_mesh = actor_mesh;
					tmpList.push_back(Entity);
				}
			}
		}
		entityList = tmpList;
		Sleep(2);
	}
}
auto RenderVisual()->VOID
{
	auto EntityList_Copy = entityList;

	for (int index = 0; index < EntityList_Copy.size(); ++index)
	{
		EntityList Entity = EntityList_Copy[index];

		auto local_pos = read<Vector3>(GameVars.local_player_root + 0x11C);
		auto head_pos = BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 28);
		auto bone_pos = BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 0);
		auto BottomBox = BloodHunt::ProjectWorldToScreen(bone_pos);
		auto HeadBox = BloodHunt::ProjectWorldToScreen(Vector3(head_pos.x, head_pos.y, head_pos.z + 15));

		auto entity_distance = local_pos.Distance(head_pos) / 100.f;

		auto CornerHeight = abs(HeadBox.y - BottomBox.y);
		auto CornerWidth = CornerHeight * 0.65;


		if (CFG.b_Visual)
		{

			if (CFG.b_EspBox)
			{
				if (CFG.in_BoxType == 0)
				{
					DrawBox(CFG.BoxColor, HeadBox.x - (CornerWidth / 2), HeadBox.y, CornerWidth, CornerHeight);
				}
				else if (CFG.in_BoxType == 1)
				{
					DrawCorneredBox(HeadBox.x - (CornerWidth / 2), HeadBox.y, CornerWidth, CornerHeight, CFG.BoxColor, 1.5);
				}
			}
			if (CFG.b_EspLine)
			{

				if (CFG.in_LineType == 0)
				{
					DrawLine(ImVec2(static_cast<float>(GameVars.ScreenWidth / 2), static_cast<float>(GameVars.ScreenHeight)), ImVec2(BottomBox.x, BottomBox.y), CFG.LineColor, 1.5f); //LINE FROM BOTTOM SCREEN
				}
				if (CFG.in_LineType == 1)
				{
					DrawLine(ImVec2(static_cast<float>(GameVars.ScreenWidth / 2), 0.f), ImVec2(BottomBox.x, BottomBox.y), CFG.LineColor, 1.5f); //LINE FROM TOP SCREEN
				}
				if (CFG.in_LineType == 2)
				{
					DrawLine(ImVec2(static_cast<float>(GameVars.ScreenWidth / 2), static_cast<float>(GameVars.ScreenHeight / 2)), ImVec2(BottomBox.x, BottomBox.y), CFG.LineColor, 1.5f); //LINE FROM CROSSHAIR
				}
			}
			if (CFG.b_EspDistance)
			{
				char dist[64];
				sprintf_s(dist, "Dist:%.fm", entity_distance);
				DrawOutlinedText(Verdana, dist, ImVec2(BottomBox.x, BottomBox.y), 14.0f, ImColor(255, 255, 255), true);
			}
			if (CFG.b_EspHealth)
			{
				auto Health = read<float>(Entity.actor_pawn + 0x688);
				auto MaxHealth = read<float>(Entity.actor_pawn + 0x56C); // idk how this works lmao
				auto procentage = Health * 100 / MaxHealth;

				float width = CornerWidth / 10;
				if (width < 2.f) width = 2.;
				if (width > 3) width = 3.;

				HealthBar(HeadBox.x - (CornerWidth / 2) - 7, HeadBox.y, width, BottomBox.y - HeadBox.y, procentage, true);
			}
			if (CFG.b_EspSkeleton)
			{

				Vector3 vHeadOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 28));
				Vector3 vNeckOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 27));
				Vector3 vSpine1Out = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 27));
				Vector3 vSpine2Out = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 25));
				Vector3 vSpine3Out = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 24));
				Vector3 vPelvisOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 9));
				Vector3 vRightThighOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 17));
				Vector3 vLeftThighOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 10));
				Vector3 vRightCalfOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 18));
				Vector3 vLeftCalfOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 11));
				Vector3 vRightFootOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 19));
				Vector3 vLeftFootOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 12));
				Vector3 vUpperArmRightOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 109));
				Vector3 vUpperArmLeftOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 64));
				Vector3 vLowerArmRightOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 110));
				Vector3 vLowerArmLeftOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 65));
				Vector3 vHandRightOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 111));
				Vector3 vHandLeftOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 66));
				Vector3 vFootOutOut = BloodHunt::ProjectWorldToScreen(BloodHunt::GetBoneWithRotation(Entity.actor_mesh, 0));


				DrawLine(ImVec2(vHeadOut.x, vHeadOut.y), ImVec2(vNeckOut.x, vNeckOut.y), CFG.SkeletonColor, 1.5f);
				DrawLine(ImVec2(vNeckOut.x, vNeckOut.y), ImVec2(vSpine1Out.x, vSpine1Out.y), CFG.SkeletonColor, 1.5f);
				DrawLine(ImVec2(vSpine1Out.x, vSpine1Out.y), ImVec2(vSpine2Out.x, vSpine2Out.y), CFG.SkeletonColor, 1.5f);
				DrawLine(ImVec2(vSpine2Out.x, vSpine2Out.y), ImVec2(vSpine3Out.x, vSpine3Out.y), CFG.SkeletonColor, 1.5f);

				DrawLine(ImVec2(vSpine1Out.x, vSpine1Out.y), ImVec2(vUpperArmRightOut.x, vUpperArmRightOut.y), CFG.SkeletonColor, 1.5f);
				DrawLine(ImVec2(vSpine1Out.x, vSpine1Out.y), ImVec2(vUpperArmLeftOut.x, vUpperArmLeftOut.y), CFG.SkeletonColor, 1.5f);

				DrawLine(ImVec2(vSpine1Out.x, vSpine1Out.y), ImVec2(vPelvisOut.x, vPelvisOut.y), CFG.SkeletonColor, 1.5f);

				DrawLine(ImVec2(vPelvisOut.x, vPelvisOut.y), ImVec2(vRightThighOut.x, vRightThighOut.y), CFG.SkeletonColor, 1.5f);
				DrawLine(ImVec2(vPelvisOut.x, vPelvisOut.y), ImVec2(vLeftThighOut.x, vLeftThighOut.y), CFG.SkeletonColor, 1.5f);

				DrawLine(ImVec2(vRightThighOut.x, vRightThighOut.y), ImVec2(vRightCalfOut.x, vRightCalfOut.y), CFG.SkeletonColor, 1.5f);
				DrawLine(ImVec2(vLeftThighOut.x, vLeftThighOut.y), ImVec2(vLeftCalfOut.x, vLeftCalfOut.y), CFG.SkeletonColor, 1.5f);

				DrawLine(ImVec2(vRightCalfOut.x, vRightCalfOut.y), ImVec2(vRightFootOut.x, vRightFootOut.y), CFG.SkeletonColor, 1.5f);
				DrawLine(ImVec2(vLeftCalfOut.x, vLeftCalfOut.y), ImVec2(vLeftFootOut.x, vLeftFootOut.y), CFG.SkeletonColor, 1.5f);

				DrawLine(ImVec2(vUpperArmRightOut.x, vUpperArmRightOut.y), ImVec2(vLowerArmRightOut.x, vLowerArmRightOut.y), CFG.SkeletonColor, 1.5f);
				DrawLine(ImVec2(vUpperArmLeftOut.x, vUpperArmLeftOut.y), ImVec2(vLowerArmLeftOut.x, vLowerArmLeftOut.y), CFG.SkeletonColor, 1.5f);

				DrawLine(ImVec2(vLowerArmLeftOut.x, vLowerArmLeftOut.y), ImVec2(vHandLeftOut.x, vHandLeftOut.y), CFG.SkeletonColor, 1.5f);
				DrawLine(ImVec2(vLowerArmRightOut.x, vLowerArmRightOut.y), ImVec2(vHandRightOut.x, vHandRightOut.y), CFG.SkeletonColor, 1.5f);

			}
		}

	}
}

void InputHandler() {
	for (int i = 0; i < 5; i++) ImGui::GetIO().MouseDown[i] = false;
	int button = -1;
	if (GetAsyncKeyState(VK_LBUTTON)) button = 0;
	if (button != -1) ImGui::GetIO().MouseDown[button] = true;
}
void Render()
{
	if (GetAsyncKeyState(VK_INSERT) & 1)
		ShowMenu = !ShowMenu;

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	RenderVisual();
	ImGui::GetIO().MouseDrawCursor = ShowMenu;

	if (ShowMenu == true)
	{
		InputHandler();
		ImGui::SetNextWindowSize(ImVec2(675, 410)); // 450,426
		ImGui::PushFont(DefaultFont);
		ImGui::Begin("Jenrix BloodHunt || jenrix#8201", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

		TabButton("Visual", &CFG.in_tab_index, 0, false);
		if (CFG.in_tab_index == 0)
		{
			ImGui::Checkbox("Enabled Visual", &CFG.b_Visual);
			if (CFG.b_Visual)
			{
				ImGui::Checkbox("ESP Box", &CFG.b_EspBox);
				if (CFG.b_EspBox)
				{
					ImGui::Combo("ESP Box Type", &CFG.in_BoxType, CFG.BoxTypes, 2);
					if (ImGui::ColorEdit3("ESP Box Color", CFG.fl_BoxColor, ImGuiColorEditFlags_NoDragDrop))
					{
						CFG.BoxColor = ImColor(CFG.fl_BoxColor[0], CFG.fl_BoxColor[1], CFG.fl_BoxColor[2]);
					}
				}
				ImGui::Checkbox("ESP Skeleton", &CFG.b_EspSkeleton);
				if (CFG.b_EspSkeleton)
				{
					if (ImGui::ColorEdit3("ESP Skeleton Color", CFG.fl_SkeletonColor, ImGuiColorEditFlags_NoDragDrop))
					{
						CFG.SkeletonColor = ImColor(CFG.fl_SkeletonColor[0], CFG.fl_SkeletonColor[1], CFG.fl_SkeletonColor[2]);
					}
				}
				ImGui::Checkbox("ESP Line", &CFG.b_EspLine);
				if (CFG.b_EspLine)
				{
					ImGui::Combo("ESP Line Type", &CFG.in_LineType, CFG.LineTypes, 3);
					if (ImGui::ColorEdit3("ESP Line Color", CFG.fl_LineColor, ImGuiColorEditFlags_NoDragDrop))
					{
						CFG.LineColor = ImColor(CFG.fl_LineColor[0], CFG.fl_LineColor[1], CFG.fl_LineColor[2]);
					}
				}
				ImGui::Checkbox("ESP Distance", &CFG.b_EspDistance);
				ImGui::Checkbox("ESP HealthBar", &CFG.b_EspHealth);
			}
		}
		ImGui::PopFont();
		ImGui::End();
	}
	ImGui::EndFrame();

	DirectX9Interface::pDevice->SetRenderState(D3DRS_ZENABLE, false);
	DirectX9Interface::pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	DirectX9Interface::pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);

	DirectX9Interface::pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	if (DirectX9Interface::pDevice->BeginScene() >= 0) {
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		DirectX9Interface::pDevice->EndScene();
	}

	HRESULT result = DirectX9Interface::pDevice->Present(NULL, NULL, NULL, NULL);
	if (result == D3DERR_DEVICELOST && DirectX9Interface::pDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
		DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
}

void MainLoop() {
	static RECT OldRect;
	ZeroMemory(&DirectX9Interface::Message, sizeof(MSG));

	while (DirectX9Interface::Message.message != WM_QUIT) {
		if (PeekMessage(&DirectX9Interface::Message, OverlayWindow::Hwnd, 0, 0, PM_REMOVE)) {
			TranslateMessage(&DirectX9Interface::Message);
			DispatchMessage(&DirectX9Interface::Message);
		}
		HWND ForegroundWindow = GetForegroundWindow();
		if (ForegroundWindow == GameVars.gameHWND) {
			HWND TempProcessHwnd = GetWindow(ForegroundWindow, GW_HWNDPREV);
			SetWindowPos(OverlayWindow::Hwnd, TempProcessHwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}

		RECT TempRect;
		POINT TempPoint;
		ZeroMemory(&TempRect, sizeof(RECT));
		ZeroMemory(&TempPoint, sizeof(POINT));

		GetClientRect(GameVars.gameHWND, &TempRect);
		ClientToScreen(GameVars.gameHWND, &TempPoint);

		TempRect.left = TempPoint.x;
		TempRect.top = TempPoint.y;
		ImGuiIO& io = ImGui::GetIO();
		io.ImeWindowHandle = GameVars.gameHWND;

		POINT TempPoint2;
		GetCursorPos(&TempPoint2);
		io.MousePos.x = TempPoint2.x - TempPoint.x;
		io.MousePos.y = TempPoint2.y - TempPoint.y;

		if (GetAsyncKeyState(0x1)) {
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
			io.MouseClickedPos[0].x = io.MousePos.x;
			io.MouseClickedPos[0].x = io.MousePos.y;
		}
		else {
			io.MouseDown[0] = false;
		}

		if (TempRect.left != OldRect.left || TempRect.right != OldRect.right || TempRect.top != OldRect.top || TempRect.bottom != OldRect.bottom) {
			OldRect = TempRect;
			GameVars.ScreenWidth = TempRect.right;
			GameVars.ScreenHeight = TempRect.bottom;
			DirectX9Interface::pParams.BackBufferWidth = GameVars.ScreenWidth;
			DirectX9Interface::pParams.BackBufferHeight = GameVars.ScreenHeight;
			SetWindowPos(OverlayWindow::Hwnd, (HWND)0, TempPoint.x, TempPoint.y, GameVars.ScreenWidth, GameVars.ScreenHeight, SWP_NOREDRAW);
			DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
		}
		Render();
	}
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	if (DirectX9Interface::pDevice != NULL) {
		DirectX9Interface::pDevice->EndScene();
		DirectX9Interface::pDevice->Release();
	}
	if (DirectX9Interface::Direct3D9 != NULL) {
		DirectX9Interface::Direct3D9->Release();
	}
	DestroyWindow(OverlayWindow::Hwnd);
	UnregisterClass(OverlayWindow::WindowClass.lpszClassName, OverlayWindow::WindowClass.hInstance);
}

bool DirectXInit() {
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &DirectX9Interface::Direct3D9))) {
		return false;
	}

	D3DPRESENT_PARAMETERS Params = { 0 };
	Params.Windowed = TRUE;
	Params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	Params.hDeviceWindow = OverlayWindow::Hwnd;
	Params.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	Params.BackBufferFormat = D3DFMT_A8R8G8B8;
	Params.BackBufferWidth = GameVars.ScreenWidth;
	Params.BackBufferHeight = GameVars.ScreenHeight;
	Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	Params.EnableAutoDepthStencil = TRUE;
	Params.AutoDepthStencilFormat = D3DFMT_D16;
	Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	Params.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	if (FAILED(DirectX9Interface::Direct3D9->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, OverlayWindow::Hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &Params, 0, &DirectX9Interface::pDevice))) {
		DirectX9Interface::Direct3D9->Release();
		return false;
	}

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui_ImplWin32_Init(OverlayWindow::Hwnd);
	ImGui_ImplDX9_Init(DirectX9Interface::pDevice);
	DirectX9Interface::Direct3D9->Release();
	return true;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, Message, wParam, lParam))
		return true;

	switch (Message) {
	case WM_DESTROY:
		if (DirectX9Interface::pDevice != NULL) {
			DirectX9Interface::pDevice->EndScene();
			DirectX9Interface::pDevice->Release();
		}
		if (DirectX9Interface::Direct3D9 != NULL) {
			DirectX9Interface::Direct3D9->Release();
		}
		PostQuitMessage(0);
		exit(4);
		break;
	case WM_SIZE:
		if (DirectX9Interface::pDevice != NULL && wParam != SIZE_MINIMIZED) {
			ImGui_ImplDX9_InvalidateDeviceObjects();
			DirectX9Interface::pParams.BackBufferWidth = LOWORD(lParam);
			DirectX9Interface::pParams.BackBufferHeight = HIWORD(lParam);
			HRESULT hr = DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
			if (hr == D3DERR_INVALIDCALL)
				IM_ASSERT(0);
			ImGui_ImplDX9_CreateDeviceObjects();
		}
		break;
	default:
		return DefWindowProc(hWnd, Message, wParam, lParam);
		break;
	}
	return 0;
}

void SetupWindow() {
	OverlayWindow::WindowClass = {
		sizeof(WNDCLASSEX), 0, WinProc, 0, 0, nullptr, LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, OverlayWindow::Name, LoadIcon(nullptr, IDI_APPLICATION)
	};

	RegisterClassEx(&OverlayWindow::WindowClass);
	if (GameVars.gameHWND) {
		static RECT TempRect = { NULL };
		static POINT TempPoint;
		GetClientRect(GameVars.gameHWND, &TempRect);
		ClientToScreen(GameVars.gameHWND, &TempPoint);
		TempRect.left = TempPoint.x;
		TempRect.top = TempPoint.y;
		GameVars.ScreenWidth = TempRect.right;
		GameVars.ScreenHeight = TempRect.bottom;
	}

	OverlayWindow::Hwnd = CreateWindowEx(NULL, OverlayWindow::Name, OverlayWindow::Name, WS_POPUP | WS_VISIBLE, GameVars.ScreenLeft, GameVars.ScreenTop, GameVars.ScreenWidth, GameVars.ScreenHeight, NULL, NULL, 0, NULL);
	DwmExtendFrameIntoClientArea(OverlayWindow::Hwnd, &DirectX9Interface::Margin);
	SetWindowLong(OverlayWindow::Hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
	ShowWindow(OverlayWindow::Hwnd, SW_SHOW);
	UpdateWindow(OverlayWindow::Hwnd);
}

bool CreateConsole = true;

int main()
{
	system("sc stop mhyprot2"); // RELOAD DRIVER JUST IN CASE
	system("CLS"); // CLEAR

	GameVars.dwProcessId = GetProcessId(GameVars.dwProcessName);
	if (!GameVars.dwProcessId)
	{
		printf("[!] process \"%s\ was not found\n", GameVars.dwProcessName);
	}
	if (!mhyprot::init())
	{
		printf("[!] failed to initialize vulnerable driver\n");
		return -1;
	}

	if (!mhyprot::driver_impl::driver_init(
		false, // print debug
		false // print seedmap
	))
	{
		printf("[!] failed to initialize driver properly\n");
		mhyprot::unload();
		return -1;
	}
	GameVars.dwProcess_Base = GetProcessBase(GameVars.dwProcessId);
	if (!GameVars.dwProcess_Base)
	{
		printf("[!] failed to get baseadress\n");
	}

	printf("[+] ProcessName: %s ID: (%d) base: %llx\n", GameVars.dwProcessName, GameVars.dwProcessId, GameVars.dwProcess_Base);

	CreateThread(nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(GameCache), nullptr, NULL, nullptr);


	if (CreateConsole == false)
		ShowWindow(GetConsoleWindow(), SW_HIDE);

	bool WindowFocus = false;
	while (WindowFocus == false)
	{
		DWORD ForegroundWindowProcessID;
		GetWindowThreadProcessId(GetForegroundWindow(), &ForegroundWindowProcessID);
		if (GetProcessId(GameVars.dwProcessName) == ForegroundWindowProcessID)
		{

			GameVars.gameHWND = GetForegroundWindow();

			RECT TempRect;
			GetWindowRect(GameVars.gameHWND, &TempRect);
			GameVars.ScreenWidth = TempRect.right - TempRect.left;
			GameVars.ScreenHeight = TempRect.bottom - TempRect.top;
			GameVars.ScreenLeft = TempRect.left;
			GameVars.ScreenRight = TempRect.right;
			GameVars.ScreenTop = TempRect.top;
			GameVars.ScreenBottom = TempRect.bottom;

			WindowFocus = true;
		}
	}

	OverlayWindow::Name = RandomString(10).c_str();
	SetupWindow();
	DirectXInit();

	ImGuiIO& io = ImGui::GetIO();
	DefaultFont = io.Fonts->AddFontDefault();
	Verdana = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Verdana.ttf", 16.0f);
	io.Fonts->Build();


	while (TRUE)
	{
		MainLoop();
	}

}
