#include "AcousticSoundboard.h"

int main(int argc, char** argv) {
	Initialize();

	while (WindowShouldClose == false) {
		Update();
	}

	Close();

	return 0;
}

void Initialize() {
	KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyboardHookCallback, GetModuleHandle(NULL), 0);

	if (KeyboardHook == NULL) {
		PrintToLog("log-error.txt", "SetWindowsHookEx failed");
	}

	// Initialize hotkeys indices
	for (int i = 0; i < NUM_SOUNDS; i++) {
		Hotkeys[i].sampleIndex = i;
		Samples[i] = NULL;
	}

	SDL_Init(SDL_INIT_EVERYTHING);
	// GL 3.0 + GLSL 130
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	Window = SDL_CreateWindow("Acoustic Soundboard", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

	GLContext = SDL_GL_CreateContext(Window);
	SDL_GL_MakeCurrent(Window, GLContext);
	SDL_GL_SetSwapInterval(1); // Enable vsync
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
	MainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 18.0f);
	ImGui_ImplSDL2_InitForOpenGL(Window, GLContext);
	const char* glsl_version = "#version 130";
	ImGui_ImplOpenGL3_Init(glsl_version);

	CurrentPage = 1;

	for (size_t i = 0; i < NUM_CHANNELS; i++) {
		ChannelStates[i].isAvailable = true;
	}

	Mix_Init(0);
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, NUM_CHANNELS, 4096);
	size_t channelsAllocated = Mix_AllocateChannels(NUM_CHANNELS);
	channelFinished = &ResetChannel;
	Mix_ChannelFinished(channelFinished);
	NumPlaybackDevices = SDL_GetNumAudioDevices(0);
	PlaybackDevices = (char**)malloc(sizeof(char*) * NumPlaybackDevices);
	if (PlaybackDevices != NULL) {
		for (int i = 0; i < NumPlaybackDevices; i++) {
			PlaybackDevices[i] = _strdup(SDL_GetAudioDeviceName(i, 0));
		}
		SelectedPlaybackDevice = PlaybackDevices[0];
	}

	InitSQLite();
	LoadHotkeysFromDatabase();
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

int ConvertSDLKeyToWin32Key(SDL_Keycode SDLK) {
	int vkey;
	switch (SDLK) {
	case SDLK_F1:
		vkey = VK_F1;
		break;
	case SDLK_F2:
		vkey = VK_F2;
		break;
	case SDLK_F3:
		vkey = VK_F3;
		break;
	case SDLK_F4:
		vkey = VK_F4;
		break;
	case SDLK_F5:
		vkey = VK_F5;
		break;
	case SDLK_F6:
		vkey = VK_F6;
		break;
	case SDLK_F7:
		vkey = VK_F7;
		break;
	case SDLK_F8:
		vkey = VK_F8;
		break;
	case SDLK_F9:
		vkey = VK_F9;
		break;
	case SDLK_F10:
		vkey = VK_F10;
		break;
	case SDLK_F11:
		vkey = VK_F11;
		break;
	case SDLK_F12:
		vkey = VK_F12;
		break;
	case SDLK_BACKQUOTE:
		vkey = VK_OEM_3;
		break;
	case SDLK_q:
		vkey = 'Q';
		break;
	case SDLK_w:
		vkey = 'W';
		break;
	case SDLK_e:
		vkey = 'E';
		break;
	case SDLK_r:
		vkey = 'R';
		break;
	case SDLK_t:
		vkey = 'T';
		break;
	case SDLK_y:
		vkey = 'Y';
		break;
	case SDLK_u:
		vkey = 'U';
		break;
	case SDLK_i:
		vkey = 'I';
		break;
	case SDLK_o:
		vkey = 'O';
		break;
	case SDLK_p:
		vkey = 'P';
		break;
	case SDLK_a:
		vkey = 'A';
		break;
	case SDLK_s:
		vkey = 'S';
		break;
	case SDLK_d:
		vkey = 'D';
		break;
	case SDLK_f:
		vkey = 'F';
		break;
	case SDLK_g:
		vkey = 'G';
		break;
	case SDLK_h:
		vkey = 'H';
		break;
	case SDLK_j:
		vkey = 'J';
		break;
	case SDLK_k:
		vkey = 'K';
		break;
	case SDLK_l:
		vkey = 'L';
		break;
	case SDLK_z:
		vkey = 'Z';
		break;
	case SDLK_x:
		vkey = 'X';
		break;
	case SDLK_c:
		vkey = 'C';
		break;
	case SDLK_v:
		vkey = 'V';
		break;
	case SDLK_b:
		vkey = 'B';
		break;
	case SDLK_n:
		vkey = 'N';
		break;
	case SDLK_m:
		vkey = 'M';
		break;
	case SDLK_MINUS:
		vkey = VK_SUBTRACT;
		break;
	case SDLK_PLUS:
		vkey = VK_ADD;
		break;
	case SDLK_LEFTBRACKET:
		vkey = VK_OEM_4; //  '[{' for US
		break;
	case SDLK_RIGHTBRACKET:
		vkey = 0xDD; //  ']}' for US
		break;
	case SDLK_BACKSLASH:
		vkey = 0xDC; //  '\|' for US
		break;
	case SDLK_COLON:
		vkey = VK_OEM_1; // ';:' for US
		break;
	case SDLK_QUOTE:
		vkey = VK_OEM_7; //  ''"' for US
		break;
	case SDLK_COMMA:
		vkey = VK_OEM_COMMA; // ',' any country
		break;
	case SDLK_PERIOD:
		vkey = VK_OEM_PERIOD; // '.' any country
		break;
	case SDLK_SLASH:
		vkey = VK_OEM_2; // '/?' for US
		break;
	case SDLK_INSERT:
		vkey = VK_INSERT;
		break;
	case SDLK_HOME:
		vkey = VK_HOME;
		break;
	case SDLK_PAGEUP:
		vkey = VK_PRIOR;
		break;
	case SDLK_DELETE:
		vkey = VK_DELETE;
		break;
	case SDLK_END:
		vkey = VK_END;
		break;
	case SDLK_PAGEDOWN:
		vkey = VK_NEXT;
		break;
	case SDLK_KP_7:
		vkey = VK_NUMPAD7;
		break;
	case SDLK_KP_8:
		vkey = VK_NUMPAD8;
		break;
	case SDLK_KP_9:
		vkey = VK_NUMPAD9;
		break;
	case SDLK_KP_4:
		vkey = VK_NUMPAD4;
		break;
	case SDLK_KP_5:
		vkey = VK_NUMPAD5;
		break;
	case SDLK_KP_6:
		vkey = VK_NUMPAD6;
		break;
	case SDLK_KP_1:
		vkey = VK_NUMPAD1;
		break;
	case SDLK_KP_2:
		vkey = VK_NUMPAD2;
		break;
	case SDLK_KP_3:
		vkey = VK_NUMPAD3;
		break;
	case SDLK_KP_DIVIDE:
		vkey = VK_DIVIDE;
		break;
	case SDLK_KP_MULTIPLY:
		vkey = VK_MULTIPLY;
		break;
	case SDLK_KP_MINUS:
		vkey = VK_SUBTRACT;
		break;
	case SDLK_KP_PLUS:
		vkey = VK_ADD;
		break;
	case SDLK_KP_DECIMAL:
		vkey = VK_DECIMAL;
		break;
	case SDLK_UP:
		vkey = VK_UP;
		break;
	case SDLK_LEFT:
		vkey = VK_LEFT;
		break;
	case SDLK_DOWN:
		vkey = VK_DOWN;
		break;
	case SDLK_RIGHT:
		vkey = VK_RIGHT;
		break;
	default: vkey = SDLK;
		break;
	}
	return vkey;
}

void DrawGUI() {
	// ---------- Main window ----------
	ImGui::Begin("Acoustic Soundboard", NULL, GUIWindowFlags);
	// ---------- Main table ----------
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
			StopAllChannels();
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
			StopAllChannels();
		}
		ImGui::PopID();
	}
	ImGui::EndTable();
	// ---------- END Main table ----------

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
	}
	// ---------- END Page display ----------

	if (ImGui::Button("Playback Devices") == true) {
		ShowPlaybackDevices = true;
	}
	ImGui::Text("%s", SelectedPlaybackDevice);
	if (ImGui::Button("Stop All Sounds") == true) {
		StopAllChannels();
	}
	ImGui::Text("Pause | Break");
	if (ImGui::Button("View Channels") == true) {
		ViewChannels = true;
		ImGui::SetNextWindowSize(ImVec2(250, 250));
	}
	if (ImGui::Checkbox("Autosave", &Autosave) == true) {
	}
	ImGui::End();
	// ---------- END Main window ----------

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
				if (UserPressedReturn) UserPressedReturn = false;
				Hotkeys[CapturedKeyIndex].key = NULL;
				Hotkeys[CapturedKeyIndex].keyText[0] = '\0';
				Hotkeys[CapturedKeyIndex].mod = NULL;
				Hotkeys[CapturedKeyIndex].modText[0] = '\0';
				Hotkeys[CapturedKeyIndex].sampleIndex = NULL;
				CaptureKeys = false;
				ShowKeyCaptureWindow = false;
			}

			if (CapturedKeyText != NULL) {
				for (int i = 0; i < NUM_SOUNDS; i++) {
					// Compare the captured key with all the stored hotkeys
					if (strcmp(Hotkeys[i].keyText, CapturedKeyText) == 0) {
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
						Hotkeys[CapturedKeyIndex].key = CapturedKeyCode;
						Hotkeys[CapturedKeyIndex].sampleIndex = CapturedKeyIndex;
						strcpy(Hotkeys[CapturedKeyIndex].keyText, SDL_GetKeyName(CapturedKeyCode));
						Hotkeys[CapturedKeyIndex].win32Key = ConvertSDLKeyToWin32Key(CapturedKeyCode);
						if (CapturedKeyMod != NULL) {
							Hotkeys[CapturedKeyIndex].mod = CapturedKeyMod;
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
			Hotkeys[CapturedKeyIndex].key = NULL;
			Hotkeys[CapturedKeyIndex].keyText[0] = '\0';
			Hotkeys[CapturedKeyIndex].win32Key = NULL;
			Hotkeys[CapturedKeyIndex].win32KeyMod = NULL;
			Hotkeys[CapturedKeyIndex].mod = NULL;
			Hotkeys[CapturedKeyIndex].modText[0] = '\0';
			Hotkeys[CapturedKeyIndex].sampleIndex = NULL;
			CapturedKeyCode = NULL;
			CapturedKeyText = NULL;
			CapturedKeyMod = NULL;
			CapturedKeyModText[0] = '\0';
		}
		ImGui::End();
		if (UserPressedReturn) UserPressedReturn = false;
	}

	else {
		CapturedKeyCode = NULL;
		CapturedKeyText = NULL;
		CapturedKeyMod = NULL;
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
		CapturedKeyText = NULL;
		ImGui::End();
	} // ----------END Key In Use Window ----------

	// ---------- Channels Window ----------
	if (ViewChannels == true) {
		if (UserPressedEscape == true) {
			ViewChannels = false;
		}
		ImGui::Begin("Channels Playing", &ViewChannels);
		ImGui::BeginTable("Channels", 2, GUITableFlags);
		ImGui::TableSetupColumn("Channel", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("File");

		ImGui::TableHeadersRow();
		for (size_t i = 0; i < NUM_CHANNELS; i++) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("%d", i);
			ImGui::TableNextColumn();
			if (ChannelStates[i].isAvailable == false) {
				ImGui::Text("Playing %s", ChannelStates[i].playingFileName);
			}
		}
		ImGui::EndTable();
		ImGui::End();

	} // ----------END Channels Window ----------

	// ---------- Playback Devices Window ----------
	if (ShowPlaybackDevices == true) {
		if (UserPressedEscape == true) {
			ShowPlaybackDevices = false;
		}
		ImGui::Begin("Playback Devices", &ShowPlaybackDevices);
		for (int i = 0; i < NumPlaybackDevices; i++) {
			ImGui::PushID(i);
			if (ImGui::Button(PlaybackDevices[i]) == true || UserPressedReturn) {
				SelectedPlaybackDevice = PlaybackDevices[i];
				Mix_OpenAudioDevice(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, NUM_CHANNELS, 22050,
					PlaybackDevices[i], SDL_AUDIO_ALLOW_ANY_CHANGE);
				ShowPlaybackDevices = false;
			}
			ImGui::PopID();
		}
		ImGui::End();
	} // ----------END Playback Devices Window ----------
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

LRESULT CALLBACK KeyboardHookCallback(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	if (CaptureKeys == false) {
		if (nCode >= 0) {
			bool altDown = (GetAsyncKeyState(VK_MENU) < 0);
			bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) < 0);
			bool shiftDown = (GetAsyncKeyState(VK_SHIFT) < 0);

			PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;

			switch (wParam) {
			case WM_SYSKEYUP:
			case WM_KEYUP:
			{
				if (p->vkCode == VK_PAUSE) {
					StopAllChannels();
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
							PlayAudio(Hotkeys[i].sampleIndex);
							// Break out of the for loop
							break;
						}
					}
				}
			}
			break;
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

void PlayAudio(size_t index) {
	bool sampleFound = true;
	if (Samples[index] == NULL) {
		Samples[index] = Mix_LoadWAV(Hotkeys[index].filePath);
		if (!Samples[index]) {
			sampleFound = false;
			OutputDebugStringA("-- DEBUG --\n");
			OutputDebugStringA(Mix_GetError());
			OutputDebugStringA("\n-- END DEBUG --\n\n");
			Hotkeys[index].filePath[0] = '\0';
			strcpy(Hotkeys[index].fileName, "File not found!");
		}
	}

	if (sampleFound == true) {
		ChannelLastUsed = Mix_PlayChannel(-1, Samples[index], 0);
		if (ChannelLastUsed > NUM_CHANNELS || ChannelLastUsed < 0) {
			OutputDebugStringA(Mix_GetError());
		}
		else {
			if (ChannelLastUsed >= 0 && ChannelLastUsed < NUM_CHANNELS) {
				strcpy(ChannelStates[ChannelLastUsed].playingFileName, Hotkeys[index].fileName);
				ChannelStates[ChannelLastUsed].isAvailable = false;
			}
		}
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

void ProcessUserInput() {
	// If we are in key capture mode, let SDL handle the inputs
	if (CaptureKeys) {
		if (Event.key.keysym.sym != SDLK_LCTRL &&
			Event.key.keysym.sym != SDLK_RCTRL &&
			Event.key.keysym.sym != SDLK_LSHIFT &&
			Event.key.keysym.sym != SDLK_RSHIFT &&
			Event.key.keysym.sym != SDLK_LALT &&
			Event.key.keysym.sym != SDLK_RALT &&
			Event.key.keysym.sym != SDLK_RETURN &&
			Event.key.keysym.sym != SDLK_ESCAPE &&
			Event.key.keysym.sym != SDLK_BACKSPACE &&
			Event.key.keysym.sym != SDLK_UP &&
			Event.key.keysym.sym != SDLK_RIGHT &&
			Event.key.keysym.sym != SDLK_DOWN &&
			Event.key.keysym.sym != SDLK_LEFT) {
			CapturedKeyCode = Event.key.keysym.sym;
			CapturedKeyText = SDL_GetKeyName(Event.key.keysym.sym);
		}

		if (Event.key.keysym.mod != NULL) {
			if (Event.key.keysym.mod & KMOD_CTRL) {
				CapturedKeyMod = KMOD_CTRL;
				strcpy(CapturedKeyModText, "CTRL + ");
				Win32CapturedKeyMod = VK_CONTROL;
			}
			else if (Event.key.keysym.mod & KMOD_SHIFT) {
				CapturedKeyMod = KMOD_SHIFT;
				strcpy(CapturedKeyModText, "SHIFT + ");
				Win32CapturedKeyMod = VK_SHIFT;
			}
			else if (Event.key.keysym.mod & KMOD_ALT) {
				CapturedKeyMod = KMOD_ALT;
				strcpy(CapturedKeyModText, "ALT + ");
				Win32CapturedKeyMod = VK_MENU;
			}
		}
	}

	switch (Event.key.keysym.sym) {
	case SDLK_RETURN:
		UserPressedReturn = true;
		break;
	case SDLK_ESCAPE:
		UserPressedEscape = true;
		break;
	case SDLK_BACKSPACE:
		UserPressedBackspace = true;
	default:
		break;
	}
}

void ResetChannel(int channel) {
	ChannelStates[channel].isAvailable = true;
	ChannelStates[channel].playingFileName[0] = '\0';
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

void StopAllChannels() {
	for (int i = 0; i < NUM_CHANNELS; i++) {
		Mix_HaltChannel(i);
		ChannelStates[i].isAvailable = true;
		ChannelStates[i].playingFileName[0] = '\0';
	}
	for (int i = 0; i < NUM_SOUNDS; i++) {
		if (Samples[i] != NULL) {
			Mix_FreeChunk(Samples[i]);
			Samples[i] = NULL;
		}
	}
}

void Update() {
	while (SDL_PollEvent(&Event)) {
		ImGui_ImplSDL2_ProcessEvent(&Event);
		switch (Event.type) {
		case SDL_QUIT: WindowShouldClose = true;
			break;
		case SDL_KEYUP: ProcessUserInput();
			break;
		default:
			break;
		}
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	GUIviewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(GUIviewport->WorkPos);
	ImGui::SetNextWindowSize(GUIviewport->WorkSize);
	DrawGUI();
	ResetNavKeys();
	ImGui::Render();
	glViewport(0, 0, GUIviewport->Size.x, GUIviewport->Size.y);
	glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(Window);
}

void Close() {
	if (Autosave == true) {
		SaveHotkeysToDatabase();
	}
	Mix_CloseAudio();
	Mix_Quit();
	free(PlaybackDevices);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	SDL_GL_DeleteContext(GLContext);
	SDL_DestroyWindow(Window);
	SDL_Quit();
}