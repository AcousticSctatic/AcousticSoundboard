/* AcousticSoundboard strives to be a simple to use soundboard for Windows. It makes use of the following software:
* 
* Dear ImGui - Graphical user interface
* SQLite3 - A databse engine
* tinyfiledialogs - A tiny, useful library for file dialogs (file explorer on Windows)
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

//int sampleIndex;
typedef struct Hotkey {
	//int index;
	int keyMod;
	int keyCode;
	char modText[MAX_PATH];
	char keyText[MAX_PATH];
	char filePath[MAX_PATH];
	char fileName[MAX_PATH];
}Hotkey;

typedef struct playback_engine {
	bool active;
	ma_engine engine;
	ma_device device;
	ma_sound sounds[MAX_SOUNDS];
	char deviceName[256];
} Playback_Engine;

typedef struct capture_engine {
	bool active;
	ma_engine engine;
	ma_device device;
	char captureDeviceName[256];
	char duplexDeviceName[256];
} Capture_Engine;

// GUI Globals
#define SOUNDS_PER_PAGE 10
#define NUM_PAGES 5
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
ImFont* MainFont;
const ImGuiViewport* GUIviewport;
ImGuiWindowFlags GUIWindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus
	| ImGuiWindowFlags_NoResize;
ImGuiTableFlags GUITableFlags = ImGuiTableFlags_Borders;
size_t CurrentPage = 1;

// Audio Globals
#define NUM_SOUNDS (SOUNDS_PER_PAGE * NUM_PAGES)
ma_context Context;
bool SoundLoaded[MAX_PLAYBACK_DEVICES][NUM_SOUNDS];
Playback_Engine PlaybackEngines[MAX_PLAYBACK_DEVICES];
Capture_Engine CaptureEngine;
ma_device_info* pPlaybackDeviceInfos;
ma_uint32 PlaybackDeviceCount;
bool* PlaybackDeviceSelected;
int NumActivePlaybackDevices = 0;
ma_device_info* pCaptureDeviceInfos;
ma_uint32 CaptureDeviceCount;
bool* CaptureDeviceSelected;
int CaptureDeviceIndex;
int NumActiveCaptureDevices = 0;
bool ShowDuplexDevices;
int DuplexDeviceIndex = 0;
ma_resource_manager ResourceManager;

// Logical Toggle Globals
bool CaptureKeys = false;
bool CapturedKeyInUse = false;
bool ShowPlaybackDeviceList = false;
bool ShowCaptureDeviceList = false;
bool ShowKeyCaptureWindow = false;
bool ShowPlaybackDevices = false;
bool UserPressedReturn = false;
bool UserPressedEscape = false;
bool UserPressedBackspace = false;
bool WindowShouldClose = false;

// Hotkey globals
DWORD CapturedKeyCode;
int CapturedKeyIndex = 0;
char CapturedKeyText[MAX_PATH];
int CapturedKeyMod;
char CapturedKeyModText[MAX_PATH];
Hotkey Hotkeys[NUM_SOUNDS];

char FilePath[MAX_PATH];
sqlite3* db;
HHOOK KeyboardHook;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_  LPSTR lpCmdLine, _In_  int nCmdShow);
LRESULT WINAPI Win32Callback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void AudioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
void ClearCapturedKey();
void CloseAudioSystem();
void CloseCaptureDevice();
void ClosePlaybackDevices();
bool CreateDeviceD3D(HWND hWnd);
void DrawGUI();
void DuplexDeviceCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
char* GetFileNameFromPath(char* const aoDestination, char const* const aSource);
int InitAudioSystem();
void InitCaptureDevice(ma_device_id* captureId, ma_device* duplexDevice);
void InitPlaybackDevice(ma_device_id* deviceId);
void InitSQLite();
LRESULT CALLBACK KeyboardHookCallback(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam);
void LoadDevicesFromDatabase();
void LoadHotkeysFromDatabase();
void PlayAudio(int iEngine, int iSound, const char* filePath);
void PrintToLog(const char* fileName, const char* logText);
void ResetDeviceD3D();
void ResetNavKeys();
void SaveDevicesToDatabase();
void SaveHotkeysToDatabase();
void SelectCaptureDevice();
void SelectPlaybackDevice();
void UnloadSound(int iSound);
void Update();