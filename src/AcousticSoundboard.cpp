#include "AcousticSoundboard.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_  LPSTR lpCmdLine, _In_  int nCmdShow)
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, Win32Callback, 0L, 0L,
		GetModuleHandleW(NULL), LoadIcon(hInstance, MAKEINTRESOURCE(101)),
		NULL, NULL, NULL, (L"Acoustic Soundboard"), NULL };
	RegisterClassEx(&wc);
	HWND hwnd = CreateWindow(wc.lpszClassName, (L"Acoustic Soundboard"),
		WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	if (!CreateDeviceD3D(hwnd))
		return 1;

	if (InitAudioSystem() < 0) 
	{
		// Handle error
	}

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyboardHookCallback, GetModuleHandle(NULL), 0);
	if (KeyboardHook == NULL) 
		PrintToLog("log-error.txt", "SetWindowsHookEx failed");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
	MainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 18.0f);

	InitSQLite();
	LoadHotkeysFromDatabase();
	LoadDevicesFromDatabase();

	IMGUI_CHECKVERSION();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX9_Init(g_pd3dDevice);
	MainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 18.0f);

	while (WindowShouldClose == false) 
	{ // Main loop
		Update();
	} 
	// Close
	
	SaveHotkeysToDatabase();
	SaveDevicesToDatabase();
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CloseAudioSystem();
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
			{ // These fall through. We are purposefully ignoring these keys.
			case VK_CONTROL:
			case VK_SHIFT:
			case VK_MENU:
			case VK_UP:
			case VK_RIGHT:
			case VK_DOWN:
			case VK_LEFT:
			case VK_RETURN:
			case VK_ESCAPE:
			case VK_BACK:
				ignoreKey = true;
				break;
			case VK_TAB: strcpy(CapturedKeyText, "TAB");
				break;
			case VK_F1: strcpy(CapturedKeyText, "F1");
				break;
			case VK_F2: strcpy(CapturedKeyText, "F2");
				break;
			case VK_F3: strcpy(CapturedKeyText, "F3");
				break;
			case VK_F4: strcpy(CapturedKeyText, "F4");
				break;
			case VK_F5: strcpy(CapturedKeyText, "F5");
				break;
			case VK_F6: strcpy(CapturedKeyText, "F6");
				break;
			case VK_F7: strcpy(CapturedKeyText, "F7");
				break;
			case VK_F8: strcpy(CapturedKeyText, "F8");
				break;
			case VK_F9: strcpy(CapturedKeyText, "F9");
				break;
			case VK_F10: strcpy(CapturedKeyText, "F10");
				break;
			case VK_F11: strcpy(CapturedKeyText, "F11");
				break;
			case VK_F12: strcpy(CapturedKeyText, "F12");
				break;
			default:
				GetKeyNameTextW(lParam, (LPWSTR)CapturedKeyText, sizeof(char) * MAX_PATH);
				break;
			}

			if (ignoreKey == false)
			{
				CapturedKeyCode = wParam;
				CapturedKeyMod = 0;
				CapturedKeyModText[0] = '\0';
				bool altDown = (GetAsyncKeyState(VK_MENU) < 0);
				bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) < 0);
				bool shiftDown = (GetAsyncKeyState(VK_SHIFT) < 0);
				if (shiftDown) {
					strcpy(CapturedKeyModText, "SHIFT +");
					CapturedKeyMod = VK_SHIFT;
				}
				else if (ctrlDown) {
					strcpy(CapturedKeyModText, "CTRL +");
					CapturedKeyMod = VK_CONTROL;
				}
				else if (altDown) {
					strcpy(CapturedKeyModText, "ALT +");
					CapturedKeyMod = VK_MENU;
				}
			}
		}
		break;
	}
	case WM_SIZE:
	{
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			ResetDeviceD3D();
		}
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

void ClearCapturedKey() 
{
	CapturedKeyCode = 0;
	CapturedKeyMod = 0;
	CapturedKeyText[0] = '\0';
	CapturedKeyModText[0] = '\0';
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
		ShowDuplexDevices = false;
		NumActiveCaptureDevices = 0;
		CaptureEngine.captureDeviceName[0] = '\0';
		CaptureEngine.duplexDeviceName[0] = '\0';

		for (ma_uint32 i = 0; i < CaptureDeviceCount; i++)
			CaptureDeviceSelected[i] = false;

		NumActiveCaptureDevices = 0;
	}
}

void ClosePlaybackDevices()
{
	for (int i = 0; i < NumActivePlaybackDevices; i++)
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
		PlaybackEngines[i].deviceName[0] = '\0';
	}

	NumActivePlaybackDevices = 0;

	for (ma_uint32 i = 0; i < PlaybackDeviceCount; i++)
		PlaybackDeviceSelected[i] = false;
}

bool CreateDeviceD3D(HWND hWnd)
{
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
		return false;

	return true;
}

void DrawGUI() 
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::Begin("Audio", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);
	// ---------- Hotkeys table ----------
	ImGui::BeginTable("Sounds", 3, GUITableFlags);
	ImGui::TableSetupColumn(" #  Hotkey", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("File");
	ImGui::TableSetupColumn("Reset file", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableHeadersRow();
	int firstElement;
	int lastElement;
	switch (CurrentPage) {
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
	for (int i = firstElement; i <= lastElement; i++) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("%02d", i + 1);
		ImGui::SameLine();
		ImGui::PushID(i);
		if (ImGui::Button("Set Hotkey") == true || (ImGui::IsItemFocused() && UserPressedReturn)) {
			CaptureKeys = true;
			CapturedKeyIndex = i;
			ShowKeyCaptureWindow = true;
		}
		ImGui::PopID();

		ImGui::SameLine();
		ImGui::Text("%s", Hotkeys[i].modText);
		ImGui::SameLine();
		ImGui::Text("%s", Hotkeys[i].keyText);
		ImGui::TableNextColumn();
		ImGui::PushID(i);
		if (ImGui::Button("Select file") == true || (ImGui::IsItemFocused() && UserPressedReturn)) 
		{
			UnloadSound(i);
			const char* const filterPatterns[] = { "*.wav", "*.mp3", "*.ogg", "*.flac" };
			const char* AudioFilePath = tinyfd_openFileDialog("Choose a file", NULL, 4, filterPatterns, NULL, 0);
			if (AudioFilePath != NULL) 
			{
				strcpy(Hotkeys[i].filePath, AudioFilePath);
				GetFileNameFromPath(Hotkeys[i].fileName, AudioFilePath);
			}
		}
		ImGui::PopID();
		ImGui::SameLine();
		ImGui::Text(Hotkeys[i].fileName);
		ImGui::TableNextColumn();
		ImGui::PushID(i);
		if (ImGui::Button("Reset") == true) 
		{
			Hotkeys[i].fileName[0] = '\0';
			Hotkeys[i].filePath[0] = '\0';

			for (int j = 0; j < NumActivePlaybackDevices; j++)
			{
				UnloadSound(i);
			}
		}
		ImGui::PopID();
	}
	ImGui::EndTable(); // ---------- END Hotkeys table ----------
	

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

	if (ImGui::Button("Stop All Sounds") == true) 
	{
		for (int i = 0; i < NUM_SOUNDS; i++)
			UnloadSound(i);
	}
	ImGui::SameLine();
	ImGui::Text("Pause | Break");
	ImGui::NewLine();
	ImGui::BeginTable("Playback Devices", 1);
	ImGui::TableSetupColumn("Playback Devices");
	ImGui::TableHeadersRow();
	for (int i = 0; i < PlaybackDeviceCount; i++)
	{
		if (PlaybackDeviceSelected[i] == true)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("%s", pPlaybackDeviceInfos[i].name);
		}
	}
	ImGui::EndTable();
	if (NumActivePlaybackDevices < MAX_PLAYBACK_DEVICES)
	{
		if (ImGui::Button("Select Playback Device"))
			ShowPlaybackDeviceList = true;
	}
	else
	{
		ShowPlaybackDeviceList = false;
	}

	if (ShowPlaybackDeviceList)
		SelectPlaybackDevice();

	if (NumActivePlaybackDevices > 0 || NumActiveCaptureDevices > 0)
	{
		if (ImGui::Button("Close Devices"))
		{
			ClosePlaybackDevices();
			CloseCaptureDevice();
		}
	}

	ImGui::BeginTable("Capture Device", 1);
	ImGui::TableSetupColumn("Capture Device");
	ImGui::TableHeadersRow();
	for (int i = 0; i < CaptureDeviceCount; i++)
	{
		if (CaptureDeviceSelected[i] == true)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("%s", pCaptureDeviceInfos[i].name);
		}
	}
	if (NumActiveCaptureDevices > 0)
	{
		ImGui::NewLine();
		ImGui::Text("This device is playing through");
		ImGui::Text("%s", &PlaybackEngines[DuplexDeviceIndex].device.playback.name[0]);
	}
	ImGui::EndTable();

	if (NumActiveCaptureDevices <= 0)
	{
		if (ImGui::Button("Select Recording Device"))
			ShowCaptureDeviceList = true;

		if (ShowCaptureDeviceList)
			SelectCaptureDevice();
	}

	// ---------- Key Capture Window ----------
	if (ShowKeyCaptureWindow == true) {
		ImGui::SetNextWindowSizeConstraints(ImVec2(500.0f, 500.0f), ImGui::GetMainViewport()->Size, NULL, NULL);
		ImGui::Begin("Set Hotkey", &ShowKeyCaptureWindow);
		ImGui::Text("SHIFT, CTRL, ALT combos are supported");
		ImGui::NewLine();
		ImGui::Text("Press a key . . .");
		ImGui::Text("%s", CapturedKeyModText);
		ImGui::Text("%s", CapturedKeyText);
		ImGui::NewLine();
		ImGui::Text("%s", "ENTER to set");
		ImGui::Text("%s", "BACKSPCE to clear");
		ImGui::NewLine();
		if (UserPressedEscape)
			ShowKeyCaptureWindow = false;

		if (ImGui::Button("Set") == true || ImGui::IsItemFocused() && UserPressedReturn) 
		{
			if (CapturedKeyCode == 0) 
			{
				Hotkeys[CapturedKeyIndex].keyCode = 0;
				Hotkeys[CapturedKeyIndex].keyMod = 0;
				Hotkeys[CapturedKeyIndex].keyText[0] = '\0';
				Hotkeys[CapturedKeyIndex].modText[0] = '\0';
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
					strcpy(Hotkeys[CapturedKeyIndex].keyText, (const char*)CapturedKeyText);
					Hotkeys[CapturedKeyIndex].keyCode = CapturedKeyCode;
					strcpy(Hotkeys[CapturedKeyIndex].modText, CapturedKeyModText);
					Hotkeys[CapturedKeyIndex].keyMod = CapturedKeyMod;
				}
			}

			CaptureKeys = false;
			ShowKeyCaptureWindow = false;
			ClearCapturedKey();
		}

		ImGui::NewLine();
		if (ImGui::Button("Clear") == true || UserPressedBackspace ||
			(ImGui::IsItemFocused() && UserPressedReturn)) {
			Hotkeys[CapturedKeyIndex].keyText[0] = '\0';
			Hotkeys[CapturedKeyIndex].keyCode = 0;
			Hotkeys[CapturedKeyIndex].keyMod = 0;
			Hotkeys[CapturedKeyIndex].modText[0] = '\0';
			ClearCapturedKey();
		}
		ImGui::End();
	}

	CaptureKeys = ShowKeyCaptureWindow;
	// ----------END Key Capture Window ----------

	// ---------- Key In Use Window ----------
	if (CapturedKeyInUse == true) {
		ImGui::Begin("Invalid Key", &CapturedKeyInUse, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Key already in use.\nClose this window and try again.");
		if (ImGui::Button(" OK ") == true || UserPressedEscape ||
			ImGui::IsItemFocused() && UserPressedReturn)
		{
			CapturedKeyInUse = false;
			CaptureKeys = false;
		}
		ClearCapturedKey();
		ImGui::End();
	} // ----------END Key In Use Window ----------

	ImGui::End(); // End main window
	ImGui::EndFrame();
	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
	if (g_pd3dDevice->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		g_pd3dDevice->EndScene();
	}

	HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
	if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDeviceD3D();
}

void DuplexDeviceCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	MA_ASSERT(pDevice->capture.format == pDevice->playback.format);
	MA_ASSERT(pDevice->capture.channels == pDevice->playback.channels);

	MA_COPY_MEMORY(pOutput, pInput, frameCount * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels));
}

char* GetFileNameFromPath(char* const aoDestination, char const* const aSource) {
	/* copy the last name after '/' or '\' */
	char const* lTmp;
	if (aSource) {
		lTmp = strrchr(aSource, '/');
		if (!lTmp) {
			lTmp = strrchr(aSource, '\\');
		}
		if (lTmp) {
			strcpy(aoDestination, lTmp + 1);
		}
		else {
			strcpy(aoDestination, aSource);
		}
	}
	else {
		*aoDestination = '\0';
	}
	return aoDestination;
}

int InitAudioSystem()
{
	ma_resource_manager_config resourceManagerConfig;
	resourceManagerConfig = ma_resource_manager_config_init();
	resourceManagerConfig.decodedFormat = ma_format_f32;
	resourceManagerConfig.decodedChannels = 0;
	resourceManagerConfig.decodedSampleRate = 48000;

	ma_result result;
	result = ma_resource_manager_init(&resourceManagerConfig, &ResourceManager);
	if (result != MA_SUCCESS)
		return -1;

	result = ma_context_init(NULL, 0, NULL, &Context);
	if (result != MA_SUCCESS)
		return -2;

	result = ma_context_get_devices(&Context, &pPlaybackDeviceInfos, &PlaybackDeviceCount,
		&pCaptureDeviceInfos, &CaptureDeviceCount);
	if (result != MA_SUCCESS)
	{
		ma_context_uninit(&Context);
		return -3;
	}

	PlaybackDeviceSelected = (bool*)malloc(sizeof(bool) * PlaybackDeviceCount);
	for (ma_uint32 i = 0; i < PlaybackDeviceCount; i++)
		PlaybackDeviceSelected[i] = false;
	CaptureDeviceSelected = (bool*)malloc(sizeof(bool) * CaptureDeviceCount);
	for (ma_uint32 i = 0; i < CaptureDeviceCount; i++)
		CaptureDeviceSelected[i] = false;

	return 0;
}

void InitCaptureDevice(ma_device_id* captureId, ma_device* duplexDevice)
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

	if (ma_device_init(&Context, &deviceConfig, &CaptureEngine.device) != MA_SUCCESS)
	{
		// Handle error
	}

	engineConfig = ma_engine_config_init();
	engineConfig.pDevice = &CaptureEngine.device;
	engineConfig.pResourceManager = &ResourceManager;
	engineConfig.noAutoStart = MA_TRUE;

	if (ma_engine_init(&engineConfig, &CaptureEngine.engine) != MA_SUCCESS)
	{
		ma_device_uninit(&CaptureEngine.device);
		// Handle error
	}

	if (ma_engine_start(&CaptureEngine.engine) != MA_SUCCESS)
	{
		// Handle error failed to start engine
	}

	NumActiveCaptureDevices++;
}

void InitPlaybackDevice(ma_device_id* deviceId)
{
	ma_device_config deviceConfig;
	ma_engine_config engineConfig;
	deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.pDeviceID = deviceId;
	deviceConfig.playback.format = ResourceManager.config.decodedFormat;
	deviceConfig.playback.channels = 0;
	deviceConfig.sampleRate = ResourceManager.config.decodedSampleRate;
	deviceConfig.dataCallback = AudioCallback;
	deviceConfig.pUserData = &PlaybackEngines[NumActivePlaybackDevices].engine;

	ma_result result = ma_device_init(&Context, &deviceConfig, &PlaybackEngines[NumActivePlaybackDevices].device);
	if (result != MA_SUCCESS) {
		// Handle error
	}

	engineConfig = ma_engine_config_init();
	engineConfig.pDevice = &PlaybackEngines[NumActivePlaybackDevices].device;
	engineConfig.pResourceManager = &ResourceManager;
	engineConfig.noAutoStart = MA_TRUE;

	result = ma_engine_init(&engineConfig, &PlaybackEngines[NumActivePlaybackDevices].engine);
	if (result != MA_SUCCESS) {
		// Handle error
		ma_device_uninit(&PlaybackEngines[NumActivePlaybackDevices].device);
	}

	result = ma_engine_start(&PlaybackEngines[NumActivePlaybackDevices].engine);
	if (result != MA_SUCCESS) {
		// Handle error
	}
	NumActivePlaybackDevices++;
}

void InitSQLite() {
	//---------- Create hotkeys table ----------
	char* zErrMsg = 0;
	int rc;
	const char* sql_statement =
		"CREATE TABLE IF NOT EXISTS HOTKEYS (KEY_INDEX INTEGER PRIMARY KEY, KEY_MOD INTEGER, KEY INTEGER, MOD_TEXT TEXT, KEY_TEXT TEXT, FILE_PATH TEXT, FILE_NAME TEXT);";
	sqlite3_stmt* prepared_statement = NULL;
	rc = sqlite3_open("hotkeys.db", &db);

	if (rc) {
		PrintToLog("log-error.txt", "SQL Error: Can't open database to create hotkeys table");
		sqlite3_close(db);
	}

	rc = sqlite3_prepare_v2(db, sql_statement, -1, &prepared_statement, NULL);
	if (rc != SQLITE_OK) {
		PrintToLog("log-error.txt", sqlite3_errmsg(db));
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_step(prepared_statement);
	if (rc != SQLITE_DONE) {
		PrintToLog("log-error.txt", sqlite3_errmsg(db));
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_finalize(prepared_statement);
	if (rc != SQLITE_OK) {
		PrintToLog("log-error.txt", sqlite3_errmsg(db));
		sqlite3_free(zErrMsg);
	}

	sqlite3_close(db);

	//---------- Create playback devices table ----------
	zErrMsg = 0;
	rc = 0;
	sql_statement =
		"CREATE TABLE IF NOT EXISTS DEVICES (DEVICE_INDEX INTEGER PRIMARY KEY, NAME TEXT);";
	prepared_statement = NULL;
	rc = sqlite3_open("hotkeys.db", &db);

	if (rc) {
		PrintToLog("log-error.txt", "SQL Error: Can't open database to create playback devices table");
		sqlite3_close(db);
	}

	rc = sqlite3_prepare_v2(db, sql_statement, -1, &prepared_statement, NULL);
	if (rc != SQLITE_OK) {
		PrintToLog("log-error.txt", sqlite3_errmsg(db));
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_step(prepared_statement);
	if (rc != SQLITE_DONE) {
		PrintToLog("log-error.txt", sqlite3_errmsg(db));
		sqlite3_free(zErrMsg);
	}

	rc = sqlite3_finalize(prepared_statement);
	if (rc != SQLITE_OK) {
		PrintToLog("log-error.txt", sqlite3_errmsg(db));
		sqlite3_free(zErrMsg);
	}

	sqlite3_close(db);
}

LRESULT CALLBACK KeyboardHookCallback(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam) 
{
	// If this code is less than zero, we must pass the message to the next hook
	if (nCode >= 0)
	{
		// Else we are checking for hotkey matches
		if (CaptureKeys == false)
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
				for (size_t i = 0; i < NUM_SOUNDS; i++) {
					if (Hotkeys[i].keyCode != NULL) {
						if (Hotkeys[i].keyMod != NULL) {
							if (p->vkCode == Hotkeys[i].keyCode) {
								if (shiftDown) {
									if (Hotkeys[i].keyMod == VK_SHIFT)
										hotkeyFound = true;
								}
								else if (ctrlDown) {
									if (Hotkeys[i].keyMod == VK_CONTROL)
										hotkeyFound = true;
								}
								else if (altDown) {
									if (Hotkeys[i].keyMod == VK_MENU)
										hotkeyFound = true;
								}
							}
						}
						// Else if there is no modifier
						else if ((p->vkCode == Hotkeys[i].keyCode) &&
							altDown == false && ctrlDown == false && shiftDown == false) {
							hotkeyFound = true;
						}

						if (hotkeyFound == true) {
							for (int j = 0; j < NumActivePlaybackDevices; j++)
							{
								PlayAudio(j, i, Hotkeys[i].filePath);
							}
							break; // Break out of the "sound search" for loop
						}
					}
				}
			} // End WM_KEYUP
			default:
				break;
			} // End wParam switch (message)
		}
	}

	return CallNextHookEx(0, nCode, wParam, lParam);
}

void LoadDevicesFromDatabase() {
	char* errorMessage = NULL;
	int rc = sqlite3_open("hotkeys.db", &db);

	if (rc) 
	{
		PrintToLog("log-error.txt", "SQL Error: Can't open database for loading devices");
	}

	else 
	{
		sqlite3_stmt* stmt;
		const char* sql = "SELECT * FROM DEVICES";
		int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
		if (rc != SQLITE_OK) 
		{
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(errorMessage);
			return;
		}

		int rowCounter = 0;
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
			if (rowCounter < MAX_PLAYBACK_DEVICES) 
			{
				strcpy(PlaybackEngines[rowCounter].deviceName, (const char*)sqlite3_column_text(stmt, 1));
				rowCounter++;
			}
			else if (rowCounter == 2)
			{
				strcpy(CaptureEngine.captureDeviceName, (const char*)sqlite3_column_text(stmt, 1));
				rowCounter++;
			}
			else if (rowCounter == 3)
			{
				strcpy(CaptureEngine.duplexDeviceName, (const char*)sqlite3_column_text(stmt, 1));
				rowCounter++;
			}
		}

		if (rc != SQLITE_DONE) 
		{
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(errorMessage);
		}
		sqlite3_finalize(stmt);

	}
	sqlite3_close(db);

	
	for (ma_uint32 i = 0; i < PlaybackDeviceCount; i++)
	{
		for (int j = 0; j < MAX_PLAYBACK_DEVICES; j++)
		{
			if (strcmp(pPlaybackDeviceInfos[i].name, PlaybackEngines[j].deviceName) == 0)
			{
				PlaybackDeviceSelected[i] = true;
				InitPlaybackDevice(&pPlaybackDeviceInfos[i].id);
				break;
			}
		}

		if (NumActivePlaybackDevices == MAX_PLAYBACK_DEVICES)
		{
			break;
		}
	}

	DuplexDeviceIndex = -1;
	for (int i = 0; i < NumActivePlaybackDevices; i++)
	{
		if (strcmp(PlaybackEngines[i].deviceName, CaptureEngine.duplexDeviceName) == 0)
		{
			DuplexDeviceIndex = i;
			break;
		}
	}

	for (ma_uint32 i = 0; i < CaptureDeviceCount; i++)
	{
		if (strcmp(pCaptureDeviceInfos[i].name, CaptureEngine.captureDeviceName) == 0)
		{
			CaptureDeviceSelected[i] = true;
			if (DuplexDeviceIndex >= 0)
			{
				InitCaptureDevice(&pCaptureDeviceInfos[i].id, &PlaybackEngines[DuplexDeviceIndex].device);
			}
			else
			{
				PrintToLog("log-error.txt", "Failed to initialize capture device. Duplex device index invalid.");
			}
			break;
		}
	}
}

void LoadHotkeysFromDatabase() {
	char* errorMessage = NULL;
	int rc = sqlite3_open("hotkeys.db", &db);

	if (rc) 
	{
		PrintToLog("log-error.txt", "SQL Error: Can't open database for loading hotkeys");
	}

	else 
	{
		sqlite3_stmt* stmt;
		const char* sql = "SELECT * FROM HOTKEYS";
		int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
		if (rc != SQLITE_OK) 
		{
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(errorMessage);
			return;
		}

		int rowCounter = 0;
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
			if (rowCounter < MAX_SOUNDS) 
			{
				Hotkeys[rowCounter].keyMod = sqlite3_column_int(stmt, 1);
				Hotkeys[rowCounter].keyCode = sqlite3_column_int(stmt, 2);
				strcpy(Hotkeys[rowCounter].modText, (const char*)sqlite3_column_text(stmt, 3));
				strcpy(Hotkeys[rowCounter].keyText, (const char*)sqlite3_column_text(stmt, 4));
				strcpy(Hotkeys[rowCounter].filePath, (const char*)sqlite3_column_text(stmt, 5));
				strcpy(Hotkeys[rowCounter].fileName, (const char*)sqlite3_column_text(stmt, 6));
				rowCounter++;
			}
		}

		if (rc != SQLITE_DONE) 
		{
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(errorMessage);
		}
		sqlite3_finalize(stmt);

	}
	sqlite3_close(db);
}

void PlayAudio(int iEngine, int iSound, const char* filePath)
{
	if (SoundLoaded[iEngine][iSound] == false)
	{
		ma_uint32 flags = MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE |
			MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_ASYNC |
			MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_STREAM;

		ma_result result = ma_sound_init_from_file(&PlaybackEngines[iEngine].engine,
			filePath, flags, NULL, NULL, &PlaybackEngines[iEngine].sounds[iSound]);

		if (result == MA_SUCCESS)
		{
			SoundLoaded[iEngine][iSound] = true;
		}
		else
		{
			// Handle error
		}
	}

	ma_data_source_seek_to_pcm_frame(PlaybackEngines[iEngine].sounds[iSound].pDataSource, 0);

	if (ma_sound_start(&PlaybackEngines[iEngine].sounds[iSound]) != MA_SUCCESS)
	{
		// Handle error failed to start sound
	}
}

void PrintToLog(const char* fileName, const char* logText) {
	SYSTEMTIME Time = {};
	GetLocalTime(&Time);
	FILE* file = 0;
	if (file = fopen(fileName, "a")) {
		fprintf(file, "%u%s%u%s%u%s%u%s%u%s%u%s%s",
			Time.wYear, "-", Time.wMonth, "-", Time.wDay, " ", Time.wHour, ":", Time.wMinute, ":", Time.wSecond, " ", logText);
		fprintf(file, "\n");
		fclose(file);
	}
}

void ResetDeviceD3D()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}

void ResetNavKeys() {
	if (UserPressedReturn)
		UserPressedReturn = false;
	if (UserPressedEscape)
		UserPressedEscape = false;
}

void SaveHotkeysToDatabase() 
{
	char* zErrMsg = 0;
	int rc;
	const char* sql_statement = "REPLACE INTO HOTKEYS(KEY_INDEX, KEY_MOD, KEY, MOD_TEXT, KEY_TEXT, FILE_PATH, FILE_NAME) VALUES (?, ?, ?, ?, ?, ?, ?)";
	sqlite3_stmt* prepared_statement = NULL;
	rc = sqlite3_open("hotkeys.db", &db);

	if (rc) {
		PrintToLog("log-error.txt", "SQL Error: Can't open database");
		sqlite3_close(db);
	}

	for (size_t i = 0; i < NUM_SOUNDS; i++) {
		rc = sqlite3_prepare_v2(db, sql_statement, -1, &prepared_statement, NULL);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}
		
		rc = sqlite3_bind_int(prepared_statement, 1, i);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_int(prepared_statement, 2, Hotkeys[i].keyMod);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_int(prepared_statement, 3, Hotkeys[i].keyCode);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_text(prepared_statement, 4, Hotkeys[i].modText, -1, NULL);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_text(prepared_statement, 5, Hotkeys[i].keyText, -1, NULL);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_text(prepared_statement, 6, Hotkeys[i].filePath, -1, NULL);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_text(prepared_statement, 7, Hotkeys[i].fileName, -1, NULL);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}

		rc = sqlite3_step(prepared_statement);
		if (rc != SQLITE_DONE) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}

		rc = sqlite3_finalize(prepared_statement);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
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
	rc = sqlite3_open("hotkeys.db", &db);

	if (rc) {
		PrintToLog("log-error.txt", "SQL Error: Can't open database");
		sqlite3_close(db);
	}

	for (int i = 0; i < 4; i++)
	{
		rc = sqlite3_prepare_v2(db, sql_statement, -1, &prepared_statement, NULL);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}

		rc = sqlite3_bind_int(prepared_statement, 1, i);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}

		if (i < MAX_PLAYBACK_DEVICES)
		{
			rc = sqlite3_bind_text(prepared_statement, 2, PlaybackEngines[i].deviceName, -1, NULL);
			if (rc != SQLITE_OK) {
				PrintToLog("log-error.txt", sqlite3_errmsg(db));
				sqlite3_free(zErrMsg);
			}
		}

		else if (i == 2)
		{
			rc = sqlite3_bind_text(prepared_statement, 2, CaptureEngine.captureDeviceName, -1, NULL);
			if (rc != SQLITE_OK) {
				PrintToLog("log-error.txt", sqlite3_errmsg(db));
				sqlite3_free(zErrMsg);
			}
		}

		else if (i == 3)
		{
			rc = sqlite3_bind_text(prepared_statement, 2, CaptureEngine.duplexDeviceName, -1, NULL);
			if (rc != SQLITE_OK) {
				PrintToLog("log-error.txt", sqlite3_errmsg(db));
				sqlite3_free(zErrMsg);
			}
		}

		rc = sqlite3_step(prepared_statement);
		if (rc != SQLITE_DONE) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}

		rc = sqlite3_finalize(prepared_statement);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}
	}

	sqlite3_close(db);
}

void SelectCaptureDevice()
{
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

		if (NumActivePlaybackDevices > 0)
		{
			for (int i = 0; i < NumActivePlaybackDevices; i++)
			{
				ImGui::PushID(i);
				if (ImGui::Button(&PlaybackEngines[i].device.playback.name[0]))
				{
					InitCaptureDevice(&pCaptureDeviceInfos[CaptureDeviceIndex].id, &PlaybackEngines[i].device);
					strcpy(CaptureEngine.captureDeviceName, pCaptureDeviceInfos[CaptureDeviceIndex].name);
					strcpy(CaptureEngine.duplexDeviceName, PlaybackEngines[i].deviceName);
					DuplexDeviceIndex = i;
					ShowDuplexDevices = false;
					ShowCaptureDeviceList = false;
				}
				ImGui::PopID();
			}
		}

		else
		{
			ImGui::Text("You need to select a playback device first.");
		}

	}
	ImGui::End();
}

void SelectPlaybackDevice()
{
	ImGui::Begin("Select Playback Device (max 2)", &ShowPlaybackDeviceList);
	ImGui::Text("Select where you want to hear sound (up to 2 devices).\nThis is usually your speakers and a virtual cable input.");
	ImGui::NewLine();
	for (int i = 0; i < PlaybackDeviceCount; i += 1)
	{
		if (PlaybackDeviceSelected[i] == false)
		{
			ImGui::PushID(i);
			if (ImGui::Button(pPlaybackDeviceInfos[i].name))
			{
				PlaybackDeviceSelected[i] = true;
				strcpy(PlaybackEngines[NumActivePlaybackDevices].deviceName, pPlaybackDeviceInfos[i].name);
				InitPlaybackDevice(&pPlaybackDeviceInfos[i].id);
			}
			ImGui::PopID();
		}
	}
	ImGui::End();
}

void UnloadSound(int iSound)
{
	for (int i = 0; i < NumActivePlaybackDevices; i++)
	{
		if (SoundLoaded[i][iSound] == true)
		{
			ma_sound_uninit(&PlaybackEngines[i].sounds[iSound]);
			SoundLoaded[i][iSound] = false;
		}
	}
}

void Update() {
	MSG msg;
	while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DrawGUI();
	ResetNavKeys();
}