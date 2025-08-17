/* AcousticSoundboard strives to be a simple to use soundboard for Windows. It makes use of the following software:
* 
* Dear ImGui - Graphical user interface
* SQLite3 - A databse engine
* 
*/
#pragma once
#include <windows.h>
#include <d3d11.h>
#include <sqlite3.h>
#include <imgui.h>
#include <miniaudio.h>
#include <dxgi.h>
#include <windef.h>
#include <cstdint>
#include <sal.h>
#pragma comment(lib,"Shlwapi.lib")
#pragma comment(lib,"Pathcch.lib")
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"sqlite3.lib")
#define MAX_PLAYBACK_DEVICES 2
#define NUM_REPEAT_SOUNDS 10
#define MAX_SOUNDS 50
#define APP_MAX_PATH 2048

typedef struct StringUTF8 {
	int numChars = APP_MAX_PATH;
	char string[APP_MAX_PATH] = { 0 };
} StringUTF8;

typedef struct StringUTF16 {
	int numWideChars = APP_MAX_PATH;
	wchar_t string[APP_MAX_PATH] = { 0 };
} StringUTF16;

typedef struct Hotkey {
	int keyMod = 0;
	uint32_t keyCode = 0;
	StringUTF8 modText;
	StringUTF16 keyText;
	StringUTF8 keyTextUTF8;
	StringUTF16 filePath;
	StringUTF8 fileNameUTF8;
} Hotkey;

typedef struct RepeatSound {
	ma_sound sound;
	bool soundLoaded;
	int soundIndex;
} RepeatSound;

typedef struct Playback_Engine {
	bool active;
	ma_engine engine;
	ma_device device;
	ma_sound sounds[MAX_SOUNDS];
	RepeatSound repeatSounds[NUM_REPEAT_SOUNDS];
	char deviceName[256];
} Playback_Engine;

typedef struct Capture_Engine {
	ma_engine engine;
	ma_device device;
	char captureDeviceName[256];
	char duplexDeviceName[256];
} Capture_Engine;

// GUI Globals
#define SOUNDS_PER_PAGE 10
#define NUM_PAGES 5
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static bool g_SwapChainOccluded = false;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
const ImGuiViewport* GUIviewport;
ImGuiWindowFlags GUIWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
ImGuiTableFlags GUITableFlags = ImGuiTableFlags_Borders;
size_t CurrentPage = 1;
StringUTF8 ErrorMsgBuffer;

// Audio Globals
#define NUM_SOUNDS (SOUNDS_PER_PAGE * NUM_PAGES)
ma_context Context;
bool SoundLoaded[MAX_PLAYBACK_DEVICES][NUM_SOUNDS];
Playback_Engine PlaybackEngines[MAX_PLAYBACK_DEVICES];
StringUTF8 SavedPlaybackDeviceName1;
StringUTF8 SavedPlaybackDeviceName2;
Capture_Engine CaptureEngine;
StringUTF8 SavedCaptureDeviceName;
StringUTF8 SavedDuplexDeviceName;
ma_device_info* pPlaybackDeviceInfos;
uint32_t PlaybackDeviceCount;
bool* PlaybackDeviceSelected;
int NumActivePlaybackDevices = 0;
ma_device_info* pCaptureDeviceInfos;
uint32_t CaptureDeviceCount;
bool* CaptureDeviceSelected;
int CaptureDeviceIndex;
int NumActiveCaptureDevices = 0;
bool ShowDuplexDevices;
int DuplexDeviceIndex = 0;
ma_resource_manager ResourceManager;
int VolumeDisplay = 100;

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
bool DisplayErrorMessageWindow = false;
#ifdef _DEBUG
bool ShowDebugWindow;
#endif

// Hotkey globals
uint32_t CapturedKeyCode;
int CapturedKeyIndex = 0;
StringUTF16 CapturedKeyText;
StringUTF8 CapturedKeyTextUTF8;
int CapturedKeyMod;
StringUTF8 CapturedKeyModText;
Hotkey Hotkeys[NUM_SOUNDS];

sqlite3* db;
HHOOK KeyboardHook;


StringUTF16 StartingDirectory;
StringUTF16 PathToDatabase;
StringUTF8 PathToDatabaseUTF8;
StringUTF16 PathToErrorLog;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_  LPSTR lpCmdLine, _In_  int nCmdShow);
LRESULT WINAPI Win32Callback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void AudioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
void CenterNextWindow(float minWidth, float minHeight);
void CleanupDeviceD3D();
void CleanupRenderTarget();
void ClearCapturedKey();
void CloseAudioSystem();
void CloseCaptureDevice();
void ClosePlaybackDevices();
void CreateDeviceD3D(HWND hWnd);
void CreateRenderTarget();
void DecodeUTF8(StringUTF8* str, StringUTF16* wideStr);
void DrawGUI();
void DrawHotkeyTable();
void DrawKeyCaptureWindow();
void DuplexDeviceCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
void EncodeUTF8(StringUTF16* wideStr, StringUTF8* str);
void InitAudioSystem();
void InitCaptureDevice(ma_device_id* captureId, char* captureDeviceName, ma_device* duplexDevice, char* duplexDeviceName);
void InitPlaybackDevice(ma_device_id* deviceId, int iEngine, char* deviceName);
void InitSQLite();
void InterpretAudioErrorMessage(ma_result result, uint32_t hotKeyIndex);
LRESULT CALLBACK KeyboardHookCallback(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam);
void LoadConfigFromDatabase();
void LoadDevicesFromDatabase();
void LoadHotkeysFromDatabase();
ma_result PlayAudio(uint32_t iEngine, uint32_t iSound, const wchar_t* filePath);
void PrintToErrorLog(const wchar_t* logText);
void PrintToErrorLogAndExit(const wchar_t* logText);
void PrintToLogWin32Error();
void ResetNavKeys();
void SaveConfigToDatabase();
void SaveDevicesToDatabase();
void SaveHotkeysToDatabase();
void SelectCaptureDevice();
void SelectPlaybackDevice();
void SetVolume();
void SQLiteCopyColumnText(sqlite3_stmt* stmt, int iRow, int iColumn, const wchar_t* const columnName, StringUTF8* destination);
void SQLiteCopyColumnText16(sqlite3_stmt* stmt, int iRow, int iColumn, const wchar_t* const columnName, StringUTF16* destination);
void UnloadSound(int iSound);