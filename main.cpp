// main.cpp : Defines the entry point for the application.
// By Geelix School of Serious Games and Edutainment.

#include "windows.h"
#include "DXRayTrace.h"

// Current Instance & Main Window Handle
HINSTANCE g_hInst;
HWND g_hWnd;

// Function declarations (included in this C++ file)
ATOM MyRegisterClass(HINSTANCE hInst);
bool CreateMainWnd(int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


// Main entry point function
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    // Initialization
    g_hInst = hInstance;
    g_hWnd = NULL;
    MyRegisterClass(hInstance);

    // Create main window
    if (!CreateMainWnd(nCmdShow)) return -1;

    // Initialize demo
    DXRayTrace rt_app;
    if (!rt_app.Initialize(g_hWnd, g_hInst)) return -1;

    // Main message loop
    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

        // Update and render
        rt_app.Update();
        rt_app.Render();
    }

    // Terminate rt_app
    rt_app.Terminate();

    return (int)msg.wParam;
}


// Registers the window class
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "DXRayTraceClass";
    wcex.hIconSm = NULL;

    return RegisterClassEx(&wcex);
}


// Creates the main window
bool CreateMainWnd(int nCmdShow)
{
    // Calculate main window size
    RECT rc = { 0, 0, 800, 600 };
    ::AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the main window
    g_hWnd = ::CreateWindow(
		"DXRayTraceClass",
		"DXRayTrace",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		NULL,
		NULL,
		g_hInst,
		NULL
	);
    
    // Check window handle
    if (g_hWnd == NULL) return false;

    ::ShowWindow(g_hWnd, nCmdShow);
    ::UpdateWindow(g_hWnd);

    return true;
}


// Processes messages for the main window
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = ::BeginPaint(hWnd, &ps);
        ::EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;

    default:
        return ::DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}
