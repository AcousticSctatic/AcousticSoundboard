#include "AcousticSoundboard.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_  LPSTR lpCmdLine, _In_  int nCmdShow)
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, Win32Callback, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL, (L"Acoustic Soundboard"), NULL };
	RegisterClassEx(&wc);
	HWND hwnd = CreateWindow(wc.lpszClassName, (L"Acoustic Soundboard"),
		WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	if (!CreateDeviceD3D(hwnd))
		return 1;

	if (InitAudioSystem() < 0) {
		// Handle error
	}

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyboardHookCallback, GetModuleHandle(NULL), 0);

	if (KeyboardHook == NULL) {
		PrintToLog("log-error.txt", "SetWindowsHookEx failed");
	}

	// Initialize hotkeys indices
	for (int i = 0; i < NUM_SOUNDS; i++) {
		Hotkeys[i].sampleIndex = i;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
	MainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 18.0f);

	CurrentPage = 1;

	InitSQLite();
	LoadHotkeysFromDatabase();

	IMGUI_CHECKVERSION();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX9_Init(g_pd3dDevice);
	MainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 18.0f);

	while (WindowShouldClose == false) {
		Update();
	}

	// Close
	if (Autosave == true) {
		SaveHotkeysToDatabase();
	}

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

	wchar_t msg[32];
	switch (message)
	{		
	case WM_SYSKEYUP: // Falls through
	case WM_KEYUP:
	{
		// If we are in key capture mode, store the inputs
		/*
		if (CaptureKeys == true)
		{
			int returnValue = 0;
			returnValue = GetKeyNameText(lParam, CapturedKeyText, sizeof(char) * MAX_PATH);
			if (returnValue == 0)
			{
				DWORD error = GetLastError();
				PrintToLog("log-error.txt", "GetKeyNameText() failed");
			}
			*/

			/*
			if (shiftDown) {
				strcpy(CapturedKeyModText, "SHIFT + ");
				Win32CapturedKeyMod = VK_SHIFT;
			}
			else if (ctrlDown) {
				strcpy(CapturedKeyModText, "CTRL + ");
				Win32CapturedKeyMod = VK_CONTROL;
			}
			else if (altDown) {
				strcpy(CapturedKeyModText, "ALT + ");
				Win32CapturedKeyMod = VK_MENU;
			}
		}
		*/

		switch (wParam)
		{ // These fall through
		case VK_CONTROL:
		case VK_SHIFT:
		case VK_MENU:
		case VK_UP:
		case VK_RIGHT:
		case VK_DOWN:
		case VK_LEFT:
			break;
		case VK_RETURN:
			UserPressedReturn = true;
			break;
		case VK_ESCAPE:
			UserPressedEscape = true;
			break;
		case VK_BACK:
			UserPressedBackspace = true;
			break;
		default:
			if (CaptureKeys == true)
				CapturedKeyCode = wParam;
			break;
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
	case WM_CHAR:
	{
		swprintf_s(msg, L"WM_KEYDOWN: %c\n", wParam);
		OutputDebugString(msg);
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

void CloseAudioSystem()
{
	for (int i = 0; i < NumActivePlaybackDevices; i++)
	{
		if (PlaybackEngines[i].active == true)
		{
			ma_engine_uninit(&PlaybackEngines[i].engine);
			ma_device_uninit(&PlaybackEngines[i].device);

			for (int j = 0; j < MAX_SOUNDS; j++)
			{
				ma_sound_uninit(&PlaybackEngines[i].sounds[j]);
			}
		}
	}

	if (CaptureEngine.active == true)
	{
		ma_engine_uninit(&CaptureEngine.engine);
		ma_device_uninit(&CaptureEngine.device);
	}

	free(PlaybackDeviceSelected);
	free(CaptureDeviceSelected);
	ma_resource_manager_uninit(&ResourceManager);
}

void CloseCaptureDevice()
{
	if (CaptureEngine.active == true)
	{
		ma_engine_uninit(&CaptureEngine.engine);
		ma_device_uninit(&CaptureEngine.device);
		CaptureEngine.active = false;
		SelectDuplexDevice = false;
		NumActiveCaptureDevices = 0;
	}

	for (ma_uint32 i = 0; i < CaptureDeviceCount; i++)
		CaptureDeviceSelected[i] = false;
}

void ClosePlaybackDevices()
{
	for (int i = 0; i < NumActivePlaybackDevices; i++)
	{
		if (PlaybackEngines[i].active == true)
		{
			ma_engine_uninit(&PlaybackEngines[i].engine);
			ma_device_uninit(&PlaybackEngines[i].device);

			for (int j = 0; j < MAX_SOUNDS; j++)
			{
				if (SoundLoaded[i][j] == true)
				{
					ma_sound_uninit(&PlaybackEngines[i].sounds[j]);
					SoundLoaded[i][j] = false;
				}
			}

			PlaybackEngines[i].active = false;
		}
	}

	for (ma_uint32 i = 0; i < PlaybackDeviceCount; i++)
		PlaybackDeviceSelected[i] = false;

	NumActivePlaybackDevices = 0;
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

void DrawGUI() {
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
			if (UserPressedReturn) UserPressedReturn = false;
			CaptureKeys = true;
			CapturedKeyIndex = i;
			ShowKeyCaptureWindow = true;
		}
		ImGui::PopID();

		ImGui::SameLine();
		if (Hotkeys[i].modText != NULL) {
			ImGui::Text("%s", Hotkeys[i].modText);
		}
		ImGui::SameLine();
		if (Hotkeys[i].keyText != NULL) {
			ImGui::Text("%s", Hotkeys[i].keyText);
		}
		ImGui::TableNextColumn();
		ImGui::PushID(i);
		if (ImGui::Button("Select file") == true || (ImGui::IsItemFocused() && UserPressedReturn)) {
			//StopAllChannels();
			const char* const filterPatterns[] = { "*.wav", "*.mp3", "*.ogg", "*.flac" };
			const char* AudioFilePath = tinyfd_openFileDialog("Choose a file", NULL, 4, filterPatterns, NULL, 0);
			if (AudioFilePath != NULL) {
				strcpy(Hotkeys[i].filePath, AudioFilePath);
				GetFileNameFromPath(Hotkeys[i].fileName, AudioFilePath);
			}
		}
		ImGui::PopID();
		ImGui::SameLine();
		ImGui::Text(Hotkeys[i].fileName);
		ImGui::TableNextColumn();
		ImGui::PushID(i);
		if (ImGui::Button("Reset") == true) {
			if (Hotkeys[i].filePath[0] != '\0') {
				Hotkeys[i].fileName[0] = '\0';
				Hotkeys[i].filePath[0] = '\0';
			}
			//StopAllChannels();
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

	if (ImGui::Button("Stop All Sounds") == true) {
		//StopAllChannels();
	}
	ImGui::SameLine();
	ImGui::Text("Pause | Break");
	if (ImGui::Checkbox("Autosave", &Autosave) == true)

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

	if (NumActivePlaybackDevices > 0)
	{
		if (ImGui::Button("Close Playback Devices"))
			ClosePlaybackDevices();
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

	if (CaptureEngine.active == true)
	{
		if (ImGui::Button("Close Capture Device"))
			CloseCaptureDevice();
	}

	if (CaptureEngine.active == false)
	{
		if (ImGui::Button("Select Recording Device"))
			ShowCaptureDeviceList = true;
	}
	if (ShowCaptureDeviceList)
		SelectCaptureDevice();

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
		if (UserPressedEscape) {
			ShowKeyCaptureWindow = false;
			UserPressedEscape = false;
		}

		if (ImGui::Button("Set") == true || UserPressedReturn) {
			if (ImGui::IsItemFocused() && CapturedKeyText == NULL) {
				if (UserPressedReturn)
					UserPressedReturn = false;

				Hotkeys[CapturedKeyIndex].keyText[0] = '\0';
				Hotkeys[CapturedKeyIndex].modText[0] = '\0';
				Hotkeys[CapturedKeyIndex].sampleIndex = NULL;
				CaptureKeys = false;
				ShowKeyCaptureWindow = false;
			}

			if (CapturedKeyText != NULL) {
				for (int i = 0; i < NUM_SOUNDS; i++) {
					// Compare the captured key with all the stored hotkeys
					if (strcmp(Hotkeys[i].keyText, (const char*)CapturedKeyText) == 0) {
						// If they are the same, check the captured hotkey
						// If the captured key mod is NULL, then check the hotkey mod
						if (CapturedKeyModText == NULL) {
							// If the hotkey mod is also NULL, then the key is in use
							if (Hotkeys[i].modText == NULL) {
								CapturedKeyInUse = true;
								ShowKeyCaptureWindow = false;
							}

						}
						// Else check the hotkey mod. If they are the same, the key is in use
						else if (strcmp(Hotkeys[i].modText, CapturedKeyModText) == 0) {
							CapturedKeyInUse = true;
							ShowKeyCaptureWindow = false;
						}
					}
				}

				if (CapturedKeyInUse == false) {
					if (CapturedKeyCode != NULL) {
						Hotkeys[CapturedKeyIndex].sampleIndex = CapturedKeyIndex;
						strcpy(Hotkeys[CapturedKeyIndex].keyText, (const char*)CapturedKeyText);
						Hotkeys[CapturedKeyIndex].win32Key = CapturedKeyCode;
						if (Win32CapturedKeyMod != NULL) {
							Hotkeys[CapturedKeyIndex].sampleIndex = CapturedKeyIndex;
							strcpy(Hotkeys[CapturedKeyIndex].modText, CapturedKeyModText);
							Hotkeys[CapturedKeyIndex].win32KeyMod = Win32CapturedKeyMod;
						}
					}

					CaptureKeys = false;
					ShowKeyCaptureWindow = false;
				}
			}
		}
		ImGui::NewLine();
		if (ImGui::Button("Clear") == true || UserPressedBackspace ||
			(ImGui::IsItemFocused() && UserPressedReturn)) {
			if (UserPressedBackspace) UserPressedBackspace = false;
			if (UserPressedReturn) UserPressedReturn = false;
			Hotkeys[CapturedKeyIndex].keyText[0] = '\0';
			Hotkeys[CapturedKeyIndex].win32Key = NULL;
			Hotkeys[CapturedKeyIndex].win32KeyMod = NULL;
			Hotkeys[CapturedKeyIndex].modText[0] = '\0';
			Hotkeys[CapturedKeyIndex].sampleIndex = NULL;
			CapturedKeyCode = NULL;
			CapturedKeyText[0] = '\0';
			CapturedKeyModText[0] = '\0';
		}
		ImGui::End();
		if (UserPressedReturn) UserPressedReturn = false;
	}

	else {
		CapturedKeyCode = NULL;
		CapturedKeyText[0] = '\0';
		CapturedKeyModText[0] = '\0';
	}
	// ----------END Key Capture Window ----------

	// ---------- Key In Use Window ----------
	if (CapturedKeyInUse == true) {
		ImGui::Begin("Invalid Key", &CapturedKeyInUse, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Key already in use.\nClose this window and try again.");
		if (ImGui::Button(" OK ") == true || UserPressedReturn || UserPressedEscape) {
			if (UserPressedReturn) UserPressedReturn = false;
			if (UserPressedEscape) UserPressedEscape = false;
			CapturedKeyInUse = false;
			CaptureKeys = false;
		}
		CapturedKeyText[0] = '\0';;
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

	CaptureEngine.active = true;
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

	/* Now that we have the device we can initialize the engine. The device is passed into the engine's config. */
	engineConfig = ma_engine_config_init();
	engineConfig.pDevice = &PlaybackEngines[NumActivePlaybackDevices].device;
	engineConfig.pResourceManager = &ResourceManager;
	engineConfig.noAutoStart = MA_TRUE;    /* Don't start the engine by default - we'll do that manually below. */

	result = ma_engine_init(&engineConfig, &PlaybackEngines[NumActivePlaybackDevices].engine);
	if (result != MA_SUCCESS) {
		// Handle error
		ma_device_uninit(&PlaybackEngines[NumActivePlaybackDevices].device);
	}

	result = ma_engine_start(&PlaybackEngines[NumActivePlaybackDevices].engine);
	if (result != MA_SUCCESS) {
		// Handle error
	}
	PlaybackEngines[NumActivePlaybackDevices].active = true;

	NumActivePlaybackDevices++;
}

void InitSQLite() {
	char* zErrMsg = 0;
	int rc;
	const char* sql_statement =
		"CREATE TABLE IF NOT EXISTS HOTKEYS (SAMPLE_INDEX INTEGER PRIMARY KEY, KEY_MOD INTEGER, KEY INTEGER, MOD_TEXT TEXT, KEY_TEXT TEXT, FILE_PATH TEXT, FILE_NAME TEXT);";
	sqlite3_stmt* prepared_statement = NULL;
	rc = sqlite3_open("hotkeys.db", &db);

	if (rc) {
		PrintToLog("log-error.txt", "SQL Error: Can't open database");
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

LRESULT CALLBACK KeyboardHookCallback(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	// If this code is less than zero, we must pass the message to the next hook
	if (nCode >= 0)
	{
		// Else we are checking for hotkey matches
		if (CaptureKeys == false)
		{
			PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
			// TODO: Instead of calling GetAsyncKeyState we might check the wParam using GetKeyNameText()
			bool altDown = (GetAsyncKeyState(VK_MENU) < 0);
			bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) < 0);
			bool shiftDown = (GetAsyncKeyState(VK_SHIFT) < 0);

			switch (wParam) 
			{
			case WM_SYSKEYUP:
			case WM_KEYUP:
			{
				if (p->vkCode == VK_PAUSE) {
					//TODO StopAllChannels();
					break;
				}

				bool hotkeyFound = false;
				for (size_t i = 0; i < NUM_SOUNDS; i++) {
					if (Hotkeys[i].win32Key != NULL) {
						if (Hotkeys[i].win32KeyMod != NULL) {
							if (p->vkCode == Hotkeys[i].win32Key) {
								if (shiftDown) {
									if (Hotkeys[i].win32KeyMod == VK_SHIFT)
										hotkeyFound = true;
								}
								else if (ctrlDown) {
									if (Hotkeys[i].win32KeyMod == VK_CONTROL)
										hotkeyFound = true;
								}
								else if (altDown) {
									if (Hotkeys[i].win32KeyMod == VK_MENU)
										hotkeyFound = true;
								}
							}
						}
						// Else if there is no modifier
						else if ((p->vkCode == Hotkeys[i].win32Key) &&
							altDown == false && ctrlDown == false && shiftDown == false) {
							hotkeyFound = true;
						}

						if (hotkeyFound == true) {
							for (int j = 0; j < NumActivePlaybackDevices; j++)
							{
								if (PlaybackEngines[j].active == true)
								{
									PlayAudio(j, 0, Hotkeys[i].filePath);
								}
							}
							
							break; // Break out of the for loop
						}
					}
				}
			}
			default:
				break;
			}

		}
	}

	return CallNextHookEx(0, nCode, wParam, lParam);
}

void LoadHotkeysFromDatabase() {
	char* errorMessage = NULL;
	int rc = sqlite3_open("hotkeys.db", &db);

	if (rc) {
		PrintToLog("log-error.txt", "SQL Error: Can't open database");
	}

	else {
		sqlite3_stmt* stmt;
		const char* sql = "SELECT * FROM HOTKEYS";
		int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(errorMessage);
			return;
		}

		int rowCounter = 0;
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
			if (rowCounter < 50) {
				Hotkeys[rowCounter].sampleIndex = sqlite3_column_int(stmt, 0);
				Hotkeys[rowCounter].win32KeyMod = sqlite3_column_int(stmt, 1);
				Hotkeys[rowCounter].win32Key = sqlite3_column_int(stmt, 2);
				strcpy(Hotkeys[rowCounter].modText, (const char*)sqlite3_column_text(stmt, 3));
				strcpy(Hotkeys[rowCounter].keyText, (const char*)sqlite3_column_text(stmt, 4));
				strcpy(Hotkeys[rowCounter].filePath, (const char*)sqlite3_column_text(stmt, 5));
				strcpy(Hotkeys[rowCounter].fileName, (const char*)sqlite3_column_text(stmt, 6));
				rowCounter++;
			}
		}

		if (rc != SQLITE_DONE) {
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

		if (result != MA_SUCCESS)
		{
			// Handle error
		}

		SoundLoaded[iEngine][iSound] = true;
	}

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
	UserPressedEscape = false;
	UserPressedReturn = false;
}

void SaveHotkeysToDatabase() {
	char* zErrMsg = 0;
	int rc;
	const char* sql_statement = "REPLACE INTO HOTKEYS(SAMPLE_INDEX, KEY_MOD, KEY, MOD_TEXT, KEY_TEXT, FILE_PATH, FILE_NAME) VALUES (?, ?, ?, ?, ?, ?, ?)";
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

		rc = sqlite3_bind_int(prepared_statement, 1, Hotkeys[i].sampleIndex);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_int(prepared_statement, 2, Hotkeys[i].win32KeyMod);
		if (rc != SQLITE_OK) {
			PrintToLog("log-error.txt", sqlite3_errmsg(db));
			sqlite3_free(zErrMsg);
		}
		rc = sqlite3_bind_int(prepared_statement, 3, Hotkeys[i].win32Key);
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

void SelectCaptureDevice()
{
	ImGui::Begin("Select Recording Device (max 1)", &ShowCaptureDeviceList);
	if (SelectDuplexDevice == false)
	{
		ImGui::Text("First, select a recording device.\nThis is usually your microphone.");
		ImGui::NewLine();

		for (int i = 0; i < CaptureDeviceCount; i++)
		{
			if (CaptureDeviceSelected[i] == false)
			{
				ImGui::PushID(i);
				if (ImGui::Button(pCaptureDeviceInfos[i].name))
				{
					CaptureDeviceSelected[i] = true;
					InitCaptureDevice(&pCaptureDeviceInfos[i].id, &PlaybackEngines[DuplexDeviceIndex].device);
					SelectDuplexDevice = true;
				}
				ImGui::PopID();
			}
		}
	}

	else
	{
		ImGui::Text("Now, select where this recording device should play through.\nThis is usually a virtual cable input.");
		ImGui::NewLine();

		if (NumActivePlaybackDevices == 0)
		{
			if (ImGui::Button("Select Playback Device"))
				ShowPlaybackDeviceList = true;
		}

		for (int i = 0; i < NumActivePlaybackDevices; i++)
		{
			ImGui::PushID(i);
			if (ImGui::Button(&PlaybackEngines[i].device.playback.name[0]))
			{
				DuplexDeviceIndex = i;
				SelectDuplexDevice = false;
				NumActiveCaptureDevices++;
				ShowCaptureDeviceList = false;
			}
			ImGui::PopID();
		}
	}

	ImGui::End();
}

void SelectPlaybackDevice()
{
	ImGui::Begin("Select Playback Device (max 2)", &ShowPlaybackDeviceList);
	for (int i = 0; i < PlaybackDeviceCount; i += 1)
	{
		if (PlaybackDeviceSelected[i] == false)
		{
			ImGui::PushID(i);
			if (ImGui::Button(pPlaybackDeviceInfos[i].name))
			{
				PlaybackDeviceSelected[i] = true;
				InitPlaybackDevice(&pPlaybackDeviceInfos[i].id);
			}
			ImGui::PopID();
		}

	}
	ImGui::End();
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