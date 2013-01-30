// Connect3.cpp : Defines the entry point for the application.
// Copyright © 2011 Steve Coward

#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <list>
#include <windows.h>
#include <process.h> // for _beginthread(), _endthreadex()
#include "Connect3AIThread.h"
#include "Render.h"
#include "Connect3.h"
#include <assert.h>
#include <io.h> // Console
#include <fcntl.h> // Console

#define MAX_LOADSTRING 100

//struct checker {
//	int rgb[3];
//	int x;
//	int y;
//};

// Global Variables:
CRender* gAppGraphics;
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HWND g_hWnd;
CConnect3AIThread* pThread;
int gWinWidth;
int gWinHeight;
CRITICAL_SECTION g_csMoveList;
CRITICAL_SECTION g_csCheckerList;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	QuitConfirm(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Help(HWND, UINT, WPARAM, LPARAM);


unsigned __stdcall Connect3AIThreadFunc(void* ptr)
{
	CConnect3AIThread* pThread;
	pThread = (CConnect3AIThread*)ptr;
	pThread->Connect3();

	_endthreadex( 0 );

	return(0);
}


VOID CALLBACK RenderTimerCallBack(
	HWND hwnd, // handle to window
	UINT uMsg, // WM_TIMER message
	UINT_PTR idEvent, // timer identifier
	DWORD dwTime // current system time
	)
{
	gAppGraphics->RenderIt();
	//std::cout << "Finished Rendering.\n" << std::flush;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	MSG msg;
	HACCEL hAccelTable;

	// define game parameters
	int numRows = NUMROWS;
	int numCols = NUMCOLS;
	int numCons = NUMCONS;

	InitializeCriticalSection( &g_csMoveList );
	InitializeCriticalSection( &g_csCheckerList );

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CONNECT3, szWindowClass, MAX_LOADSTRING);

	// Allocate a console window for debug
	AllocConsole();

	HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long) handle_out, _O_TEXT);
	FILE* hf_out = _fdopen(hCrt, "w");
	setvbuf(hf_out, NULL, _IONBF, 1);
	*stdout = *hf_out;

	HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
	hCrt = _open_osfhandle((long) handle_in, _O_TEXT);
	FILE* hf_in = _fdopen(hCrt, "r");
    setvbuf(hf_in, NULL, _IONBF, 128);
    *stdin = *hf_in;

	// Create an instance of the rendering class
	gAppGraphics = new CRender(numRows, numCols);
	//gAppGraphics->m_csCheckerList = g_csCheckerList;
	//gAppGraphics->m_csMoveList = g_csMoveList;

	gWinHeight = numRows * gAppGraphics->m_cellSize +
				 gAppGraphics->m_borderTop +
				 gAppGraphics->m_borderBot;
	gWinWidth  = gAppGraphics->m_borderL +
				 numCols * gAppGraphics->m_cellSize +
				 gAppGraphics->m_borderR +
				 gAppGraphics->m_historyW;

	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CONNECT3));

	bool bDoThread = true;
	if (bDoThread) {
		pThread = new CConnect3AIThread(numRows, numCols, numCons);
		if (pThread != NULL) {
			HANDLE hThread;
			unsigned ThreadID;
			hThread = (HANDLE)_beginthreadex(
				NULL,
				0,
				Connect3AIThreadFunc,
				(void*)pThread,
				CREATE_SUSPENDED,
				&ThreadID
				);

			if (hThread == NULL) {
				delete pThread;
				pThread = NULL;
				return 1;
			}
			else {
				gAppGraphics->m_pThreadAI = pThread;
				pThread->setThreadID(ThreadID);
				pThread->setThreadHandle(hThread);
				pThread->setParentThreadID(GetCurrentThreadId());
				pThread->setCellSize(gAppGraphics->m_cellSize);
				pThread->setBoardPosX(gAppGraphics->m_borderL);
				pThread->setBoardPosY(gAppGraphics->m_borderBot);
				pThread->setDisplayHeight(gAppGraphics->m_winHeight);
				pThread->setDisplayWidth(gAppGraphics->m_winWidth);

				ResumeThread(hThread);
			}
		}
	}
#ifdef _DEBUG
	else {
		// non threaded debug mode
		pThread = new CConnect3AIThread(numRows, numCols, numCons);
		if (pThread != NULL) {
			gAppGraphics->m_pThreadAI = pThread;
			pThread->setCellSize(80);
			pThread->setBoardPosX(30);
			pThread->setBoardPosY(80);
			pThread->setDisplayHeight(gWinHeight);
			pThread->setDisplayWidth(gWinWidth);
		}
	}
#endif // _DEBUG

	// Expects resource ID value to be dependent on length of time out for each level
	pThread->setTimeOut(IDM_LEVEL_10SECONDS-IDM_LEVEL_FIRSTLEVEL);
	HMENU hmenuMain = GetMenu(g_hWnd);
	CheckMenuItem(hmenuMain, IDM_LEVEL_10SECONDS, MF_CHECKED);
				
	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);

	// prime the message structure
	PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);

	// Create a timer to ensure timely screen redraws
	UINT RenderTimer = ::SetTimer(
		NULL, 12345, FRAMESPERSEC, RenderTimerCallBack
	);
	

	bool bGotMsg;
	while (WM_QUIT != msg.message)
	{
		bGotMsg = (GetMessage(&msg, NULL, 0U, 0U) != 0);

		if (bGotMsg)
		{
			if (!TranslateAccelerator(g_hWnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			// Translate and dispatch the message
			//TranslateMessage(&msg);
			//DispatchMessage(&msg);
		}
		//::Sleep(250);
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CONNECT3));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_CONNECT3);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
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
   //HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   g_hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      10, 10, gWinWidth, gWinHeight, NULL, NULL, hInstance, NULL);

   if (!g_hWnd)
   {
      return FALSE;
   }

   //ShowWindow(g_hWnd, nCmdShow);
   //UpdateWindow(g_hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_CREATE:
		{
			gAppGraphics->CreateRenderContext(hWnd);
			break;
		}
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_HELP:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_HELPBOX), hWnd, Help);
			break;
		case IDM_NEWGAME:
			{
				Cmd* c = new Cmd();
				c->cmd = NEW_GAME;
				c->p.x = 0; 
				c->p.y = 0; 
				pThread->m_qCmds.push(c);
				break;
			}
		case IDM_LEVEL_DISABLETIMER:
			{
				Cmd* c = new Cmd();
				c->cmd = DISABLE_TIMER;
				c->p.x = 0; 
				c->p.y = 0; 
				pThread->m_qCmds.push(c);
				break;
			}
		case IDM_HINT:
			{
				Cmd* c = new Cmd();
				c->cmd = HINT;
				c->p.x = 0; 
				c->p.y = 0; 
				pThread->m_qCmds.push(c);
				break;
			}
		case IDM_SWAPSIDES:
			{
				Cmd* c = new Cmd();
				c->cmd = SWAP_SIDES;
				c->p.x = 0; 
				c->p.y = 0; 
				pThread->m_qCmds.push(c);
				break;
			}
		case IDM_ANALYSIS:
			{
				Cmd* c = new Cmd();
				c->cmd = ANALYSIS;
				c->p.x = 0; 
				c->p.y = 0; 
				pThread->m_qCmds.push(c);
				break;
			}
		case IDM_SETUPGAME:
			{
				Cmd* c = new Cmd();
				c->cmd = SETUP_GAME;
				c->p.x = 0; 
				c->p.y = 0; 
				pThread->m_qCmds.push(c);
				break;
			}
		case IDM_UNDOMOVE:
			{
				Cmd* c = new Cmd();
				c->cmd = UNDO;
				c->p.x = 0; 
				c->p.y = 0; 
				pThread->m_qCmds.push(c);
				break;
			}
		case IDM_FORCEMOVE:
			{
				pThread->setForceMove(true);

				break;
			}
		case IDM_LEVEL_1SECOND:
		case IDM_LEVEL_2SECONDS:
		case IDM_LEVEL_5SECONDS:
		case IDM_LEVEL_10SECONDS:
		case IDM_LEVEL_30SECONDS:
		case IDM_LEVEL_1MINUTE:
			{
				pThread->setTimeOut((wmId - IDM_LEVEL_0SECOND));

				// Could probably query sub menu for the number of items instead
				HMENU hmenuMain = GetMenu(hWnd);
				for (int i=0; i<=IDM_LEVEL_LASTLEVEL-IDM_LEVEL_FIRSTLEVEL; i++) {
					CheckMenuItem(hmenuMain, IDM_LEVEL_FIRSTLEVEL+i, MF_UNCHECKED);
				}
				CheckMenuItem(hmenuMain, wmId, MF_CHECKED);
				break;
			}
		case IDM_EXIT:
			{
				int iResults = MessageBox(
					NULL,
					L"Are you sure you want to quit?",
					L"Quit?",
					MB_YESNO
					);
				if (iResults == IDYES) {
					DestroyWindow(hWnd);
					DeleteCriticalSection( &g_csMoveList );
					DeleteCriticalSection( &g_csCheckerList );
				}
				break;
			}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_LBUTTONUP:
		{
			Cmd* c = new Cmd();
			c->cmd = CLICK;
			c->p.x = lParam & 0x0000FFFF; 
			c->p.y = (lParam & 0xFFFF0000) >> 16; 
			pThread->m_qCmds.push(c);
			break;
		}
	case WM_RBUTTONUP:
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
// Message handler for help box.
INT_PTR CALLBACK Help(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
