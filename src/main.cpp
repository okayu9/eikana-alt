// Copyright 2026 Yumeto Inaoka
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <windows.h>
#include <shellapi.h>

#include "AltSoloDetector.h"
#include "AppState.h"
#include "Autostart.h"
#include "CapsRemapInstaller.h"
#include "ImeController.h"
#include "InjectedTag.h"
#include "KeyboardHook.h"
#include "Resource.h"
#include "TrayIcon.h"

#include <array>
#include <cwchar>

namespace {

constexpr wchar_t kWindowClass[] = L"EikanaAltMessageWindow";
constexpr wchar_t kMutexName[]   = L"Local\\eikana-alt-singleton";

HWND              g_hwnd = nullptr;
AltSoloDetector   g_detector;
TrayIcon          g_tray;
HANDLE            g_instanceMutex = nullptr;
UINT              g_taskbarCreatedMsg = 0;

struct ElevatedAction {
    const wchar_t* flag;
    int (*handler)() noexcept;
};

constexpr ElevatedAction kElevatedActions[] = {
    {L"--install-caps-remap",   &CapsRemapInstaller::installAsElevated},
    {L"--uninstall-caps-remap", &CapsRemapInstaller::uninstallAsElevated},
};

[[nodiscard]] int dispatchElevated() noexcept {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return -1;

    int result = -1;
    if (argc >= 2) {
        for (const auto& a : kElevatedActions) {
            if (std::wcscmp(argv[1], a.flag) == 0) {
                result = a.handler();
                break;
            }
        }
    }
    LocalFree(argv);
    return result;
}

// After swallowing the solo Alt up, inject an inert key + the matching Alt up
// so Windows clears its menu-bar trigger flag. VK_F24 is essentially never
// bound by applications, unlike VK_NONAME which aliases an IMM commit code.
void injectMenuSuppressionAndAltUp(bool isLeft) noexcept {
    const WORD altVk = isLeft ? VK_LMENU : VK_RMENU;
    const DWORD altFlags = KEYEVENTF_KEYUP | (isLeft ? 0u : KEYEVENTF_EXTENDEDKEY);

    std::array<INPUT, 3> inputs = {{
        {.type = INPUT_KEYBOARD,
         .ki = {.wVk = VK_F24, .dwExtraInfo = Injected::Tag}},
        {.type = INPUT_KEYBOARD,
         .ki = {.wVk = VK_F24, .dwFlags = KEYEVENTF_KEYUP, .dwExtraInfo = Injected::Tag}},
        {.type = INPUT_KEYBOARD,
         .ki = {.wVk = altVk, .dwFlags = altFlags, .dwExtraInfo = Injected::Tag}},
    }};
    SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}

bool onKeyEvent(WPARAM wParam, const KBDLLHOOKSTRUCT& kbd) noexcept {
    if (kbd.dwExtraInfo == Injected::Tag) [[unlikely]] return false;
    if (!AppState::isEnabled())            [[unlikely]] return false;
    return g_detector.onKeyEvent(wParam, kbd);
}

void onSoloAlt(AltSoloDetector::SoloKey which) noexcept {
    const bool isLeft = (which == AltSoloDetector::SoloKey::LeftAlt);
    injectMenuSuppressionAndAltUp(isLeft);
    HWND captured = GetForegroundWindow();
    (void)PostMessageW(g_hwnd, WM_APP_SOLO_ALT,
                       static_cast<WPARAM>(which),
                       reinterpret_cast<LPARAM>(captured));
}

void handleToggleAutostart(HWND owner) noexcept {
    const bool target = !Autostart::isEnabled();
    if (!Autostart::setEnabled(target)) {
        MessageBoxW(owner,
                    L"自動起動の設定変更に失敗しました。",
                    L"eikana-alt",
                    MB_OK | MB_ICONERROR);
    }
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
    if (msg == g_taskbarCreatedMsg && g_taskbarCreatedMsg != 0) {
        g_tray.recreate();
        return 0;
    }

    switch (msg) {
        case WM_APP_TRAY:
            switch (LOWORD(lParam)) {
                case WM_RBUTTONUP:
                case WM_CONTEXTMENU: {
                    const TrayIcon::State state{
                        .altImeEnabled       = AppState::isEnabled(),
                        .capsRemapInstalled  = CapsRemapInstaller::isInstalled(),
                        .autostartEnabled    = Autostart::isEnabled(),
                    };
                    g_tray.showContextMenu(hwnd, state);
                    return 0;
                }
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_TOGGLE_ENABLED:
                    AppState::toggle();
                    g_detector.reset();
                    return 0;
                case IDM_TOGGLE_CAPS_REMAP:
                    if (CapsRemapInstaller::isInstalled()) {
                        CapsRemapInstaller::uninstallElevated();
                    } else {
                        CapsRemapInstaller::installElevated();
                    }
                    return 0;
                case IDM_TOGGLE_AUTOSTART:
                    handleToggleAutostart(hwnd);
                    return 0;
                case IDM_EXIT:
                    DestroyWindow(hwnd);
                    return 0;
            }
            return 0;

        case WM_APP_SOLO_ALT: {
            const HWND captured = reinterpret_cast<HWND>(lParam);
            if (captured == g_hwnd || captured != GetForegroundWindow()) {
                return 0;
            }
            const auto which = static_cast<AltSoloDetector::SoloKey>(wParam);
            if (which == AltSoloDetector::SoloKey::LeftAlt) {
                ImeController::setEnglish();
            } else {
                ImeController::setJapanese();
            }
            return 0;
        }

        case WM_DESTROY:
            KeyboardHook::uninstall();
            g_tray.destroy();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

[[nodiscard]] bool acquireSingleInstance() noexcept {
    g_instanceMutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (!g_instanceMutex) return false;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(g_instanceMutex);
        g_instanceMutex = nullptr;
        return false;
    }
    return true;
}

void releaseSingleInstance() noexcept {
    if (g_instanceMutex) {
        ReleaseMutex(g_instanceMutex);
        CloseHandle(g_instanceMutex);
        g_instanceMutex = nullptr;
    }
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    const int elevated = dispatchElevated();
    if (elevated >= 0) return elevated;

    if (!acquireSingleInstance()) {
        return 0;
    }

    AppState::load();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = kWindowClass;
    if (!RegisterClassW(&wc)) {
        releaseSingleInstance();
        return 1;
    }

    g_hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                             kWindowClass, L"",
                             WS_POPUP,
                             0, 0, 0, 0,
                             nullptr, nullptr, hInst, nullptr);
    if (!g_hwnd) {
        UnregisterClassW(kWindowClass, hInst);
        releaseSingleInstance();
        return 1;
    }

    g_taskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");

    if (!g_tray.create(g_hwnd, WM_APP_TRAY)) {
        DestroyWindow(g_hwnd);
        UnregisterClassW(kWindowClass, hInst);
        releaseSingleInstance();
        return 1;
    }

    g_detector.setCallback(onSoloAlt);

    if (!KeyboardHook::install(onKeyEvent)) {
        g_tray.destroy();
        DestroyWindow(g_hwnd);
        UnregisterClassW(kWindowClass, hInst);
        releaseSingleInstance();
        return 1;
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnregisterClassW(kWindowClass, hInst);
    releaseSingleInstance();
    return static_cast<int>(msg.wParam);
}
