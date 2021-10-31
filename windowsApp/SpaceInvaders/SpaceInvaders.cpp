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
void PlaySoundResource(int lpResourceName);
void PlaySoundPlayerDie();
void PlaySoundFleetMove1();
void PlaySoundFleetMove2();
void PlaySoundFleetMove3();
void PlaySoundFleetMove4();
void PlaySoundInvaderDie();
void PlaySoundShoot();
void PlaySoundUFO();
void PlaySoundUFOHit();

void DrawScreen(HWND hWnd, HDC hdc);
void LoadROMIntoMemory();

void RefreshScreen();

Adapter platformAdapter;
Machine machine;
Memory memory;
Emulator8080 emulator;
uint8_t* g_videoBuffer;
bool g_gameRunning;
bool g_screenNeedsRefresh;

HWND g_hWndGameWindow;

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
	int screenPixelBufferSize = 256 * 224; //57344 total pixels
	g_videoBuffer = reinterpret_cast<uint8_t*>(std::malloc(screenPixelBufferSize * sizeof(uint8_t)));

	//Connect machine and platform sound output
	platformAdapter.setShootFunction(&PlaySoundShoot);
	platformAdapter.setPlayerDieSoundFunction(&PlaySoundPlayerDie);
	platformAdapter.setInvaderDieFunction(&PlaySoundInvaderDie);
	platformAdapter.setUFOFunction(&PlaySoundUFO);
	platformAdapter.setUFOHitFunction(&PlaySoundUFOHit);
	platformAdapter.setFleetMove1Function(&PlaySoundFleetMove1);
	platformAdapter.setFleetMove2Function(&PlaySoundFleetMove2);
	platformAdapter.setFleetMove3Function(&PlaySoundFleetMove3);
	platformAdapter.setFleetMove4Function(&PlaySoundFleetMove4);
	
	//Screen refresh indicator
	platformAdapter.setRefreshScreenFunction(RefreshScreen);

	machine.setPlatformAdapter(&platformAdapter);

	LoadROMIntoMemory();

	//Create emulator with assigned memory
	emulator.connectMemory(&memory);
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

    // Main message loop:
	/*
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
	*/

	//Main game loop
	g_gameRunning = true;
	while (g_gameRunning)
	{
		//Prepare to gather input
		platformAdapter.setInputChanged(false);

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
		machine.step();
		
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
            // TODO: Add any drawing code that uses hdc here...

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
	int height = 256;
	int width = 224;
	int byteSize = 1; //Each pixel is 8 bits
	int bitCount = byteSize * 8; // could be 8, 16, 24, 32, 64 bits per color, only 0/1 from game
	int totalSize = height * width * byteSize;  // 57,344 total pixels

	//Iterate over each byte of of memory and put bits into video buffer
	//int bufferIndex = 0; //Used for testing drawing raw data (rotated clockwise)
	
	int rowWidth = 224;
	int rowIndex = 255;
	int columnIndex = 0;
	for (int i = 0x2400; i < 0x4000; ++i)
	{
		
		uint8_t bitBlock = memory.read(i);

		//bitBlock = 0x65; //Test value 0110 0101, uncomment to verify screen is being drawn		
		
		/*
		//This draws the screen as in memory, which is rotated 90' clockwise
		g_videoBuffer[bufferIndex] = (bitBlock >> 0) & 0x1 ? 255 : 0;  
		g_videoBuffer[bufferIndex + 1] = (bitBlock >> 1) & 0x1 ? 255 : 0;  
		g_videoBuffer[bufferIndex + 2] = (bitBlock >> 2) & 0x1 ? 255 : 0; 
		g_videoBuffer[bufferIndex + 3] = (bitBlock >> 3) & 0x1 ? 255 : 0; 
		g_videoBuffer[bufferIndex + 4] = (bitBlock >> 4) & 0x1 ? 255 : 0;
		g_videoBuffer[bufferIndex + 5] = (bitBlock >> 5) & 0x1 ? 255 : 0;
		g_videoBuffer[bufferIndex + 6] = (bitBlock >> 6) & 0x1 ? 255 : 0;
		g_videoBuffer[bufferIndex + 7] = (bitBlock >> 7) & 0x1 ? 255 : 0;

		bufferIndex += 8;
		*/

		//Rotate pixels counter-clockwise, so draw every column bottom-up
		uint8_t low = 0x00;
		uint8_t high = 0xFF;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 0) & 0x1 ? high : low;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 1) & 0x1 ? high : low;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 2) & 0x1 ? high : low;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 3) & 0x1 ? high : low;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 4) & 0x1 ? high : low;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 5) & 0x1 ? high : low;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 6) & 0x1 ? high : low;
		g_videoBuffer[rowIndex-- * rowWidth + columnIndex] = (bitBlock >> 7) & 0x1 ? high : low;

		if (rowIndex < 0)
		{
			rowIndex = 255;
			columnIndex += 1;
		}
		
	}

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

//https://stackoverflow.com/questions/9240188/how-to-load-a-custom-binary-resource-in-a-vc-static-library-as-part-of-a-dll
void LoadROMIntoMemory()
{
	HRSRC myResource = ::FindResource(NULL, MAKEINTRESOURCE(IDR_RCDATA1), RT_RCDATA);
	if (!myResource)
		return;
	unsigned int myResourceSize = ::SizeofResource(NULL, myResource);


	HGLOBAL myResourceData = ::LoadResource(NULL, myResource);
	if (!myResourceData)
		return;

	void* pMyBinaryData = ::LockResource(myResourceData);
	if (!pMyBinaryData)
		return;

	char* result = reinterpret_cast<char*>(pMyBinaryData);

	//Create memory block and assign to memory
	std::unique_ptr<std::vector<uint8_t>> memoryBlock =
		std::make_unique<std::vector<uint8_t>>(0x4000);

	memoryBlock->insert(memoryBlock->begin(), result, result + myResourceSize);
	
	//memcpy(memoryBlock->data(), &pMyBinaryData[0], myResourceSize * sizeof(char));

	//Assign memory
	memory.setMemoryBlock(std::move(memoryBlock));

	UnlockResource(myResourceData);
	FreeResource(myResourceData);
}

/*
void LoadROMIntoMemory()
{
	//Create memory block and assign to memory
	std::unique_ptr<std::vector<uint8_t>> memoryBlock =
		std::make_unique<std::vector<uint8_t>>(0x4000);

	//Read ROM into the memory
	//https://www.cplusplus.com/doc/tutorial/files/
	std::ifstream romFile;

	//Open file for reading, at end of file
	romFile.open("..\\..\\roms\\invaders\\invaders", std::ios::binary|std::ios::ate);
	if (romFile.is_open())
	{
		// Get file size
		int romLength = romFile.tellg();
		// Reset file pointer to beginning
		romFile.seekg(0, std::ios::beg);
		// Create a buffer and read into it
		romFile.read(reinterpret_cast<char*>(memoryBlock->data()), romLength);
		romFile.close();

		//Assign memory
		memory.setMemoryBlock(std::move(memoryBlock));
	}
	else
	{
		std::cerr << "Failed to read ROM file" << std::endl;
	}

}
*/

void PlaySoundResource(int lpResourceName)
{
	HRSRC hResInfo;
	HANDLE hRes;
	LPCWSTR lpWavInMemory;

	hResInfo = FindResource(NULL, MAKEINTRESOURCE(lpResourceName), L"WAVE");

	if (hResInfo == NULL)
		return;

	hRes = LoadResource(NULL, hResInfo);

	if (hRes == NULL)
		return;

	lpWavInMemory = (LPCWSTR)LockResource(hRes);

	sndPlaySound(lpWavInMemory, SND_MEMORY | SND_ASYNC |
		SND_NODEFAULT);

	UnlockResource(hRes);
	FreeResource(hRes);
}

void PlaySoundPlayerDie()
{
	PlaySoundResource(IDR_PLAYER_DIE);
}

void PlaySoundFleetMove1()
{
	PlaySoundResource(IDR_FLEET_MOVE_1);
}

void PlaySoundFleetMove2()
{
	PlaySoundResource(IDR_FLEET_MOVE_2);
}

void PlaySoundFleetMove3()
{
	PlaySoundResource(IDR_FLEET_MOVE_3);
}

void PlaySoundFleetMove4()
{
	PlaySoundResource(IDR_FLEET_MOVE_4);
}

void PlaySoundInvaderDie()
{
	PlaySoundResource(IDR_INVADER_DIE);
}

void PlaySoundShoot()
{
	PlaySoundResource(IDR_SHOOT);
}

void PlaySoundUFO()
{
	PlaySoundResource(IDR_UFO);
}

void PlaySoundUFOHit()
{
	PlaySoundResource(IDR_UFO_HIT);
}

void RefreshScreen()
{
	InvalidateRect(g_hWndGameWindow, 0, 0);
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