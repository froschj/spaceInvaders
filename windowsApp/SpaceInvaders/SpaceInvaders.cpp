// SpaceInvaders.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <mmsystem.h> //For PlaySounds
#include "SpaceInvaders.h"
#include "platformAdapter.hpp"
#include "machine.hpp"
#include "memory.hpp"
#include "emulator.hpp"
#include <vector>
#include <fstream>

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

//TODO add all input/output port handling
//1P left/right/shoot, 2P left/right/shoot, CREDIT, etc
void OnLeft();
void OnRight();
void OnFire();
void OnCoin();
void PlaySISound(); //Temp sound function
void DrawScreen(HWND hWnd, HDC hdc);


Adapter platformAdapter;
Machine machine;
Memory memory;
Emulator8080 emulator;
uint8_t* g_videoBuffer;


//end declares

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

	//Create gui app video buffer
	int screenPixelBufferSize = 256 * 224; //57344 total pixels
	g_videoBuffer = reinterpret_cast<uint8_t*>(std::malloc(screenPixelBufferSize * sizeof(uint8_t)));

	//Connecting machine and platform adapters
	platformAdapter.setInvoke(&PlaySISound);
	machine.setPlatformAdapter(&platformAdapter);

	//Create memory block and assign to memory
	std::unique_ptr<std::vector<uint8_t>> memoryBlock =
		std::make_unique<std::vector<uint8_t>>(0x4000);

	//Read ROM into the memory
	std::ifstream romFile;
	romFile.open("..\\..\\roms\\invaders\\invaders", std::ios::binary);
	if (romFile.fail()) {
		std::cerr << "Could not open file: " << "invaders" << std::endl;
		return 1;
	}

	// file opened successfully, read it into memory
	// get the length
	romFile.seekg(0, romFile.end);
	int romLength = romFile.tellg();
	romFile.seekg(0, romFile.beg);

	// create a buffer and read into it
	romFile.read(reinterpret_cast<char*>(memoryBlock->data()), romLength);
	if (romFile.fail()) {
		std::cerr << "Error reading file." << std::endl;
		return 1;
	}
	romFile.close();

	//Assign memory
	memory.setMemoryBlock(std::move(memoryBlock));

	//Create emulator with assigned memory
	emulator.setMemory(&memory);
	emulator.reset(0x0000);

	//Let's run the emulator some?
	for (int i = 0; i < 30000; ++i)
	{
		int cycles = emulator.step();
	}

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

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

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
	   CW_USEDEFAULT, 0, 224 * 4, 256 * 4, nullptr, nullptr, hInstance, nullptr);


   if (!hWnd)
   {
      return FALSE;
   }

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
            // TODO: Add any drawing code that uses hdc here...

			DrawScreen(hWnd, hdc);

            EndPaint(hWnd, &ps);
        }
        break;
	case WM_KEYDOWN:
		switch (wParam)
		{
			case VK_LEFT:
				OnLeft();
			break;
			case VK_SPACE:
				OnFire();
				InvalidateRect(hWnd, 0, 0);
				break;
			case VK_RIGHT:
				OnRight();
				break;

			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
    case WM_DESTROY:
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

//TODO replace with final code
//Temp sound trigger for playing a sound resource, from MSDN documentation
void PlaySISound()
{

	HRSRC hResInfo;
	HANDLE hRes;
	LPCWSTR lpWavInMemory;

	hResInfo = FindResource(NULL, MAKEINTRESOURCE(IDR_SHOOT), L"WAVE");

	if (hResInfo == NULL)
		return;

	hRes = LoadResource(NULL, hResInfo);

	if (hRes == NULL)
		return;

	lpWavInMemory = (LPCWSTR) LockResource(hRes);

	sndPlaySound(lpWavInMemory, SND_MEMORY | SND_SYNC |
		SND_NODEFAULT);

	UnlockResource(hRes);
	FreeResource(hRes);

}

void OnFire()
{
	OutputDebugString(_T("On Fire Down!\n"));

	machine.playSound();
}

void OnCoin()
{
	OutputDebugString(_T("On Coin Down!\n"));
}

void OnRight()
{
	OutputDebugString(_T("On Right Down!\n"));
}

void OnLeft()
{
	OutputDebugString(_T("On Left Down!\n"));

}


void DrawScreen(HWND hWnd, HDC hdc)
{
	//Space Invaders screen: 256x224 pixels
	int height = 256;
	int width = 224;
	int byteSize = 1; //Each pixel is 8 bits
	int bitCount = byteSize * 8; // could be 8, 16, 24, 32, 64 bits per color, only 0/1 from game
	int totalSize = height * width * byteSize;  // 57,344 total pixels

	//TODO need to rotate screen, just trying to draw it currently
	//Iterate over each byte of of memory and put bits into video buffer
	int bufferIndex = 0;
	for (int i = 0x2400; i < 0x4000; ++i)
	{
		
		uint8_t bitBlock = memory.read(i);

		//bitBlock = 0x65; //Test value 0110 0101, uncomment to verify screen is being drawn

		g_videoBuffer[bufferIndex] = (bitBlock >> 0) & 0x1 ? 255 : 0;  
		g_videoBuffer[bufferIndex + 1] = (bitBlock >> 1) & 0x1 ? 255 : 0;  
		g_videoBuffer[bufferIndex + 2] = (bitBlock >> 2) & 0x1 ? 255 : 0; 
		g_videoBuffer[bufferIndex + 3] = (bitBlock >> 3) & 0x1 ? 255 : 0; 
		g_videoBuffer[bufferIndex + 4] = (bitBlock >> 4) & 0x1 ? 255 : 0;
		g_videoBuffer[bufferIndex + 5] = (bitBlock >> 5) & 0x1 ? 255 : 0;
		g_videoBuffer[bufferIndex + 6] = (bitBlock >> 6) & 0x1 ? 255 : 0;
		g_videoBuffer[bufferIndex + 7] = (bitBlock >> 7) & 0x1 ? 255 : 0;

		bufferIndex += 8;
	}

	
	//int squareSize = 1024;

	BITMAPINFO bi{};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = width;
	bi.bmiHeader.biHeight = -height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = bitCount;
	bi.bmiHeader.biCompression = BI_RGB;


	//Get current screen size
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	int targetWidth = clientRect.right - clientRect.left;
	int targetHeight = clientRect.bottom - clientRect.top;

	//TODO Find the correct ratio to fit in the screen

	int x = StretchDIBits(hdc, 0, 0, targetWidth, targetHeight, 0, 0, width, height, g_videoBuffer, &bi, DIB_RGB_COLORS, SRCCOPY);

}

/*
void DrawScreenExample(HDC hdc)
{

	void* p = VirtualAlloc(NULL, 512 * 512 * 4, MEM_COMMIT, PAGE_READWRITE);


	if (p == NULL)
		return;

	PBYTE bytes = reinterpret_cast<PBYTE>(p);
	if (bytes == NULL)
		return;

	for (int i = 0; i < 512 * 512 * 4; ++i)
	{
		bytes[i] = rand() % 256;
	}


	BITMAPINFO bi{};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = 512;
	bi.bmiHeader.biHeight = -512;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	int x = StretchDIBits(hdc, 0, 0, 512, 512, 0, 0, 512, 512, bytes, &bi, DIB_RGB_COLORS, SRCCOPY);

	if (p)
	{
		VirtualFree(p, 0, MEM_RELEASE);
	}
}
*/