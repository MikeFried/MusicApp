#include "MusicApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Register the window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(nullptr, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Create the window
    HWND hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        L"Music Performance App",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd)
    {
        MessageBoxW(nullptr, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Create the menu
    CreateMainMenu(hwnd);

    // Show the window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
            HandleCommand(hwnd, wParam);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            EndPaint(hwnd, &ps);
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CreateMainMenu(HWND hwnd)
{
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();

    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_SETTINGS, L"Settings");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_EXIT, L"Exit");

    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"File");

    SetMenu(hwnd, hMenuBar);
}

void HandleCommand(HWND hwnd, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        case ID_FILE_SETTINGS:
        {
            ConfigDialog dialog(hwnd);
            dialog.Show();
            break;
        }

        case ID_FILE_EXIT:
            DestroyWindow(hwnd);
            break;
    }
} 