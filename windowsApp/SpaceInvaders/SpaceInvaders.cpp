/*
CS467 - Build an emulator and run space invaders rom
Jon Frosch & Phil Sheets

This is the platform-specific (Windows application) code for the
program. It is used to display the screen, gather input, play sounds,
and advance the machine emulator every frame.
*/
#include "framework.h"
#include <mmsystem.h> //For PlaySounds
#include "SpaceInvaders.h"
#include "platformAdapter.hpp"
#include "machine.hpp"
#include "memory.hpp"
#include "emulator.hpp"
#include <vector>
#include <fstream>
#include <memory>
#include "soundDevice.h"
#include "snapshot.h"
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

//SpaceInvaders variables and forward declares
void DrawScreen(HWND hWnd, HDC hdc);
void LoadROMIntoMemory();

void RefreshScreen();
void TakeSnapshot();
void RewindSnapshot();

Adapter platformAdapter;
Machine machine;
std::unique_ptr<SpaceInvaderMemory> memory = nullptr;
Emulator8080 emulator;
uint8_t* g_videoBuffer;
bool g_gameRunning;
bool g_screenNeedsRefresh;

HWND g_hWndGameWindow;

const int NUMBER_OF_COLORS = 4;
static uint8_t COLOR_BLACK = 0x00;
static uint8_t COLOR_WHITE = 0x01;
static uint8_t COLOR_MAGENTA = 0x02;
static uint8_t COLOR_GREEN = 0x03;
RGBQUAD COLOR_TABLE[NUMBER_OF_COLORS];
const int NATIVE_HEIGHT_PIXELS = 256;
const int NATIVE_WIDTH_PIXELS = 224;
const int BMP_BITS_PER_PIXEL = 8;
const int DEFAULT_SCALE_FACTOR = 2;
BITMAPINFO* bi;

std::unique_ptr<InvaderSoundDevice> soundPlayer;

bool g_paused;
std::vector<SpaceInvaderMemory> rewindMemory;
std::vector<std::shared_ptr<Snapshot>> snapshots;
int g_snapshotIndex;

//end declares

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    //** Configure machine and emulator **

	//Create gui app video buffer
	int screenPixelBufferSize = NATIVE_HEIGHT_PIXELS * NATIVE_WIDTH_PIXELS; //57344 total pixels
	g_videoBuffer = reinterpret_cast<uint8_t*>(std::malloc(screenPixelBufferSize * sizeof(uint8_t)));

	//Create the bitmap color palette to be used for drawing the screen
	COLOR_TABLE[COLOR_BLACK] = RGBQUAD{ 0x00,0x00,0x00,0 }; // color 0, black for background
	COLOR_TABLE[COLOR_WHITE] = RGBQUAD{ 0xff,0xff,0xff,0 }; // color 1, white for foreground
	COLOR_TABLE[COLOR_MAGENTA] = RGBQUAD{ 0x44,0x11,0xff,0 }; // color 2, magenta for upper foreground band
	COLOR_TABLE[COLOR_GREEN] = RGBQUAD{ 0x08,0x9d,0x13,0 }; // color 3, green for upper foreground band

	bi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + NUMBER_OF_COLORS * sizeof(RGBQUAD));
	if (!bi) exit(EXIT_FAILURE);
	memset(bi, 0, sizeof(BITMAPINFOHEADER) + NUMBER_OF_COLORS * sizeof(RGBQUAD));
	bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi->bmiHeader.biWidth = NATIVE_WIDTH_PIXELS;
	bi->bmiHeader.biHeight = -NATIVE_HEIGHT_PIXELS;
	bi->bmiHeader.biPlanes = 1;
	bi->bmiHeader.biBitCount = BMP_BITS_PER_PIXEL;
	bi->bmiHeader.biCompression = BI_RGB;
	bi->bmiHeader.biClrUsed = NUMBER_OF_COLORS;

	for (int i = 0; i < NUMBER_OF_COLORS; ++i)
	{
		bi->bmiColors[i] = COLOR_TABLE[i];
	}

	//Connect machine and platform sound output
	soundPlayer = std::make_unique<InvaderSoundDevice>("..\\..\\sounds\\");
	platformAdapter.setShootFunction([]() {soundPlayer->playSound(InvaderSoundDevice::sfx::SHOT); });
	platformAdapter.setPlayerDieSoundFunction([]() {soundPlayer->playSound(InvaderSoundDevice::sfx::PLAYER_DEATH); });
	platformAdapter.setInvaderDieFunction([]() {soundPlayer->playSound(InvaderSoundDevice::sfx::INVADER_DEATH); });
	platformAdapter.setStartUFOFunction([]() {soundPlayer->playSound(InvaderSoundDevice::sfx::UFO); });
	platformAdapter.setStopUFOFunction([]() {soundPlayer->stopSound(InvaderSoundDevice::sfx::UFO);} );
	platformAdapter.setUFOHitFunction([]() {soundPlayer->playSound(InvaderSoundDevice::sfx::UFO_HIT); });
	platformAdapter.setFleetMove1Function([]() {soundPlayer->playSound(InvaderSoundDevice::sfx::FLEET_MOVE_1); });
	platformAdapter.setFleetMove2Function([]() {soundPlayer->playSound(InvaderSoundDevice::sfx::FLEET_MOVE_2); });
	platformAdapter.setFleetMove3Function([]() {soundPlayer->playSound(InvaderSoundDevice::sfx::FLEET_MOVE_3); });
	platformAdapter.setFleetMove4Function([]() {soundPlayer->playSound(InvaderSoundDevice::sfx::FLEET_MOVE_4); });

	//Screen refresh indicator
	platformAdapter.setRefreshScreenFunction(RefreshScreen);

	machine.setPlatformAdapter(&platformAdapter);

	memory = std::make_unique<SpaceInvaderMemory>();

	LoadROMIntoMemory();

	//Create emulator with assigned memory
	emulator.connectMemory(memory.get());
	emulator.reset(0x0000);

	//Connect machine to emulator
	machine.setEmulator(&emulator);

	//** End Configure machine and emulator **

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SPACEINVADERS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SPACEINVADERS));

    MSG msg;
	 
	//Main game loop
	g_gameRunning = true;
	while (g_gameRunning)
	{
		//Prepare to gather input
		if (!g_paused)
		{
			platformAdapter.setInputChanged(false);
		}

		//Check for input or need to redraw screen
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		//Advance the machine, it will trigger a screen refresh when needed
		if (!g_paused)
		{
			machine.step();
		}
		
	}
	free(g_videoBuffer);
	free(bi);
    return (int) msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SPACEINVADERS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SPACEINVADERS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   //Space Invaders screen: 256x224 pixels
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	   CW_USEDEFAULT, 0, NATIVE_WIDTH_PIXELS * DEFAULT_SCALE_FACTOR, NATIVE_HEIGHT_PIXELS * DEFAULT_SCALE_FACTOR, nullptr, nullptr, hInstance, nullptr);
	
   if (!hWnd)
   {
      return FALSE;
   }

   g_hWndGameWindow = hWnd;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
	
			DrawScreen(hWnd, hdc);

            EndPaint(hWnd, &ps);
        }
        break;
	case WM_KEYDOWN:
		switch (wParam)
		{
			case VK_LEFT:
				platformAdapter.setP1LeftButtonDown(true);
			break;
			case VK_SPACE:
				platformAdapter.setP1ShootButtonDown(true);
				break;
			case VK_RIGHT:
				platformAdapter.setP1RightButtonDown(true);
				break;
			case 0x58:
				//x key
				platformAdapter.setP2LeftButtonDown(true);
				break;
			case 0x43:
				//c key
				platformAdapter.setP2RightButtonDown(true);
				break;
			case 0x5A:
				//z key
				platformAdapter.setP2ShootButtonDown(true);
				break;
			case 0x31:
				// 1 key
				platformAdapter.setP1StartButtonDown(true);
				break;
			case 0x32:
				//2 key
				platformAdapter.setP2StartButtonDown(true);
				break;
			case 0x33:
				//3 key
				platformAdapter.setCoin(true);
				break;
			case 0x34:
				//4 key, sound test UFO
				machine.writePortValue(3, 0x01);
				break;
			case 0x35:
				//5 key, sound test shoot
				machine.writePortValue(3, 0x02);
				break;
			case 0x36:
				//6 key, sound test player die
				machine.writePortValue(3, 0x04);
				break;
			case 0x37:
				//7 key, sound test invader die
				machine.writePortValue(3, 0x08);
				break;
			case 0x38:
				//8 key, sound test fleet move 1
				machine.writePortValue(5, 0x01);
				break;
			case 0x39:
				//9 key, sound test fleet move 2
				machine.writePortValue(5, 0x02);
				break;
			case 0x30:
				//0 key, sound test fleet move 3
				machine.writePortValue(5, 0x04);
				break;
			case VK_OEM_MINUS:
				// - key, sound test fleet move 4
				machine.writePortValue(5, 0x08);
				break;
			case VK_OEM_PLUS:
				// + key, sound test UFO hit
				machine.writePortValue(5, 0x10);
				break;
			case 0x50:
				//P key
				g_paused = !g_paused;
				break;
			case 0x49:
				//I key
				//TakeSnapshot
				TakeSnapshot();
				break;
			case 0x4F:
				//O key
				//Attempt to go back in time
				g_paused = true;
				RewindSnapshot();
				RefreshScreen();
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_LEFT:
			platformAdapter.setP1LeftButtonDown(false);
			break;
		case VK_SPACE:
			platformAdapter.setP1ShootButtonDown(false);
			break;
		case VK_RIGHT:
			platformAdapter.setP1RightButtonDown(false);
			break;
		case 0x58:
			//x key
			platformAdapter.setP2LeftButtonDown(false);
			break;
		case 0x43:
			//c key
			platformAdapter.setP2RightButtonDown(false);
			break;
		case 0x5A:
			//z key
			platformAdapter.setP2ShootButtonDown(false);
			break;
		case 0x31:
			// 1 key
			platformAdapter.setP1StartButtonDown(false);
			break;
		case 0x32:
			//2 key
			platformAdapter.setP2StartButtonDown(false);
			break;
		case 0x33:
			//3 key
			platformAdapter.setCoin(false);
			break;
		case 0x34:
			//4 key
			// clear sound bits
			machine.writePortValue(3, 0x00);
			break;
		case 0x35:
			//5 key
			// clear sound bits
			machine.writePortValue(3, 0x00);
			break;
		case 0x36:
			// 6 key
			// clear sound bits
			machine.writePortValue(3, 0x00);
			break;
		case 0x37:
			// 7 key
			// clear sound bits
			machine.writePortValue(3, 0x00);
			break;
		case 0x38:
			// 8 key
			// clear sound bits
			machine.writePortValue(5, 0x00);
			break;
		case 0x39:
			// 9 key
			// clear sound bits
			machine.writePortValue(5, 0x00);
			break;
		case 0x30:
			// 0 key
			// clear sound bits
			machine.writePortValue(5, 0x00);
			break;
		case VK_OEM_MINUS:
			// clear sound bits
			machine.writePortValue(5, 0x00);
			break;
		case VK_OEM_PLUS:
			// clear sound bits
			machine.writePortValue(5, 0x00);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
    case WM_DESTROY:
		g_gameRunning = false;
        PostQuitMessage(0);
        break;
	case WM_QUIT:
		g_gameRunning = false;
		PostQuitMessage(0);
		break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


void DrawScreen(HWND hWnd, HDC hdc)
{
	//Space Invaders screen: 256x224 pixels
	//Each byte in the g_videoBuffer is a pixel to make it easy to rotate
	int height = NATIVE_HEIGHT_PIXELS;
	int width = NATIVE_WIDTH_PIXELS;
	//int byteSize = 1; //Each pixel is 8 bits
	//int bitCount = byteSize * 8; // could be 8, 16, 24, 32, 64 bits per color, only 0/1 from game
	//int totalSize = height * width * byteSize;  // 57,344 total pixels

	//Iterate over each byte of of memory and put bits into video buffer
	//int bufferIndex = 0; //Used for testing drawing raw data (rotated clockwise)
	
	int rowWidth = NATIVE_WIDTH_PIXELS;
	int rowIndex = NATIVE_HEIGHT_PIXELS - 1; //reduce by 1 for 0-indexing
	int columnIndex = 0;

	static int VIDEO_BUFFER_BEGIN = 0x2400;
	static int VIDEO_BUFFER_END = 0x4000;

	for (int i = VIDEO_BUFFER_BEGIN; i < VIDEO_BUFFER_END; ++i)
	{
		uint8_t bitBlock = memory->read(i);

		//Rotate pixels counter-clockwise, so draw every column bottom-up
		auto foreground = [](int row, int column)
		{
			if ((row <= 32) || ((row > 64) && (row <= 184)) || ((row > 240) && ((column <= 15) || (column > 134))))
			{
				return COLOR_WHITE;
			} 
			else if (row > 32 && row <= 64)
			{
				return COLOR_MAGENTA;
			}
			else
			{
				return COLOR_GREEN;
			}
		};
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 0) & 0x1 ? foreground(rowIndex + 1, columnIndex) : COLOR_BLACK;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 1) & 0x1 ? foreground(rowIndex + 1, columnIndex) : COLOR_BLACK;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 2) & 0x1 ? foreground(rowIndex + 1, columnIndex) : COLOR_BLACK;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 3) & 0x1 ? foreground(rowIndex + 1, columnIndex) : COLOR_BLACK;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 4) & 0x1 ? foreground(rowIndex + 1, columnIndex) : COLOR_BLACK;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 5) & 0x1 ? foreground(rowIndex + 1, columnIndex) : COLOR_BLACK;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 6) & 0x1 ? foreground(rowIndex + 1, columnIndex) : COLOR_BLACK;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 7) & 0x1 ? foreground(rowIndex + 1, columnIndex) : COLOR_BLACK;

		if (rowIndex < 0)
		{
			rowIndex = NATIVE_HEIGHT_PIXELS - 1; //reduce by 1 for 0-indexing
			columnIndex += 1;
		}
		
	}

	//Enforce correct aspect ratio for displayed screen
	static float aspectRatio = 256/224;
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	int targetWidth = clientRect.right - clientRect.left;
	int targetHeight = clientRect.bottom - clientRect.top;
	if (targetHeight >= targetWidth * aspectRatio)
	{
		targetHeight = targetWidth * aspectRatio;
		//targetWidth = targetHeight / aspectRatio;
	}
	else
	{
		//targetWidth = targetHeight / aspectRatio;
		//targetHeight = targetWidth * aspectRatio;
	}

	int x = StretchDIBits(hdc, 0, 0, targetWidth, targetHeight, 0, 0, NATIVE_WIDTH_PIXELS, NATIVE_HEIGHT_PIXELS, g_videoBuffer, bi, DIB_RGB_COLORS, SRCCOPY);
}

//Load the bundled ROM file of the application into the memory object
//used by the emulator.
//https://stackoverflow.com/questions/9240188/how-to-load-a-custom-binary-resource-in-a-vc-static-library-as-part-of-a-dll
void LoadROMIntoMemory()
{
	HRSRC hRomResource = FindResource(NULL, MAKEINTRESOURCE(IDR_ROM_FILE), RT_RCDATA);
	if (!hRomResource)
		return;

	unsigned int romSize = SizeofResource(NULL, hRomResource);

	HGLOBAL hRomData = LoadResource(NULL, hRomResource);
	if (!hRomData)
		return;

	void* pRomBinaryData = LockResource(hRomData);
	if (!pRomBinaryData)
		return;

	uint8_t* romArray = reinterpret_cast<uint8_t*>(pRomBinaryData);

	// copy the ROM resource into the memory object for the processor
	memory->flashROM(romArray, romSize);

	UnlockResource(hRomData);
	FreeResource(hRomData);
}

void RefreshScreen()
{
	InvalidateRect(g_hWndGameWindow, 0, 0);
}

void TakeSnapshot()
{
	std::shared_ptr<Snapshot> snapshot = std::make_shared<Snapshot>();
	std::unique_ptr<State8080> state = emulator.getState();
	snapshot->state.a = state->a;
	snapshot->state.b = state->b;
	snapshot->state.c = state->c;
	snapshot->state.d = state->d;
	snapshot->state.e = state->e;
	snapshot->state.h = state->h;
	snapshot->state.l = state->l;
	snapshot->state.sp = state->sp;
	snapshot->state.pc = state->pc;
	snapshot->state.loadFlags(state->getFlags());

	snapshot->copyMemory(emulator.memory);
	if (g_snapshotIndex == snapshots.size())
	{
		snapshots.push_back(snapshot);
	}
	else
	{
		//We've rewound, so start overwriting
		snapshots[g_snapshotIndex] = snapshot;		
	}
	g_snapshotIndex++;
}

void RewindSnapshot()
{
	if (g_snapshotIndex == 0)
	{
		return;
	}

	g_snapshotIndex--;
	State8080 state = snapshots[g_snapshotIndex]->state;
	emulator.state.a = state.a;
	emulator.state.b = state.b;
	emulator.state.c = state.c;
	emulator.state.d = state.d;
	emulator.state.e = state.e;
	emulator.state.h = state.h;
	emulator.state.l = state.l;
	emulator.state.sp = state.sp;
	emulator.state.pc = state.pc;
	emulator.state.loadFlags(state.getFlags());
	uint8_t* dataPtr = snapshots[g_snapshotIndex]->romData.data();
	//memory->flashROM(snapshots[g_snapshotIndex]->romData.data(), snapshots[g_snapshotIndex]->romData.size(), 0);
	memory->flashROM(dataPtr, snapshots[g_snapshotIndex]->romData.size(), 0);
}