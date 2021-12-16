/* AcousticSoundboard strives to be a simple to use soundboard for Windows. It makes use of the following software:
* 
* SDL2 - Simple DirectMedia Layer - Low level functions such as window, event, input handling, audio devices
* SDL Mixer - Ssound mixing library
* Dear ImGui - Graphical user interface
* SQLite3 - A databse engine
* tinyfiledialogs - A tiny, useful library for file dialogs (file explorer on Windows)
* libFLAC - Playing FLAC files
* libmpg123 - Playing mp3 files
* libogg - Playing ogg files
*/
#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengles2.h>
#include <SDL_mixer.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <tinyfiledialogs.h>
#include <winuser.h>
#include <sqlite3.h>

typedef struct Hotkey {
	SDL_Keycode key;
	Uint16 mod;
	int win32KeyMod;
	int win32Key;
	char keyText[MAX_PATH];
	char modText[MAX_PATH];
	int sampleIndex;
	char filePath[MAX_PATH];
	char fileName[MAX_PATH];
}Hotkey;

typedef struct ChannelState {
	bool isAvailable;
	char playingFileName[MAX_PATH];
}ChannelState;

#define NUM_CHANNELS 8
#define SOUNDS_PER_PAGE 10
#define NUM_PAGES 5
#define NUM_SOUNDS SOUNDS_PER_PAGE * NUM_PAGES
bool Autosave = true;
sqlite3* db;
ImFont* MainFont;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
const ImGuiViewport* GUIviewport;
ImGuiWindowFlags GUIWindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus
	| ImGuiWindowFlags_NoResize;
ImGuiTableFlags GUITableFlags = ImGuiTableFlags_Borders;
SDL_Window* Window;
SDL_GLContext GLContext;
SDL_Event Event;
HHOOK KeyboardHook;
int win32LastKey;
uint32_t Timer;
bool WindowShouldClose = false;
bool CaptureKeys = false;
size_t CurrentPage;
Mix_Chunk* Samples[NUM_SOUNDS];
void (*channelFinished)(int channel);
ChannelState ChannelStates[NUM_CHANNELS];
int ChannelLastUsed = 0;
char** PlaybackDevices;
const char* SelectedPlaybackDevice;
int NumPlaybackDevices = 0;
bool ViewChannels = false;
bool ShowKeyCaptureWindow;
bool ShowPlaybackDevices;
const char* CapturedKeyText;
Uint16 CapturedKeyMod;
int Win32CapturedKeyMod;
char CapturedKeyModText[MAX_PATH];
SDL_Keycode CapturedKeyCode;
bool CapturedKeyInUse = false;
Hotkey Hotkeys[NUM_SOUNDS];
int CapturedKeyIndex = 0;
bool UserPressedReturn = false;
bool UserPressedEscape = false;
bool UserPressedBackspace = false;

void Initialize();
void InitSQLite();
int ConvertSDLKeyToWin32Key(SDL_Keycode SDLK);
void DrawGUI();
char* GetFileNameFromPath(char* const aoDestination, char const* const aSource);
LRESULT CALLBACK KeyboardHookCallback(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam);
void LoadHotkeysFromDatabase();
void PlayAudio(size_t index);
void PrintToLog(const char* fileName, const char* logText);
void ProcessUserInput();
void ResetChannel(int channel);
void ResetNavKeys();
void SaveHotkeysToDatabase();
void StopAllChannels();
void Update();
void Close();

