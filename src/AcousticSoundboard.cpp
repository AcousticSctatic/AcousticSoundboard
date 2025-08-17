#include "AcousticSoundboard.h"
#include <cderr.h>
#include <dxgi.h>
#include <dxgiformat.h>
#include <windef.h>
#include <winerror.h>
#include <corecrt.h>
#include <cstring>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <combaseapi.h>
#include <commdlg.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <libloaderapi.h>
#include <minwinbase.h>
#include <PathCch.h>
#include <processenv.h>
#include <stringapiset.h>
#include <synchapi.h>
#include <Windows.h>
#include <WinNls.h>
#include <cstdint>
#include <sal.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <miniaudio.h>
#include <sqlite3.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_  LPSTR lpCmdLine, _In_  int nCmdShow)
{
	GetModuleFileNameW(NULL, StartingDirectory.string, StartingDirectory.numWideChars);
	ImGui_ImplWin32_EnableDpiAwareness();
	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, Win32Callback, 0L, 0L,
		GetModuleHandleW(NULL), LoadIcon(hInstance, MAKEINTRESOURCE(101)),
		NULL, NULL, NULL, (L"Acoustic Soundboard"), NULL };
	RegisterClassEx(&wc);
	HWND hwnd = CreateWindow(wc.lpszClassName, (L"Acoustic Soundboard"),
		WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), NULL, NULL, wc.hInstance, NULL);

	CreateDeviceD3D(hwnd);
	InitAudioSystem();
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyboardHookCallback, GetModuleHandle(NULL), 0);
	if (KeyboardHook == NULL)
	{
		PrintToErrorLogAndExit(L"SetWindowsHookEx failed. Cannot continue.");
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 20.0f);
	ImGuiStyle& style = ImGui::GetStyle();
	ImGui::StyleColorsDark();
	style.ScaleAllSizes(main_scale);
	style.FontScaleDpi = main_scale;
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	long retVal = PathCchRemoveFileSpec(StartingDirectory.string, StartingDirectory.numWideChars);
	if (retVal != 0)
	{
		return 1;
	}
	bool success = SetCurrentDirectoryW(StartingDirectory.string);
	if (success == false)
	{
		PrintToLogWin32Error();
		return 1;
	}
	wmemcpy(PathToErrorLog.string, StartingDirectory.string, StartingDirectory.numWideChars);
	retVal = PathCchAppend(PathToErrorLog.string, PathToErrorLog.numWideChars, L"log-error.txt");
	if (retVal != 0)
	{
		return 1;
	}	
	wmemcpy(PathToDatabase.string, StartingDirectory.string, StartingDirectory.numWideChars);
	retVal = PathCchAppend(PathToDatabase.string, PathToDatabase.numWideChars, L"hotkeys.db");
	if (retVal != 0)
	{
		return 1;
	}

	EncodeUTF8(&PathToDatabase, &PathToDatabaseUTF8);
	InitSQLite();
	LoadHotkeysFromDatabase();
	LoadDevicesFromDatabase();
	LoadConfigFromDatabase();
	SetVolume();

	while (WindowShouldClose == false) 
	{
		MSG msg;
		while (GetMessageW(&msg, NULL, 0U, 0U))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			// Handle window being minimized or screen locked
			if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
			{
				::Sleep(10);
				continue;
			}
			g_SwapChainOccluded = false;
			DrawGUI();
			ResetNavKeys();
		}
	} 

	SaveDevicesToDatabase();
	CloseAudioSystem();
	SaveHotkeysToDatabase();
	SaveConfigToDatabase();
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CleanupDeviceD3D();
	DestroyWindow(hwnd);
	UnregisterClass(wc.lpszClassName, wc.hInstance);
	return 0;
}

LRESULT CALLBACK Win32Callback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	switch (message)
	{		
	case WM_SYSKEYDOWN: // Falls through
	case WM_KEYDOWN: 
	{
		switch (wParam)
		{
		case VK_RETURN: UserPressedReturn = true;
			break;
		case VK_ESCAPE: UserPressedEscape = true;
			break;
		case VK_BACK: UserPressedBackspace = true;
			break;
		default:
			break;
		}

		if (CaptureKeys == true)
		{
			bool ignoreKey = false;
			switch (wParam)
			{ // These fall through. 
			case VK_CONTROL:
			case VK_SHIFT:
			case VK_MENU:
			case VK_RETURN:
			case VK_ESCAPE:
			case VK_PAUSE:
			case VK_BACK:
			case VK_LWIN:
			case VK_RWIN:
				ignoreKey = true;
				break; //We are purposefully ignoring the above keys.
			case VK_TAB: wcscpy(CapturedKeyText.string, L"Tab");
				break;
			case VK_F1: wcscpy(CapturedKeyText.string, L"F1");
				break;
			case VK_F2: wcscpy(CapturedKeyText.string, L"F2");
				break;
			case VK_F3: wcscpy(CapturedKeyText.string, L"F3");
				break;
			case VK_F4: wcscpy(CapturedKeyText.string, L"F4");
				break;
			case VK_F5: wcscpy(CapturedKeyText.string, L"F5");
				break;
			case VK_F6: wcscpy(CapturedKeyText.string, L"F6");
				break;
			case VK_F7: wcscpy(CapturedKeyText.string, L"F7");
				break;
			case VK_F8: wcscpy(CapturedKeyText.string, L"F8");
				break;
			case VK_F9: wcscpy(CapturedKeyText.string, L"F9");
				break;
			case VK_F10: wcscpy(CapturedKeyText.string, L"F10");
				break;
			case VK_F11: wcscpy(CapturedKeyText.string, L"F11");
				break;
			case VK_F12: wcscpy(CapturedKeyText.string, L"F12");
				break;
			case VK_SCROLL:wcscpy(CapturedKeyText.string, L"Scroll Lock");
				break;
			case VK_INSERT: wcscpy(CapturedKeyText.string, L"Insert");
				break;
			case VK_HOME: wcscpy(CapturedKeyText.string, L"Home");
				break;
			case VK_PRIOR: wcscpy(CapturedKeyText.string, L"Page Up");
				break;
			case VK_DELETE: wcscpy(CapturedKeyText.string, L"Delete");
				break;
			case VK_END: wcscpy(CapturedKeyText.string, L"End");
				break;
			case VK_NEXT: wcscpy(CapturedKeyText.string, L"Page Down");
				break;
			case VK_CAPITAL: wcscpy(CapturedKeyText.string, L"Caps Lock");
				break;
			case VK_NUMLOCK: wcscpy(CapturedKeyText.string, L"Num Lock");
				break;
			case VK_DIVIDE: wcscpy(CapturedKeyText.string, L"Num /");
				break;
			case VK_MULTIPLY: wcscpy(CapturedKeyText.string, L"Num *");
				break;
			case VK_SUBTRACT: wcscpy(CapturedKeyText.string, L"Num -");
				break;
			case VK_ADD: wcscpy(CapturedKeyText.string, L"Num +");
				break;
			case VK_DECIMAL: wcscpy(CapturedKeyText.string, L"Num .");
				break;
			case VK_NUMPAD0: wcscpy(CapturedKeyText.string, L"Num 0");
				break;
			case VK_NUMPAD1: wcscpy(CapturedKeyText.string, L"Num 1");
				break;
			case VK_NUMPAD2: wcscpy(CapturedKeyText.string, L"Num 2");
				break;
			case VK_NUMPAD3: wcscpy(CapturedKeyText.string, L"Num 3");
				break;
			case VK_NUMPAD4: wcscpy(CapturedKeyText.string, L"Num 4");
				break;
			case VK_NUMPAD5: wcscpy(CapturedKeyText.string, L"Num 5");
				break;
			case VK_NUMPAD6: wcscpy(CapturedKeyText.string, L"Num 6");
				break;
			case VK_NUMPAD7: wcscpy(CapturedKeyText.string, L"Num 7");
				break;
			case VK_NUMPAD8: wcscpy(CapturedKeyText.string, L"Num 8");
				break;
			case VK_NUMPAD9: wcscpy(CapturedKeyText.string, L"Num 9");
				break;
			case VK_UP: wcscpy(CapturedKeyText.string, L"Up");
				break;
			case VK_RIGHT: wcscpy(CapturedKeyText.string, L"Right");
				break;
			case VK_DOWN: wcscpy(CapturedKeyText.string, L"Down");
				break;
			case VK_LEFT: wcscpy(CapturedKeyText.string, L"Left");
				break;
			default:
				const int scanCode = (lParam >> 16) & 0x00ff;
				BYTE keyboardState[256];
				bool gotKeyboardState = GetKeyboardState(keyboardState);
				if (gotKeyboardState == false)
				{
					PrintToLogWin32Error();
				}
				keyboardState[VK_CONTROL] = 0; // Zero out the byte that indicates control characters because we don't want those
				keyboardState[VK_SHIFT] = 0; // Zero out the byte that indicates SHIFT
				ToUnicode((uint32_t)wParam, scanCode, keyboardState, CapturedKeyText.string, CapturedKeyText.numWideChars - 1, 0); // Subtract 1 from the array chars because Windows may not null terminate it
				CharUpperW(CapturedKeyText.string);
				break;
			}

			if (ignoreKey == false)
			{
				CapturedKeyCode = (uint32_t)wParam;
				CapturedKeyMod = 0;
				EncodeUTF8(&CapturedKeyText, &CapturedKeyTextUTF8);
				bool altDown = (GetAsyncKeyState(VK_MENU) < 0);
				bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) < 0);
				bool shiftDown = (GetAsyncKeyState(VK_SHIFT) < 0);
				if (shiftDown) {
					strcpy(CapturedKeyModText.string, "SHIFT +");
					CapturedKeyMod = VK_SHIFT;
				}
				else if (ctrlDown) {
					strcpy(CapturedKeyModText.string, "CTRL +");
					CapturedKeyMod = VK_CONTROL;
				}
				else if (altDown) {
					strcpy(CapturedKeyModText.string, "ALT +");
					CapturedKeyMod = VK_MENU;
				}
			}
		}
		break;
	}
	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED)
		{
			break;
		}
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		break;
	}
	case WM_CLOSE:
		PostQuitMessage(0);
		WindowShouldClose = true;
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void AudioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	(void)pInput;
	ma_engine_read_pcm_frames((ma_engine*)pDevice->pUserData, pOutput, frameCount, NULL);
}

void CenterNextWindow(float minWidth, float minHeight)
{
	ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
	ImVec2 viewportCenter = { viewportSize.x / 2, viewportSize.y / 2 };
	ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, minHeight), viewportSize, NULL, NULL);
	ImGui::SetNextWindowPos(viewportCenter, 0, ImVec2(0.5f, 0.5f));
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

void ClearCapturedKey() 
{
	CapturedKeyCode = 0;
	CapturedKeyMod = 0;
	wmemset(CapturedKeyText.string, L'\0', CapturedKeyText.numWideChars);
	memset(CapturedKeyTextUTF8.string,'\0', CapturedKeyTextUTF8.numChars);
	memset(CapturedKeyModText.string,'\0', CapturedKeyModText.numChars);
}

void CloseAudioSystem()
{
	ClosePlaybackDevices();
	CloseCaptureDevice();

	free(PlaybackDeviceSelected);
	free(CaptureDeviceSelected);
	ma_resource_manager_uninit(&ResourceManager);
}

void CloseCaptureDevice()
{
	if (NumActiveCaptureDevices > 0)
	{
		ma_engine_uninit(&CaptureEngine.engine);
		ma_device_uninit(&CaptureEngine.device);
		CaptureEngine.captureDeviceName[0] = '\0';
		CaptureEngine.duplexDeviceName[0] = '\0';
		NumActiveCaptureDevices = 0;
	}

	for (ma_uint32 i = 0; i < CaptureDeviceCount; i++)
		CaptureDeviceSelected[i] = false;

	ShowDuplexDevices = false;
}

void ClosePlaybackDevices()
{
	for (int i = 0; i < MAX_PLAYBACK_DEVICES; i++)
	{
		if (PlaybackEngines[i].active == true)
		{
			ma_engine_uninit(&PlaybackEngines[i].engine);
			ma_device_uninit(&PlaybackEngines[i].device);



			for (int j = 0; j < NUM_SOUNDS; j++)
			{
				if (SoundLoaded[i][j] == true)
				{
					ma_sound_uninit(&PlaybackEngines[i].sounds[j]);
					SoundLoaded[i][j] = false;
				}
			}
			PlaybackEngines[i].active = false;
			PlaybackEngines[i].deviceName[0] = '\0';
		}
	}


	NumActivePlaybackDevices = 0;

	for (ma_uint32 i = 0; i < PlaybackDeviceCount; i++)
	{
		PlaybackDeviceSelected[i] = false;
	}
}

void CreateDeviceD3D(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
	{
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	}
	if (res != S_OK)
	{
		PrintToErrorLog(L"Failed to create Direct3D device. Cannot continue.");
		exit(1);
	}

	CreateRenderTarget();
	return;
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer = NULL;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (pBackBuffer != NULL)
	{
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
		pBackBuffer->Release();
	}
}

void DecodeUTF8(StringUTF8* str, StringUTF16* wideStr)
{
	str->string[str->numChars - 1] = '\0';
	int numCharsRequired = 0;
	numCharsRequired = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str->string, -1, NULL, 0);
	if (wideStr->numWideChars < numCharsRequired)
	{
		PrintToErrorLog(L"DecodeUTF8() the first call to MultiByteToWideChar found the buffer size required was larger than the given number of chars in the UTF-16 string (variable wideStr)");
		exit(1); // This error is considered unrecoverable and I want it to be very visible (app closing)
	}
	int retVal = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str->string, -1, wideStr->string, numCharsRequired);
	if (retVal == 0)
	{
		PrintToLogWin32Error();
	}
}

void DrawGUI() 
{
	// Handle window resize (we don't resize directly in the WM_SIZE handler)
	if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
	{
		CleanupRenderTarget();
		g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
		g_ResizeWidth = g_ResizeHeight = 0;
		CreateRenderTarget();
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();	
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::Begin("Audio", NULL, GUIWindowFlags);
	DrawHotkeyTable();
		// ---------- Page display ----------
	ImGui::Text("%s %d %s %d", "Page ", CurrentPage, " / ", 5);
	if (ImGui::Button("< Previous Page") == true) {
		if (CurrentPage > 1) {
			CurrentPage--;
		}
	}
	ImGui::SameLine();
	ImGui::SameLine();
	if (ImGui::Button("> Next Page") == true) {
		if (CurrentPage < 5) {
			CurrentPage++;
		}
	} // ---------- END Page display ----------

	#ifdef _DEBUG
	if (ImGui::Button("Debug info") == true)
		ShowDebugWindow = true;

	if (ShowDebugWindow == true)
	{
		ImGui::Begin("Debug", &ShowDebugWindow);
		ImGui::Text("NumActivePlaybackDevices: \t %d", NumActivePlaybackDevices);
		ImGui::Text("NumActiveCaptureDevices: \t %d", NumActiveCaptureDevices);
		ImGui::NewLine();
		ImGui::Text("PlaybackEngine 0: \t %s", PlaybackEngines[0].deviceName);
		ImGui::Text("PlaybackEngine 0 active: \t %i", PlaybackEngines[0].active);
		ImGui::Text("PlaybackEngine 1: \t %s", PlaybackEngines[1].deviceName);
		ImGui::Text("PlaybackEngine 1 active: \t %i", PlaybackEngines[1].active);
		ImGui::NewLine();
		ImGui::Text("Capture device: \t %s", CaptureEngine.captureDeviceName);
		ImGui::Text("Duplex device: \t %s", CaptureEngine.duplexDeviceName);
		ImGui::NewLine();
		ImGui::Text("Last Playback 0: \t %s", SavedPlaybackDeviceName1.string);
		ImGui::Text("Last Playback 1: \t %s", SavedPlaybackDeviceName2.string);
		ImGui::Text("Last Capture: \t %s", SavedCaptureDeviceName.string);
		ImGui::Text("Last Duplex: \t %s", SavedDuplexDeviceName.string);
		ImGui::End();
	}
	#endif

	ImGui::NewLine();
	ImGui::Text("Volume");
	ImGui::PushItemWidth(150.0f);
	if (ImGui::SliderInt("##", &VolumeDisplay, 0, 100))
	{
		SetVolume();
	}
	ImGui::NewLine();

	if (ImGui::Button("Stop All Sounds") == true) 
	{
		for (int i = 0; i < NUM_SOUNDS; i++)
		{
			UnloadSound(i);
		}
	}
	ImGui::SameLine();
	ImGui::Text("Pause | Break");

	ImGui::NewLine();
	if (NumActivePlaybackDevices > 0 || NumActiveCaptureDevices > 0)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(150, 0, 0, 255));
		if (ImGui::Button("Close Devices"))
		{
			ClosePlaybackDevices();
			ShowPlaybackDeviceList = false;
			CloseCaptureDevice();
			ShowCaptureDeviceList = false;
		}
		ImGui::PopStyleColor();
	}

	ImGui::BeginTable("Playback Devices", 1);
	ImGui::TableSetupColumn("Playback Devices");
	ImGui::TableHeadersRow();
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	if (NumActivePlaybackDevices < MAX_PLAYBACK_DEVICES)
	{
		if (ImGui::Button("Select Playback Device"))
		{
			ShowPlaybackDeviceList = true;
		}

		if (UserPressedEscape == true)
		{
			ShowPlaybackDeviceList = false;
		}

		if (ShowPlaybackDeviceList)
		{
			SelectPlaybackDevice();
		}
	}
	else
	{
		ShowPlaybackDeviceList = false;
	}

	for (int i = 0; i < MAX_PLAYBACK_DEVICES; i++)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("%s", PlaybackEngines[i].deviceName);
	}
	ImGui::EndTable();

	ImGui::BeginTable("Capture Device", 1);
	ImGui::TableSetupColumn("Capture Device");
	ImGui::TableHeadersRow();
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	if (NumActiveCaptureDevices <= 0)
	{
		if (ImGui::Button("Select Recording Device"))
		{
			ShowCaptureDeviceList = true;
		}

		if (UserPressedEscape == true)
		{
			ShowCaptureDeviceList = false;
		}

		if (ShowCaptureDeviceList)
		{
			SelectCaptureDevice();
		}
	}
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::Text("%s", CaptureEngine.captureDeviceName);
	if (NumActiveCaptureDevices > 0)
	{
		ImGui::NewLine();
		ImGui::Text("This device is playing through");
		ImGui::Text("%s", CaptureEngine.duplexDeviceName);
	}
	ImGui::EndTable();

	if (ShowKeyCaptureWindow == true)
	{
		DrawKeyCaptureWindow();
	}
	CaptureKeys = ShowKeyCaptureWindow;

	// ---------- Key In Use Window ----------
	if (CapturedKeyInUse == true) 
	{
		ImGui::Begin("Invalid Key", &CapturedKeyInUse, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Key already in use.\nClose this window and try again.");
		if (ImGui::Button(" OK ") == true || UserPressedEscape || UserPressedReturn)
		{
			if (UserPressedEscape == true)
			{
				UserPressedEscape = false;
			}
			if (UserPressedReturn == true)
			{
				UserPressedReturn = false;
			}
			CapturedKeyInUse = false;
			CaptureKeys = false;
		}
		ClearCapturedKey();
		ImGui::End();
	} // ----------END Key In Use Window ----------

	// ---------- Error Message Window ----------
	if (DisplayErrorMessageWindow == true) 
	{
		CenterNextWindow(300.0f, 300.0f);
		ImGui::Begin("Error", &DisplayErrorMessageWindow, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text(ErrorMsgBuffer.string);
		if (ImGui::Button(" OK ") == true || UserPressedEscape || UserPressedReturn)
		{
			DisplayErrorMessageWindow = false;
		}
		ImGui::End();
	} // ----------END Audio Error Message Window ----------

	ImGui::End(); // End main window
	ImGui::Render();
	const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
	g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
	g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
}

void DrawHotkeyTable()
{
	ImGui::BeginTable("Sounds", 3, GUITableFlags);
	ImGui::TableSetupColumn(" #  Hotkey", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("File");
	ImGui::TableSetupColumn("Reset file", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableHeadersRow();
	int firstElement;
	int lastElement;
	switch (CurrentPage)
	{
	case 1:
		firstElement = 0;
		lastElement = 9;
		break;
	case 2:
		firstElement = 10;
		lastElement = 19;
		break;
	case 3:
		firstElement = 20;
		lastElement = 29;
		break;
	case 4:
		firstElement = 30;
		lastElement = 39;
		break;
	case 5:
		firstElement = 40;
		lastElement = 49;
		break;
	}
	for (int i = firstElement; i <= lastElement; i++)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("%02d", i + 1);
		ImGui::SameLine();
		ImGui::PushID(i);
		if (ImGui::Button("Set Hotkey") == true || ImGui::IsItemFocused() && UserPressedReturn) {
			if (UserPressedReturn == true)
			{
				UserPressedReturn = false;
			}
			CaptureKeys = true;
			CapturedKeyIndex = i;
			ShowKeyCaptureWindow = true;
		}
		ImGui::PopID();

		ImGui::SameLine();
		ImGui::Text(Hotkeys[i].modText.string);
		ImGui::SameLine();
		ImGui::Text(Hotkeys[i].keyTextUTF8.string);
		ImGui::TableNextColumn();
		ImGui::PushID(i);
		if (ImGui::Button("Select file") == true || (ImGui::IsItemFocused() && UserPressedReturn))
		{
			if (UserPressedReturn == true)
			{
				UserPressedReturn = false;
			}
			UnloadSound(i);
			StringUTF16 fileNamebuffer;
			StringUTF16 fullPathbuffer;
			wchar_t* filePart = NULL;
			OPENFILENAMEW ofn = { 0 };
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = NULL;
			ofn.lpstrFile = fileNamebuffer.string;
			ofn.nMaxFile = fileNamebuffer.numWideChars;
			ofn.lpstrFilter = L"Audio (.wav, .mp3, .ogg, .flac)\0*.wav;*.mp3;*.ogg;*.flac\0";
			ofn.nFilterIndex = 1;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			if (GetOpenFileNameW(&ofn)) 
			{
				DWORD wideCharsRequired = GetFullPathNameW(fileNamebuffer.string, fullPathbuffer.numWideChars, fullPathbuffer.string, &filePart);
				if (wideCharsRequired == 0)
				{
					PrintToLogWin32Error();
				}
				else
				{
					wmemcpy(Hotkeys[i].filePath.string, fileNamebuffer.string, Hotkeys[i].filePath.numWideChars);
					// Zero fileNameBuffer so we can reuse it to store just the file name part of the path
					wmemset(fileNamebuffer.string, L'\0', fileNamebuffer.numWideChars);
					wcscpy(fileNamebuffer.string, filePart);
					EncodeUTF8(&fileNamebuffer, &Hotkeys[i].fileNameUTF8);
				}
			}
			else 
			{
				DWORD dialogError = CommDlgExtendedError();
				if (dialogError == FNERR_BUFFERTOOSMALL)
				{
					snprintf(ErrorMsgBuffer.string, ErrorMsgBuffer.numChars, "The file path is too long. This application supports file paths up to %d characters. Shorten the path and try again.", APP_MAX_PATH);
					DisplayErrorMessageWindow = true;
				}
				else
				{
					PrintToLogWin32Error();
				}
			}
		}
		ImGui::PopID();
		ImGui::SameLine();
		ImGui::Text(Hotkeys[i].fileNameUTF8.string);
		ImGui::TableNextColumn();
		ImGui::PushID(i);
		if (ImGui::Button("Reset") == true || ImGui::IsItemFocused() && UserPressedReturn)
		{
			if (UserPressedReturn == true)
			{
				UserPressedReturn = false;
			}

			wmemset(Hotkeys[i].filePath.string, L'\0', Hotkeys[i].filePath.numWideChars);
			memset(Hotkeys[i].fileNameUTF8.string, '\0', Hotkeys[i].fileNameUTF8.numChars);
			UnloadSound(i);
		}
		ImGui::PopID();
	}
	ImGui::EndTable();
}

void DrawKeyCaptureWindow()
{
	CenterNextWindow(500.0f, 500.0f);
	ImGui::Begin("Set Hotkey", &ShowKeyCaptureWindow, ImGuiWindowFlags_NoNav);
	ImGui::Text("SHIFT, CTRL, ALT combos are supported");
	ImGui::NewLine();
	ImGui::Text("Press a key . . .");
	ImGui::Text("%s", CapturedKeyModText.string);
	ImGui::Text("%s", CapturedKeyTextUTF8.string);
	ImGui::NewLine();
	ImGui::Text("%s", "ENTER to set");
	ImGui::Text("%s", "BACKSPCE to clear");
	ImGui::Text("%s", "ESCAPE to exit");
	ImGui::NewLine();
	if (UserPressedEscape == true)
	{
		UserPressedEscape = false;
		ShowKeyCaptureWindow = false;
	}

	if (ImGui::Button("Set") == true || UserPressedReturn)
	{
		if (UserPressedReturn == true)
			UserPressedReturn = false;

		if (CapturedKeyCode == 0)
		{
			Hotkeys[CapturedKeyIndex].keyCode = 0;
			Hotkeys[CapturedKeyIndex].keyMod = 0;
			wmemset(Hotkeys[CapturedKeyIndex].keyText.string, L'\0', Hotkeys[CapturedKeyIndex].keyText.numWideChars);
			memset(Hotkeys[CapturedKeyIndex].modText.string, '\0', Hotkeys[CapturedKeyIndex].modText.numChars);
			memset(Hotkeys[CapturedKeyIndex].keyTextUTF8.string, '\0', Hotkeys[CapturedKeyIndex].keyTextUTF8.numChars);
		}

		else
		{
			for (int i = 0; i < NUM_SOUNDS; i++)
			{
				// Compare the captured key with all the stored hotkeys
				if (Hotkeys[i].keyCode == CapturedKeyCode)
				{
					// Then compare the captured keyMod with the corresponding stored keyMod
					if (Hotkeys[i].keyMod == CapturedKeyMod)
					{
						CapturedKeyInUse = true;
						ShowKeyCaptureWindow = false;
					}
				}
			}

			if (CapturedKeyInUse == false)
			{
				wmemcpy(Hotkeys[CapturedKeyIndex].keyText.string, CapturedKeyText.string, CapturedKeyText.numWideChars);
				memcpy(Hotkeys[CapturedKeyIndex].keyTextUTF8.string, CapturedKeyTextUTF8.string, CapturedKeyTextUTF8.numChars);
				Hotkeys[CapturedKeyIndex].keyCode = CapturedKeyCode;
				memcpy(Hotkeys[CapturedKeyIndex].modText.string, CapturedKeyModText.string, CapturedKeyModText.numChars);
				Hotkeys[CapturedKeyIndex].keyMod = CapturedKeyMod;
			}
		}

		CaptureKeys = false;
		ShowKeyCaptureWindow = false;
		ClearCapturedKey();
	}

	ImGui::NewLine();
	if (ImGui::Button("Clear") == true || UserPressedBackspace) {
		if (UserPressedBackspace == true)
		{
			UserPressedBackspace = false;
		}

		Hotkeys[CapturedKeyIndex].keyCode = 0;
		Hotkeys[CapturedKeyIndex].keyMod = 0;

		wmemset(Hotkeys[CapturedKeyIndex].keyText.string, L'\0', Hotkeys[CapturedKeyIndex].keyText.numWideChars);
		memset(Hotkeys[CapturedKeyIndex].modText.string, '\0', Hotkeys[CapturedKeyIndex].modText.numChars);
		memset(Hotkeys[CapturedKeyIndex].keyTextUTF8.string, '\0', Hotkeys[CapturedKeyIndex].keyTextUTF8.numChars);
		ClearCapturedKey();
	}
	ImGui::End();
}

void DuplexDeviceCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	memcpy((pOutput), (pInput), (frameCount * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels)));
}

void EncodeUTF8(StringUTF16* wideStr, StringUTF8* str)
{
	wideStr->string[wideStr->numWideChars - 1] = L'\0';
	int numBytesRequired = 0;
	numBytesRequired = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wideStr->string, -1, NULL, 0, NULL, NULL);
	if (str->numChars < numBytesRequired)
	{
		PrintToErrorLogAndExit(L"EncodeUTF8() the first call to WideCharToMultiByte found the buffer size required was larger than the given number of chars in the UTF8-string (variable str)");
		// This error is considered unrecoverable and I want it to be very visible
	}
	numBytesRequired = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wideStr->string, -1, str->string, numBytesRequired, NULL, NULL);
	if (numBytesRequired == 0)
	{
		PrintToLogWin32Error();
	}
}

void InitAudioSystem()
{
	ma_resource_manager_config resourceManagerConfig;
	resourceManagerConfig = ma_resource_manager_config_init();
	resourceManagerConfig.decodedFormat = ma_format_f32;
	resourceManagerConfig.decodedChannels = 0;
	resourceManagerConfig.decodedSampleRate = 48000;

	ma_result result;
	result = ma_resource_manager_init(&resourceManagerConfig, &ResourceManager);
	if (result != MA_SUCCESS)
	{
		PrintToErrorLogAndExit(L"Failed to initialize miniaudio resource manager. Cannot continue.");
	}

	result = ma_context_init(NULL, 0, NULL, &Context);
	if (result != MA_SUCCESS)
	{
		PrintToErrorLogAndExit(L"Failed to initialize miniaudio context. Cannot continue.");
	}

	result = ma_context_get_devices(&Context, &pPlaybackDeviceInfos, &PlaybackDeviceCount, &pCaptureDeviceInfos, &CaptureDeviceCount);
	if (result != MA_SUCCESS)
	{
		PrintToErrorLogAndExit(L"Failed to initialize miniaudio context. Cannot continue.");
	}

	PlaybackDeviceSelected = (bool*)malloc(sizeof(bool) * PlaybackDeviceCount);
	if (PlaybackDeviceSelected != NULL)
	{
		for (ma_uint32 i = 0; i < PlaybackDeviceCount; i++)
		{
			PlaybackDeviceSelected[i] = false;
		}
	}
	CaptureDeviceSelected = (bool*)malloc(sizeof(bool) * CaptureDeviceCount);
	if (CaptureDeviceSelected != NULL)
	{
		for (ma_uint32 i = 0; i < CaptureDeviceCount; i++)
		{
			CaptureDeviceSelected[i] = false;
		}
	}

	return;
}

void InitCaptureDevice(ma_device_id* captureId, char* captureDeviceName, ma_device* duplexDevice, char* duplexDeviceName)
{
	ma_device_config deviceConfig;
	ma_engine_config engineConfig;

	deviceConfig = ma_device_config_init(ma_device_type_duplex);
	deviceConfig.capture.pDeviceID = captureId;
	deviceConfig.capture.format = duplexDevice->playback.format;
	deviceConfig.capture.channels = duplexDevice->playback.channels;
	deviceConfig.capture.shareMode = ma_share_mode_shared;
	deviceConfig.playback.pDeviceID = &duplexDevice->playback.id;
	deviceConfig.playback.format = duplexDevice->playback.format;
	deviceConfig.playback.channels = duplexDevice->playback.channels;
	deviceConfig.dataCallback = DuplexDeviceCallback;
	deviceConfig.pUserData = &CaptureEngine.engine;

	ma_result result;
	result = ma_device_init(&Context, &deviceConfig, &CaptureEngine.device);
	if (result != MA_SUCCESS)
	{
		PrintToErrorLogAndExit(L"Failed to initialize miniaudio capture device. Cannot continue.");
	}

	engineConfig = ma_engine_config_init();
	engineConfig.pDevice = &CaptureEngine.device;
	engineConfig.pResourceManager = &ResourceManager;
	engineConfig.noAutoStart = MA_TRUE;

	result = ma_engine_init(&engineConfig, &CaptureEngine.engine);
	if (result != MA_SUCCESS)
	{
		PrintToErrorLogAndExit(L"Failed to initialize miniaudio capture device engine. Cannot continue.");
	}

	result = ma_engine_start(&CaptureEngine.engine);
	if (result != MA_SUCCESS)
	{
		PrintToErrorLogAndExit(L"Failed to start miniaudio capture device engine. Cannot continue.");
	}

	strcpy(CaptureEngine.captureDeviceName, captureDeviceName);
	strcpy(CaptureEngine.duplexDeviceName, duplexDeviceName);
	NumActiveCaptureDevices++;
}

void InitPlaybackDevice(ma_device_id* deviceId, int iEngine, char* deviceName)
{
	ma_device_config deviceConfig;
	ma_engine_config engineConfig;
	deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.pDeviceID = deviceId;
	deviceConfig.playback.format = ResourceManager.config.decodedFormat;
	deviceConfig.playback.channels = 0;
	deviceConfig.sampleRate = ResourceManager.config.decodedSampleRate;
	deviceConfig.dataCallback = AudioCallback;
	deviceConfig.pUserData = &PlaybackEngines[iEngine].engine;

	ma_result result = ma_device_init(&Context, &deviceConfig, &PlaybackEngines[iEngine].device);
	if (result != MA_SUCCESS) 
	{
		PrintToErrorLogAndExit(L"Failed to initialize miniaudio playback device. Cannot continue.");
	}

	engineConfig = ma_engine_config_init();
	engineConfig.pDevice = &PlaybackEngines[iEngine].device;
	engineConfig.pResourceManager = &ResourceManager;
	engineConfig.noAutoStart = MA_TRUE;

	result = ma_engine_init(&engineConfig, &PlaybackEngines[iEngine].engine);
	if (result != MA_SUCCESS) 
	{
		PrintToErrorLogAndExit(L"Failed to initialize miniaudio playback device engine. Cannot continue.");
	}

	result = ma_engine_start(&PlaybackEngines[iEngine].engine);
	if (result != MA_SUCCESS) 
	{
		PrintToErrorLogAndExit(L"Failed to start miniaudio playback device engine. Cannot continue.");
	}

	strcpy(PlaybackEngines[iEngine].deviceName, deviceName);
	PlaybackEngines[iEngine].active = true;
	NumActivePlaybackDevices++;
}

void InitSQLite() {
	//---------- Create hotkeys table ----------
	char* zErrMsg = 0;
	int rc;
	const char* sql_statement =
		"CREATE TABLE IF NOT EXISTS HOTKEYS (KEY_INDEX INTEGER PRIMARY KEY, KEY_MOD INTEGER, KEY INTEGER, MOD_TEXT TEXT, KEY_TEXT TEXT, KEY_TEXT_UTF8 TEXT, FILE_PATH TEXT, FILE_NAME_UTF8 TEXT);";
	sqlite3_stmt* prepared_statement = NULL;
	rc = sqlite3_open(PathToDatabaseUTF8.string, &db);

	if (rc) {
		PrintToErrorLog(L"SQL Error: Can't open database to create hotkeys table");
		sqlite3_close(db);
	}

	rc = sqlite3_prepare_v2(db, sql_statement, -1, &prepared_statement, NULL);
	if (rc != SQLITE_OK) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_step(prepared_statement);
	if (rc != SQLITE_DONE) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_finalize(prepared_statement);
	if (rc != SQLITE_OK) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}

	sqlite3_close(db);

	//---------- Create playback devices table ----------
	zErrMsg = 0;
	rc = 0;
	sql_statement =
		"CREATE TABLE IF NOT EXISTS DEVICES (DEVICE_INDEX INTEGER PRIMARY KEY, NAME TEXT);";
	prepared_statement = NULL;
	rc = sqlite3_open(PathToDatabaseUTF8.string, &db);

	if (rc) {
		PrintToErrorLog(L"SQL Error: Can't open database to create playback devices table");
		sqlite3_close(db);
	}

	rc = sqlite3_prepare_v2(db, sql_statement, -1, &prepared_statement, NULL);
	if (rc != SQLITE_OK) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_step(prepared_statement);
	if (rc != SQLITE_DONE) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_finalize(prepared_statement);
	if (rc != SQLITE_OK) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}

	//---------- Create configuration table ----------
	zErrMsg = 0;
	rc = 0;
	sql_statement =
		"CREATE TABLE IF NOT EXISTS CONFIG (CONFIG_INDEX INTEGER PRIMARY KEY, VOLUME INTEGER);";
	prepared_statement = NULL;
	rc = sqlite3_open(PathToDatabaseUTF8.string, &db);

	if (rc) {
		PrintToErrorLog(L"SQL Error: Can't open database to create config table");
		sqlite3_close(db);
	}

	rc = sqlite3_prepare_v2(db, sql_statement, -1, &prepared_statement, NULL);
	if (rc != SQLITE_OK) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_step(prepared_statement);
	if (rc != SQLITE_DONE) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_finalize(prepared_statement);
	if (rc != SQLITE_OK) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}

	sqlite3_close(db);
}

void InterpretAudioErrorMessage(ma_result result, uint32_t hotKeyIndex)
{
	switch (result)
	{
	case MA_DOES_NOT_EXIST:
		snprintf(ErrorMsgBuffer.string, ErrorMsgBuffer.numChars, "The selected file does not exist at the last known file path for hotkey %d", hotKeyIndex);
		break;
	case MA_INVALID_ARGS:
		snprintf(ErrorMsgBuffer.string, ErrorMsgBuffer.numChars, "The call to PlayAudio() had invalid arguments. Check the file for hotkey %d", hotKeyIndex);
		break;
	default:
		snprintf(ErrorMsgBuffer.string, ErrorMsgBuffer.numChars, "The call to PlayAudio() failed for a reason not specified for hotkey %d", hotKeyIndex);
		break;
	}
	return;
}

LRESULT CALLBACK KeyboardHookCallback(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam) 
{
	// If nCode is less than zero, we must pass the message to the next hook
	if (nCode >= 0 && CaptureKeys == false)
	{
		switch (wParam) 
		{
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
			bool altDown = (GetAsyncKeyState(VK_MENU) < 0);
			bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) < 0);
			bool shiftDown = (GetAsyncKeyState(VK_SHIFT) < 0);

			if (p->vkCode == VK_PAUSE) 
			{
				for (int i = 0; i < NUM_SOUNDS; i++)
				{
					UnloadSound(i);
				}
				break;
			}

			bool hotkeyFound = false;
			for (uint32_t i = 0; i < NUM_SOUNDS; i++) 
			{
				if (Hotkeys[i].keyCode != 0) 
				{
					if (Hotkeys[i].keyMod != 0) 
					{
						if (p->vkCode == Hotkeys[i].keyCode) 
						{
							if (shiftDown) {
								if (Hotkeys[i].keyMod == VK_SHIFT)
								{
									hotkeyFound = true;
								}
							}
							else if (ctrlDown) 
							{
								if (Hotkeys[i].keyMod == VK_CONTROL)
								{
									hotkeyFound = true;
								}
							}
							else if (altDown) 
							{
								if (Hotkeys[i].keyMod == VK_MENU)
								{
									hotkeyFound = true;
								}
							}
						}
					}
					// Else if there is no modifier
					else if ((p->vkCode == Hotkeys[i].keyCode) &&
						altDown == false &&
						ctrlDown == false &&
						shiftDown == false) 
					{
						hotkeyFound = true;
					}

					if (hotkeyFound == true) 
					{
						if (NumActivePlaybackDevices == 0)
						{
							snprintf(ErrorMsgBuffer.string, ErrorMsgBuffer.numChars, "No playback device selected.");
							DisplayErrorMessageWindow = true;
							break;
						}

						for (uint32_t j = 0; j < MAX_PLAYBACK_DEVICES; j++)
						{
							if (PlaybackEngines[j].active == true)
							{
								ma_result result = PlayAudio(j, i, Hotkeys[i].filePath.string);
								if (result != MA_SUCCESS)
								{
									if (Hotkeys[i].filePath.string[0] == NULL)
									{
										snprintf(ErrorMsgBuffer.string, ErrorMsgBuffer.numChars, "No file selected for hotkey %d", i + 1);
									}
									else
									{
										InterpretAudioErrorMessage(result, i + 1);
									}

									DisplayErrorMessageWindow = true;
									break;
								}
							}
						}
						break; // Break out of the "sound search" loop
					}
				}
			}
		} // End WM_KEYUP
		default:
			break;
		} // End wParam switch (message)
	}
	return CallNextHookEx(0, nCode, wParam, lParam);
}

void LoadConfigFromDatabase()
{
	char* errorMessage = NULL;
	int rc = sqlite3_open(PathToDatabaseUTF8.string, &db);

	if (rc)
	{
		PrintToErrorLog(L"SQL Error: Can't open database for loading config");
		return;
	}

	sqlite3_stmt* stmt;
	const char* sql = "SELECT * FROM CONFIG";
	int rowCounter = 0;
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		goto cleanup;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		if (rowCounter < 1)
		{
			VolumeDisplay = sqlite3_column_int(stmt, 1);
			rowCounter++;
		}
	}

	if (rc != SQLITE_DONE)
	{
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
	}

cleanup:
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	sqlite3_free(errorMessage);
}

void LoadDevicesFromDatabase() 
{
	char* errorMessage = NULL;
	int rc = sqlite3_open(PathToDatabaseUTF8.string, &db);

	if (rc) 
	{
		PrintToErrorLog(L"SQL Error: Can't open database for loading devices");
		return;
	}

	sqlite3_stmt* stmt;
	int rowCounter = 0;
	const char* sql = "SELECT * FROM DEVICES";
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) 
	{
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		goto cleanup;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		if (rowCounter < 4)
		{
			if (rowCounter == 0)
			{
				SQLiteCopyColumnText(stmt, rowCounter, 1, L"DeviceName", &SavedPlaybackDeviceName1);
			}
			else if (rowCounter == 1)
			{
				SQLiteCopyColumnText(stmt, rowCounter, 1, L"DeviceName", &SavedPlaybackDeviceName2);
			}
			else if (rowCounter == 2)
			{
				SQLiteCopyColumnText(stmt, rowCounter, 1, L"DeviceName", &SavedCaptureDeviceName);
			}
			else if (rowCounter == 3)
			{
				SQLiteCopyColumnText(stmt, rowCounter, 1, L"DeviceName", &SavedDuplexDeviceName);
			}
			rowCounter++;
		}
	}

	if (rc != SQLITE_DONE)
	{
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
	}

cleanup:
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	sqlite3_free(errorMessage);
	
	DuplexDeviceIndex = -1;
	for (int i = 0; i < MAX_PLAYBACK_DEVICES; i++)
	{
		const char* currentDevice = NULL;
		if (i == 0)
		{
			currentDevice = SavedPlaybackDeviceName1.string;
		}
		else if (i == 1)
		{
			currentDevice = SavedPlaybackDeviceName2.string;
		}

		for (ma_uint32 j = 0; j < PlaybackDeviceCount; j++)
		{
			if (strcmp(pPlaybackDeviceInfos[j].name, currentDevice) == 0)
			{
				PlaybackDeviceSelected[j] = true;
				InitPlaybackDevice(&pPlaybackDeviceInfos[j].id, i, pPlaybackDeviceInfos[j].name);
				break;
			}
		}

		if (strcmp(PlaybackEngines[i].deviceName, SavedDuplexDeviceName.string) == 0)
		{
			DuplexDeviceIndex = i;
		}
	}

	if (DuplexDeviceIndex >= 0)
	{
		for (ma_uint32 i = 0; i < CaptureDeviceCount; i++)
		{
			if (strcmp(pCaptureDeviceInfos[i].name, SavedCaptureDeviceName.string) == 0)
			{
				CaptureDeviceSelected[i] = true;
				InitCaptureDevice(&pCaptureDeviceInfos[i].id, pCaptureDeviceInfos[i].name,
					&PlaybackEngines[DuplexDeviceIndex].device, PlaybackEngines[DuplexDeviceIndex].deviceName);
				break;
			}
		}
	}
}

void LoadHotkeysFromDatabase() {
	char* errorMessage = NULL;
	int rc = sqlite3_open(PathToDatabaseUTF8.string, &db);

	if (rc) 
	{
		PrintToErrorLog(L"SQL Error: Can't open database for loading hotkeys");
		return;
	}

	sqlite3_stmt* stmt;
	int rowCounter = 0;
	const char* sql = "SELECT * FROM HOTKEYS";
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) 
	{
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		goto cleanup;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) 
	{
		if (rowCounter < MAX_SOUNDS) 
		{
			Hotkeys[rowCounter].keyMod = sqlite3_column_int(stmt, 1);
			Hotkeys[rowCounter].keyCode = sqlite3_column_int(stmt, 2);
			SQLiteCopyColumnText(stmt, rowCounter, 3, L"modText", &Hotkeys[rowCounter].modText);
			SQLiteCopyColumnText16(stmt, rowCounter, 4, L"keyText", &Hotkeys[rowCounter].keyText);
			SQLiteCopyColumnText(stmt, rowCounter, 5, L"keyTextUTF8", &Hotkeys[rowCounter].keyTextUTF8);
			SQLiteCopyColumnText16(stmt, rowCounter, 6, L"filePath", &Hotkeys[rowCounter].filePath);
			SQLiteCopyColumnText(stmt, rowCounter, 7, L"fileNameUTF8", &Hotkeys[rowCounter].fileNameUTF8);
			rowCounter++;
		}
	}

	if (rc != SQLITE_DONE) 
	{
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
	}

cleanup:
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	sqlite3_free(errorMessage);
}

ma_result PlayAudio(uint32_t iEngine, uint32_t iSound, const wchar_t* filePath)
{
	ma_result result = MA_SUCCESS;
	if (SoundLoaded[iEngine][iSound] == false)
	{
		ma_uint32 flags = MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE |
			MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_ASYNC |
			MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_STREAM;

		result = ma_sound_init_from_file_w(&PlaybackEngines[iEngine].engine,
			filePath, flags, NULL, NULL, &PlaybackEngines[iEngine].sounds[iSound]);

		if (result != MA_SUCCESS)
		{
			return result;
		}

		SoundLoaded[iEngine][iSound] = true;
	}

	result = ma_sound_start(&PlaybackEngines[iEngine].sounds[iSound]);
	return result;
}

void PrintToErrorLog(const wchar_t* logText) 
{
	wchar_t timeString[32] = { 0 };
	struct tm newtime;
	__time32_t aclock;
	errno_t retVal;
	_time32(&aclock);
	_localtime32_s(&newtime, &aclock);

	retVal = _wasctime_s(timeString, sizeof(timeString) / sizeof(wchar_t), &newtime);
	if (retVal)
	{
		wcscpy(timeString, L"ERROR: _wasctime_s failed");
	}

	FILE* file = 0;
	if (file = _wfopen(PathToErrorLog.string, L"a")) 
	{
		fwprintf(file, L"%s%s%s", timeString, L" ", logText);
		fwprintf(file, L"\n");
		fclose(file);
	}
}

void PrintToErrorLogAndExit(const wchar_t* logText)
{
	PrintToErrorLog(logText);
	exit(1);
}

void PrintToLogWin32Error()
{
	DWORD dLastError = GetLastError();
	if (dLastError == 0)
	{
		return; // "The operation completed successfully". No need to log.
	}

	LPCTSTR strErrorMessage = NULL;

	FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		dLastError,
		0,
		(LPWSTR)&strErrorMessage,
		0,
		NULL);

	PrintToErrorLog(strErrorMessage);
}

void ResetNavKeys() {
	UserPressedReturn = false;
	UserPressedEscape = false;
	UserPressedBackspace = false;
}

void SaveConfigToDatabase()
{
	char* zErrMsg = 0;
	int rc;
	const char* sql_statement = "REPLACE INTO CONFIG(CONFIG_INDEX, VOLUME) VALUES (?, ?)";
	sqlite3_stmt* prepared_statement = NULL;
	rc = sqlite3_open(PathToDatabaseUTF8.string, &db);

	if (rc) 
	{
		PrintToErrorLog(L"SQL Error: Can't open database");
		return;
	}

	rc = sqlite3_prepare_v2(db, sql_statement, -1, &prepared_statement, NULL);
	if (rc != SQLITE_OK) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}
	rc = sqlite3_bind_int(prepared_statement, 1, 1);
	if (rc != SQLITE_OK) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}
	rc = sqlite3_bind_int(prepared_statement, 2, VolumeDisplay);
	if (rc != SQLITE_OK) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}
	rc = sqlite3_step(prepared_statement);
	if (rc != SQLITE_DONE) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}
	rc = sqlite3_finalize(prepared_statement);
	if (rc != SQLITE_OK) {
		PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
		sqlite3_free(zErrMsg);
	}
	sqlite3_close(db);
}

void SaveHotkeysToDatabase() 
{
	char* zErrMsg = 0;
	int rc;
	const char* sql_statement = "REPLACE INTO HOTKEYS(KEY_INDEX, KEY_MOD, KEY, MOD_TEXT, KEY_TEXT, KEY_TEXT_UTF8, FILE_PATH, FILE_NAME_UTF8) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
	sqlite3_stmt* prepared_statement = NULL;
	rc = sqlite3_open(PathToDatabaseUTF8.string, &db);


	if (rc) {
		PrintToErrorLog(L"SQL Error: Can't open database");
		sqlite3_close(db);
	}

	for (int i = 0; i < NUM_SOUNDS; i++) {
		rc = sqlite3_prepare_v2(db, sql_statement, -1, &prepared_statement, NULL);
		if (rc != SQLITE_OK) {
			const char* error = sqlite3_errmsg(db);
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}
		
		rc = sqlite3_bind_int(prepared_statement, 1, i);
		if (rc != SQLITE_OK) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_int(prepared_statement, 2, Hotkeys[i].keyMod);
		if (rc != SQLITE_OK) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_int64(prepared_statement, 3, Hotkeys[i].keyCode);
		if (rc != SQLITE_OK) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_text(prepared_statement, 4, Hotkeys[i].modText.string, -1, NULL);
		if (rc != SQLITE_OK) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_text16(prepared_statement, 5, Hotkeys[i].keyText.string, -1, NULL);
		if (rc != SQLITE_OK) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_text(prepared_statement, 6, Hotkeys[i].keyTextUTF8.string, -1, NULL);
		if (rc != SQLITE_OK) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_text16(prepared_statement, 7, Hotkeys[i].filePath.string, -1, NULL);
		if (rc != SQLITE_OK) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_text(prepared_statement, 8, Hotkeys[i].fileNameUTF8.string, -1, NULL);
		if (rc != SQLITE_OK) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}

		rc = sqlite3_step(prepared_statement);
		if (rc != SQLITE_DONE) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}

		rc = sqlite3_finalize(prepared_statement);
		if (rc != SQLITE_OK) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}
	}

	sqlite3_close(db);
}

void SaveDevicesToDatabase()
{
	char* zErrMsg = 0;
	int rc;
	const char* sql_statement = "REPLACE INTO DEVICES(DEVICE_INDEX, NAME) VALUES (?, ?)";
	sqlite3_stmt* prepared_statement = NULL;
	rc = sqlite3_open(PathToDatabaseUTF8.string, &db);

	if (rc) {
		PrintToErrorLog(L"SQL Error: Can't open database");
		return;
	}

	for (int i = 0; i < 4; i++)
	{
		rc = sqlite3_prepare_v2(db, sql_statement, -1, &prepared_statement, NULL);
		if (rc != SQLITE_OK) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}

		rc = sqlite3_bind_int(prepared_statement, 1, i);
		if (rc != SQLITE_OK) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}

		if (i < MAX_PLAYBACK_DEVICES)
		{
			rc = sqlite3_bind_text(prepared_statement, 2, PlaybackEngines[i].deviceName, -1, NULL);
			if (rc != SQLITE_OK) {
				PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
				sqlite3_free(zErrMsg);
			}
		}

		else if (i == 2)
		{
			rc = sqlite3_bind_text(prepared_statement, 2, CaptureEngine.captureDeviceName, -1, NULL);
			if (rc != SQLITE_OK) {
				PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
				sqlite3_free(zErrMsg);
			}
		}

		else if (i == 3)
		{
			rc = sqlite3_bind_text(prepared_statement, 2, CaptureEngine.duplexDeviceName, -1, NULL);
			if (rc != SQLITE_OK) {
				PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
				sqlite3_free(zErrMsg);
			}
		}

		rc = sqlite3_step(prepared_statement);
		if (rc != SQLITE_DONE) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}

		rc = sqlite3_finalize(prepared_statement);
		if (rc != SQLITE_OK) {
			PrintToErrorLog((const wchar_t*)sqlite3_errmsg16(db));
			sqlite3_free(zErrMsg);
		}
	}

	sqlite3_close(db);
}

void SelectCaptureDevice()
{
	CenterNextWindow(500.0f, 500.0f);
	ImGui::Begin("Select Recording Device (max 1)", &ShowCaptureDeviceList);
	if (ShowDuplexDevices == false)
	{
		ImGui::Text("First, select a recording device.\nThis is usually your microphone.");
		ImGui::NewLine();

		for (ma_uint32 i = 0; i < CaptureDeviceCount; i++)
		{
			ImGui::PushID(i);
			if (ImGui::Button(pCaptureDeviceInfos[i].name))
			{
				CaptureDeviceIndex = i;
				CaptureDeviceSelected[i] = true;
				ShowDuplexDevices = true;
			}
			ImGui::PopID();
		}
	}

	if (ShowDuplexDevices == true)
	{
		ImGui::Text("Now, select where this recording device should play through.\nThis is usually a virtual cable input.");
		ImGui::NewLine();

		if (NumActivePlaybackDevices == 0)
		{
			ImGui::Text("You need to select a playback device first.");
		}

		else
		{
			for (int i = 0; i < MAX_PLAYBACK_DEVICES; i++)
			{
				if (PlaybackEngines[i].active == true)
				{
					ImGui::PushID(i);
					if (ImGui::Button(&PlaybackEngines[i].deviceName[0]))
					{
						InitCaptureDevice(&pCaptureDeviceInfos[CaptureDeviceIndex].id, pCaptureDeviceInfos[CaptureDeviceIndex].name,
							&PlaybackEngines[i].device, PlaybackEngines[i].deviceName);
						ShowDuplexDevices = false;
						ShowCaptureDeviceList = false;
					}
					ImGui::PopID();
				}
			}
		}
	}
	ImGui::End();
}

void SelectPlaybackDevice()
{
	CenterNextWindow(500.0f, 500.0f);
	ImGui::Begin("Select Playback Device (max 2)", &ShowPlaybackDeviceList);
	ImGui::Text("Select where you want to hear sound (up to 2 devices).\nThis is usually your speakers and a virtual cable input.");
	ImGui::NewLine();
	for (uint32_t i = 0; i < PlaybackDeviceCount; i += 1)
	{
		if (PlaybackDeviceSelected[i] == false)
		{
			ImGui::PushID(i);
			if (ImGui::Button(pPlaybackDeviceInfos[i].name))
			{
				PlaybackDeviceSelected[i] = true;
				for (int j = 0; j < MAX_PLAYBACK_DEVICES; j++)
				{
					if (PlaybackEngines[j].active == false)
					{
						InitPlaybackDevice(&pPlaybackDeviceInfos[i].id, j, pPlaybackDeviceInfos[i].name);
						break;
					}
				}
			}
			ImGui::PopID();
		}
	}
	ImGui::End();
}

void SetVolume()
{
	for (int i = 0; i < MAX_PLAYBACK_DEVICES; i++)
	{
		if (PlaybackEngines[i].active == true)
		{
			ma_engine_set_volume(&PlaybackEngines[i].engine, VolumeDisplay / 100.0f);
		}
	}
}

void SQLiteCopyColumnText(sqlite3_stmt* stmt, int iRow, int iColumn, const wchar_t* const columnName, StringUTF8* destination)
{
	const char* textPtr = (const char*)sqlite3_column_text(stmt, iColumn);
	int bytesOfText = sqlite3_column_bytes(stmt, iColumn);
	int bytesToWrite = bytesOfText + 1; // The count of bytes returned by SQLite does not include the zero terminator, so add 1
	int bytesInDestination = sizeof(char) * destination->numChars;
	if (bytesInDestination < bytesToWrite)
	{
		const size_t errMsgBufferSize = 1024;
		wchar_t errMsgBuffer[errMsgBufferSize] = { 0 };
		_snwprintf(errMsgBuffer, errMsgBufferSize, L"The size for %s at row %d (%d) was less than the required size returned by the database (%d)", columnName, iRow, bytesInDestination, bytesToWrite);
		PrintToErrorLogAndExit(errMsgBuffer);
	}

	memcpy(destination->string, textPtr, bytesToWrite);
}

void SQLiteCopyColumnText16(sqlite3_stmt* stmt, int iRow, int iColumn, const wchar_t* const columnName, StringUTF16* destination)
{
	const wchar_t* textPtr = (const wchar_t*)sqlite3_column_text16(stmt, iColumn);
	int bytesOfText = sqlite3_column_bytes16(stmt, iColumn);
	int bytesToWrite = bytesOfText + 1; // The count of bytes returned by SQLite does not include the zero terminator, so add 1
	int bytesInDestination = sizeof(wchar_t) * destination->numWideChars;
	if (bytesInDestination < bytesToWrite)
	{
		const size_t errMsgBufferSize = 1024;
		wchar_t errMsgBuffer[errMsgBufferSize] = { 0 };
		_snwprintf(errMsgBuffer, errMsgBufferSize, L"The size for %s at row %d (%d) was less than the required size returned by the database (%d)", columnName, iRow, bytesInDestination, bytesToWrite);
		PrintToErrorLogAndExit(errMsgBuffer);
	}
	memcpy(destination->string, textPtr, bytesToWrite);
}

void UnloadSound(int iSound)
{
	for (int i = 0; i < MAX_PLAYBACK_DEVICES; i++)
	{
		if (SoundLoaded[i][iSound] == true)
		{
			ma_sound_uninit(&PlaybackEngines[i].sounds[iSound]);
			SoundLoaded[i][iSound] = false;
		}
	}
}

