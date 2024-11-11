#include <Windows.h>
#include <chrono>
#include <thread>

HHOOK hHook = nullptr;
bool windowsKeyPressed = false;
bool otherKeyPressed = false;

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

        // Handle key press
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            if (pKeyboard->vkCode == VK_LWIN)
            {
                windowsKeyPressed = true;
                otherKeyPressed = false;  // Reset when Windows key is pressed
                return 1;  // Block the default Windows key action for now
            }

            if (windowsKeyPressed && pKeyboard->vkCode != VK_LWIN)
            {
                otherKeyPressed = true;  // Another key is pressed while Windows key is down
                // Simulate Win + [key]
                keybd_event(VK_LWIN, 0, 0, 0);  // Simulate Windows key press
                keybd_event((BYTE)pKeyboard->vkCode, 0, 0, 0);  // Simulate the other key press
                keybd_event((BYTE)pKeyboard->vkCode, 0, KEYEVENTF_KEYUP, 0);  // Release the other key
                keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);  // Release Windows key

                return 1;  // Block this event from propagating further
            }
        }

        // Handle Windows key release
        if (wParam == WM_KEYUP && pKeyboard->vkCode == VK_LWIN)
        {
            if (!otherKeyPressed)
            {
                // Trigger Alt + Space if no other key was pressed with Windows key
                std::thread([]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Add a slight delay
                    TriggerAltSpace();
                    }).detach();  // Use a detached thread to avoid blocking
            }

            windowsKeyPressed = false;  // Reset key tracking
            return 1;  // Block the default Windows key behavior
        }
    }

    return CallNextHookEx(hHook, nCode, wParam, lParam);  // Pass to the next hook
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
    //HWND Console = GetConsoleWindow();
    //ShowWindow(Console, SW_HIDE);
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);
    if (!hHook)
    {
        MessageBox(nullptr, L"Failed to install hook!", L"Error", MB_ICONERROR);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    return 0;
}