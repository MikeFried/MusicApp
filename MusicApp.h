#pragma once

#define UNICODE
#define _UNICODE
#include <windows.h>

// Window class name
#define WINDOW_CLASS_NAME L"MusicAppMainWindow"

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateMainMenu(HWND hwnd);
void HandleCommand(HWND hwnd, WPARAM wParam);

// Menu command IDs
#define ID_FILE_SETTINGS 1001
#define ID_FILE_EXIT 1002 