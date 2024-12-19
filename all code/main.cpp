#include "resource.h"

#include <Windows.h>
#include <chrono>
#include <thread>
#include <iostream>

HHOOK hHook = nullptr;
NOTIFYICONDATA nid = {};
bool windowsKeyPressed = false;
bool shiftKeyPressed = false;
bool ctrlKeyPressed = false;
bool otherKeyPressed = false;

bool isOnStartUpChecked = false;
bool isInTaskbarChecked = false;

void StoreInRegistry() {
    HKEY hKey;
    const std::wstring keyPath = L"Software\\RunAsWinKey";  // path of the registry file

    // open reg file
    if (RegCreateKeyEx(HKEY_CURRENT_USER, keyPath.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        
        // save values
        RegSetValueEx(hKey, L"OnStartup", 0, REG_DWORD, (const BYTE*)&isOnStartUpChecked, sizeof(DWORD));
        RegSetValueEx(hKey, L"InTaskbar", 0, REG_DWORD, (const BYTE*)&isInTaskbarChecked, sizeof(DWORD));

        RegCloseKey(hKey);
    }
}

void ReadFromRegistry() {
    HKEY hKey;
    const std::wstring keyPath = L"Software\\MyAppSettings";  // path

    // open reg
    if (RegOpenKeyEx(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD settingValue = 0;
        DWORD size = sizeof(DWORD);

        // read values
        if (RegQueryValueEx(hKey, L"OnStartup", 0, NULL, (LPBYTE)&settingValue, &size) == ERROR_SUCCESS) {
            isOnStartUpChecked = (settingValue != 0);  // Convert DWORD to bool
        }

        if (RegQueryValueEx(hKey, L"InTaskbar", 0, NULL, (LPBYTE)&settingValue, &size) == ERROR_SUCCESS) {
            isInTaskbarChecked = (settingValue != 0);  // Convert DWORD to bool
        }

        RegCloseKey(hKey);
    }
}

bool SetStartupRegistryEntry(bool enable) {
    HKEY hKey;
    const wchar_t* keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    if (RegOpenKeyEx(HKEY_CURRENT_USER, keyPath, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    if (enable) {
        // Get the path of the current executable
        wchar_t exePath[MAX_PATH];
        GetModuleFileName(NULL, exePath, MAX_PATH);

        // Add the registry value
        RegSetValueEx(hKey, L"PTRunAsWinKey", 0, REG_SZ,
            (const BYTE*)exePath,
            (wcslen(exePath) + 1) * sizeof(wchar_t));
    }
    else {
        // Remove the registry value
        RegDeleteValue(hKey, L"PTRunAsWinKey");
    }

    RegCloseKey(hKey);
    return true;
}

// ----------------------------------------------------------------------------------------

bool SetTaskbarVisibility(HWND hWnd, bool show) {
    if (show) {
        // Show in taskbar
        SetWindowLong(hWnd, GWL_EXSTYLE,
            GetWindowLong(hWnd, GWL_EXSTYLE) & ~WS_EX_TOOLWINDOW);
        ShowWindow(hWnd, SW_SHOW);
    }
    else {
        // Hide from taskbar
        SetWindowLong(hWnd, GWL_EXSTYLE,
            GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
        ShowWindow(hWnd, SW_HIDE);
    }
    return true;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_USER + 1)  // Custom tray icon message
    {
        if (lParam == WM_RBUTTONUP)  // Right-click on tray icon
        {
            HMENU hMenu = CreatePopupMenu();

            // Create menu items with proper checkmark flags
            AppendMenu(hMenu, MF_STRING, 1, L"Run on Startup");
            AppendMenu(hMenu, MF_STRING, 2, L"Show in Taskbar");
            AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(hMenu, MF_STRING, 3, L"Exit");

            // Set the check state separately using CheckMenuItem
            CheckMenuItem(hMenu, 1, MF_BYCOMMAND | (isOnStartUpChecked ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hMenu, 2, MF_BYCOMMAND | (isInTaskbarChecked ? MF_CHECKED : MF_UNCHECKED));

            POINT mousePointer;
            GetCursorPos(&mousePointer);
            SetForegroundWindow(hWnd);

            int trackMenu = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                mousePointer.x, mousePointer.y, 0, hWnd, nullptr);

            DestroyMenu(hMenu);
            
            switch (trackMenu) {
            case 1:  // Run on Startup
                isOnStartUpChecked = !isOnStartUpChecked;
                SetStartupRegistryEntry(isOnStartUpChecked);
                StoreInRegistry();
                break;
            case 2:  // Show in Taskbar
                isInTaskbarChecked = !isInTaskbarChecked;
                SetTaskbarVisibility(hWnd, isInTaskbarChecked);
                StoreInRegistry();
                break;
            case 3:  // Exit
                PostQuitMessage(0);
                break;
            }

        }

        //if (lParam == WM_RBUTTONUP)  // Right-click on tray icon
        //{
        //    HMENU hMenu = CreatePopupMenu();

        //    // Create menu items
        //    AppendMenu(hMenu, MF_STRING | (isOnStartUpChecked ? MF_CHECKED : MF_UNCHECKED),
        //        1, L"Run on Startup");
        //    AppendMenu(hMenu, MF_STRING | (isInTaskbarChecked ? MF_CHECKED : MF_UNCHECKED),
        //        2, L"Show in Taskbar");
        //    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
        //    AppendMenu(hMenu, MF_STRING, 3, L"Exit");

        //    POINT mousePointer;
        //    GetCursorPos(&mousePointer);
        //    SetForegroundWindow(hWnd);

        //    int trackMenu = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
        //        mousePointer.x, mousePointer.y, 0, hWnd, nullptr);

        //    DestroyMenu(hMenu);

        //    switch (trackMenu) {
        //    case 1:  // Run on Startup
        //        isOnStartUpChecked = !isOnStartUpChecked;
        //        SetStartupRegistryEntry(isOnStartUpChecked);
        //        StoreInRegistry();
        //        break;

        //    case 2:  // Show in Taskbar
        //        isInTaskbarChecked = !isInTaskbarChecked;
        //        SetTaskbarVisibility(hWnd, isInTaskbarChecked);
        //        StoreInRegistry();
        //        break;

        //    case 3:  // Exit
        //        PostQuitMessage(0);
        //        break;
        //    }
        //}
    }
    else if (message == WM_CREATE)
    {
        // Initialize window state based on saved settings
        SetTaskbarVisibility(hWnd, isInTaskbarChecked);
        if (isOnStartUpChecked) {
            SetStartupRegistryEntry(true);
        }
    }
    else if (message == WM_DESTROY)
    {
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// ----------------------------------------------------------------------------------------

void TriggerAltSpace()
{
    keybd_event(VK_MENU, 0, 0, 0);  // Press Alt key
    keybd_event(VK_SPACE, 0, 0, 0);  // Press Space key
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);  // Release Alt key
    keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);  // Release Space key
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;

        // Handle key press events
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            switch (pKeyboard->vkCode)
            {
            case VK_LWIN:
                windowsKeyPressed = true;
                otherKeyPressed = false;
                return 1;  // Block single Windows key action temporarily

            case VK_SHIFT:
                shiftKeyPressed = true;
                break;

            case VK_CONTROL:
                ctrlKeyPressed = true;
                break;

            default:
                if (windowsKeyPressed)
                {
                    otherKeyPressed = true;

                    // Simulate Win + Shift/Ctrl + Key combinations
                    if (shiftKeyPressed)
                        keybd_event(VK_SHIFT, 0, 0, 0);  // Press Shift key if needed
                    if (ctrlKeyPressed)
                        keybd_event(VK_CONTROL, 0, 0, 0);  // Press Ctrl key if needed

                    // Simulate Windows key + current key
                    keybd_event(VK_LWIN, 0, 0, 0);
                    keybd_event((BYTE)pKeyboard->vkCode, 0, 0, 0);

                    // Release keys in reverse order
                    keybd_event((BYTE)pKeyboard->vkCode, 0, KEYEVENTF_KEYUP, 0);
                    if (shiftKeyPressed)
                        keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
                    if (ctrlKeyPressed)
                        keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
                    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);

                    return 1;  // Block further processing
                }
                break;
            }
        }

        // Handle key release events
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
        {
            switch (pKeyboard->vkCode)
            {
            case VK_LWIN:
                if (!otherKeyPressed)
                {
                    // Trigger Alt + Space if only Windows key was pressed
                    std::thread([]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        TriggerAltSpace();
                        }).detach();
                }
                windowsKeyPressed = false;
                return 1;

            case VK_SHIFT:
                shiftKeyPressed = false;
                break;

            case VK_CONTROL:
                ctrlKeyPressed = false;
                break;
            }
        }
    }

    return CallNextHookEx(hHook, nCode, wParam, lParam);  // Pass to next hook
}

// ----------------------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TrayAppClass";

    if (!RegisterClass(&wc)) {
        MessageBox(nullptr, L"Failed to register window class!", L"Error", MB_ICONERROR);
        return 1;
    }

    // Create the window
    HWND hWnd = CreateWindowEx(
        0,                      // Extended style
        wc.lpszClassName,       // Class name
        L"Tray App",           // Window title
        WS_OVERLAPPEDWINDOW,    // Style
        CW_USEDEFAULT,         // X position
        CW_USEDEFAULT,         // Y position
        CW_USEDEFAULT,         // Width
        CW_USEDEFAULT,         // Height
        NULL,                  // Parent window
        NULL,                  // Menu
        hInstance,             // Instance handle
        NULL                   // Additional data
    );

    if (!hWnd) {
        MessageBox(nullptr, L"Failed to create window!", L"Error", MB_ICONERROR);
        return 1;
    }

    // Load the icon
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
    if (!hIcon) {
        MessageBox(nullptr, L"Failed to load icon!", L"Error", MB_ICONERROR);
        return 1;
    }

    // Setup tray icon
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = WM_USER + 1;  // This matches the message in WindowProc
    nid.hIcon = hIcon;
    wcscpy_s(nid.szTip, L"PTRun As Win-Key");

    // Add the tray icon
    if (!Shell_NotifyIcon(NIM_ADD, &nid)) {
        MessageBox(nullptr, L"Failed to create tray icon!", L"Error", MB_ICONERROR);
        DestroyWindow(hWnd);
        return 1;
    }

    // Hide the main window
    ShowWindow(hWnd, SW_HIDE);

    // Set up the keyboard hook
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);
    if (!hHook) {
        MessageBox(nullptr, L"Failed to install hook!", L"Error", MB_ICONERROR);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        DestroyWindow(hWnd);
        return 1;
    }

    // Read settings from registry
    ReadFromRegistry();

    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    Shell_NotifyIcon(NIM_DELETE, &nid);
    if (hHook) {
        UnhookWindowsHookEx(hHook);
    }

    return static_cast<int>(msg.wParam);
}