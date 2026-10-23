#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>

void MimicCtrlC() {
    while ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || 
           (GetAsyncKeyState(VK_MENU) & 0x8000) || 
           (GetAsyncKeyState('K') & 0x8000)) {
        Sleep(10);
    }

    INPUT inputs[4] = {0};

    inputs[0].type = INPUT_KEYBOARD; inputs[0].ki.wVk = VK_CONTROL;
    inputs[1].type = INPUT_KEYBOARD; inputs[1].ki.wVk = 'C';
    inputs[2].type = INPUT_KEYBOARD; inputs[2].ki.wVk = 'C'; inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD; inputs[3].ki.wVk = VK_CONTROL; inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(4, inputs, sizeof(INPUT));
    Sleep(50); 
}

void MimicCtrlv() {
    while ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || 
           (GetAsyncKeyState(VK_MENU) & 0x8000) || 
           (GetAsyncKeyState('K') & 0x8000)) {
        Sleep(10);
    }

    INPUT inputs[4] = {0};

    inputs[0].type = INPUT_KEYBOARD; inputs[0].ki.wVk = VK_CONTROL;
    inputs[1].type = INPUT_KEYBOARD; inputs[1].ki.wVk = 'V';
    inputs[2].type = INPUT_KEYBOARD; inputs[2].ki.wVk = 'V'; inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD; inputs[3].ki.wVk = VK_CONTROL; inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(4, inputs, sizeof(INPUT));
}

int copy_to_clipboard_w(const wchar_t* text) {
    if (text == NULL) return 0;

    size_t len = (wcslen(text) + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    if (!hMem) return 0;

    memcpy(GlobalLock(hMem), text, len);
    GlobalUnlock(hMem);

    if (!OpenClipboard(NULL)) {
        GlobalFree(hMem);
        return 0;
    }
    
    EmptyClipboard();               
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();               
    return 1;
}

wchar_t* get_clipboard_data_w() {
    if (!OpenClipboard(NULL)) return NULL;

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == NULL) {
        CloseClipboard();
        return NULL;
    }

    wchar_t *pszText = (wchar_t*)GlobalLock(hData);
    wchar_t *resultBuffer = NULL;

    if (pszText != NULL) {
        resultBuffer = malloc((wcslen(pszText) + 1) * sizeof(wchar_t));
        if (resultBuffer != NULL) {
            wcscpy(resultBuffer, pszText);
        }
        GlobalUnlock(hData);
    }

    CloseClipboard();
    return resultBuffer; 
}

UINT char_to_vk(wchar_t character, HKL layout) {
    SHORT rawResult = VkKeyScanExW(character, layout);
    if (rawResult == -1) return 0xFFFFFFFF;
    return (UINT)rawResult;
}

int VirtualKeyToCharEx(UINT vkCode, HKL keyboardLayout, wchar_t* outBuffer, int bufferSize, int shiftPressed) {
    UINT scanCode = MapVirtualKeyEx(vkCode, MAPVK_VK_TO_VSC, keyboardLayout);
    BYTE keyboardState[256] = {0};
    if (shiftPressed) {
        keyboardState[VK_SHIFT] = 0x80; 
    }

    return ToUnicodeEx(vkCode, scanCode, keyboardState, outBuffer, bufferSize, 0, keyboardLayout);
}

void lang_change(const wchar_t* text, HKL layoutFrom, HKL layoutTo) {
    if (!text) return;
    size_t len = wcslen(text);
    wchar_t* resText = malloc((len + 1) * sizeof(wchar_t));
    
    for (size_t i = 0; i < len; i++) {
        UINT vk = char_to_vk(text[i], layoutFrom);
        wchar_t currbuff[4] = {0}; 
        
        
        int shift = (vk != 0xFFFFFFFF && (HIBYTE(vk) & 1)) ? 1 : 0;
        
        if (vk != 0xFFFFFFFF && VirtualKeyToCharEx(LOBYTE(vk), layoutTo, currbuff, 4, shift) > 0) {
            resText[i] = currbuff[0];
        } else {
            resText[i] = text[i]; 
        }
    }
    resText[len] = L'\0';
    
    copy_to_clipboard_w(resText);
    free(resText);
}

int main() {
    HKL keyboardHEB = LoadKeyboardLayoutW(L"0000040D", KLF_NOTELLSHELL);
    HKL keyboardENG = LoadKeyboardLayoutW(L"00000409", KLF_NOTELLSHELL);  
    
    if (!keyboardHEB || !keyboardENG) {
        printf("Failed to load keyboard layouts.\n");
        return 1;
    }

    if (RegisterHotKey(NULL, 1, MOD_CONTROL | MOD_ALT, 'K')) {
        printf("Hotkey (Ctrl + Alt + K) registered successfully.\n");
    } else {
        printf("Failed to register hotkey. Error code: %lu\n", GetLastError());
        return 1;
    }

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            MimicCtrlC();
            wchar_t *clipboardData = get_clipboard_data_w();
            if (clipboardData) {
                if (clipboardData[0] >= 'A' && clipboardData[0] <= 'z')
                    lang_change(clipboardData, keyboardENG, keyboardHEB);
                else
                    lang_change(clipboardData, keyboardHEB, keyboardENG);
                MimicCtrlv();
                free(clipboardData);
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterHotKey(NULL, 1);
    return 0;
}
