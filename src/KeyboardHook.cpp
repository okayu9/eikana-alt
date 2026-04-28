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

#include "KeyboardHook.h"

namespace {

HHOOK                 g_hook = nullptr;
KeyboardHook::Handler g_handler = nullptr;

LRESULT CALLBACK proc(int nCode, WPARAM wParam, LPARAM lParam) noexcept {
    if (nCode == HC_ACTION && g_handler) [[likely]] {
        const auto* kbd = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        if (g_handler(wParam, *kbd)) {
            return 1;
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

} // namespace

bool KeyboardHook::install(Handler handler) noexcept {
    if (g_hook) return false;
    g_handler = handler;
    g_hook = SetWindowsHookExW(WH_KEYBOARD_LL, proc,
                               GetModuleHandleW(nullptr), 0);
    if (!g_hook) {
        g_handler = nullptr;
        return false;
    }
    return true;
}

void KeyboardHook::uninstall() noexcept {
    if (g_hook) {
        UnhookWindowsHookEx(g_hook);
        g_hook = nullptr;
    }
    g_handler = nullptr;
}
