/* AcousticSoundboard strives to be a simple to use soundboard for Windows. It makes use of the following software:
* 
* Dear ImGui - Graphical user interface
* SQLite3 - A databse engine
* tinyfiledialogs - A tiny, useful library for file dialogs (file explorer on Windows)
* libogg - Playing ogg files
*/
#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <windows.h>
#include <winuser.h>
#include <d3d9.h>
#include "sqlite3.h"
#include "tinyfiledialogs.h"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#define MAX_ENGINES 3
#define MAX_PLAYBACK_DEVICES 2
#define MAX_SOUNDS 50

typedef struct Hotkey {
	int win32KeyMod;
	int win32Key;
	char keyText[MAX_PATH];
	char modText[MAX_PATH];
	int sampleIndex;
	char filePath[MAX_PATH];
	char fileName[MAX_PATH];
}Hotkey;

typedef struct playback_engine {
	bool active;
	ma_engine engine;
	ma_device device;
	ma_sound sounds[MAX_SOUNDS];
} Playback_Engine;

typedef struct capture_engine {
	bool active;
	ma_engine engine;
	ma_device device;
} Capture_Engine;

static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
ImFont* MainFont;
ma_context Context;
bool SoundLoaded[MAX_PLAYBACK_DEVICES][MAX_SOUNDS];
Playback_Engine PlaybackEngines[MAX_PLAYBACK_DEVICES];
Capture_Engine CaptureEngine;
ma_device_info* pPlaybackDeviceInfos;
ma_uint32 PlaybackDeviceCount;
bool* PlaybackDeviceSelected;
int NumActivePlaybackDevices = 0;
ma_device_info* pCaptureDeviceInfos;
ma_uint32 CaptureDeviceCount;
bool* CaptureDeviceSelected;
int NumActiveCaptureDevices = 0;
bool SelectDuplexDevice;
int DuplexDeviceIndex = 0;
char FilePath[MAX_PATH];
ma_resource_manager ResourceManager;
bool WindowShouldClose = false;
bool ShowPlaybackDeviceList = false;
bool ShowCaptureDeviceList = false;

#define SOUNDS_PER_PAGE 10
#define NUM_PAGES 5
#define NUM_SOUNDS SOUNDS_PER_PAGE * NUM_PAGES
bool Autosave = true;
sqlite3* db;
const ImGuiViewport* GUIviewport;
ImGuiWindowFlags GUIWindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus
	| ImGuiWindowFlags_NoResize;
ImGuiTableFlags GUITableFlags = ImGuiTableFlags_Borders;
HHOOK KeyboardHook;
int win32LastKey;
bool CaptureKeys = false;
size_t CurrentPage;
bool ShowKeyCaptureWindow;
bool ShowPlaybackDevices;
char CapturedKeyText[MAX_PATH];
DWORD CapturedKeyCode;
int Win32CapturedKeyMod;
char CapturedKeyModText[MAX_PATH];
bool CapturedKeyInUse = false;
Hotkey Hotkeys[NUM_SOUNDS];
int CapturedKeyIndex = 0;
bool UserPressedReturn = false;
bool UserPressedEscape = false;
bool UserPressedBackspace = false;

void AudioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
void DuplexDeviceCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
int InitAudioSystem();
void InitCaptureDevice(ma_device_id* captureId, ma_device* duplexDevice);
void InitPlaybackDevice(ma_device_id* deviceId);
void SelectCaptureDevice();
void SelectPlaybackDevice();
void PlayAudio(int iEngine, int iSound, const char* filePath);
void CloseAudioSystem();
void CloseCaptureDevice();
void ClosePlaybackDevices();
bool CreateDeviceD3D(HWND hWnd);
void ResetDeviceD3D();
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_  LPSTR lpCmdLine, _In_  int nCmdShow);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI Win32Callback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void InitSQLite();
void DrawGUI();
char* GetFileNameFromPath(char* const aoDestination, char const* const aSource);
LRESULT CALLBACK KeyboardHookCallback(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam);
void LoadHotkeysFromDatabase();
void PrintToLog(const char* fileName, const char* logText);
void ResetNavKeys();
void SaveHotkeysToDatabase();
void Update();

