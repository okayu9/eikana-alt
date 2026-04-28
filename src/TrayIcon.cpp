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

#include "TrayIcon.h"

#include "Resource.h"

#include <strsafe.h>

namespace {

constexpr UINT kTrayUid = 1;
constexpr wchar_t kTip[] = L"eikana-alt  (左Alt = 英数 / 右Alt = かな)";

} // namespace

bool TrayIcon::create(HWND owner, UINT callbackMessage) noexcept {
    if (created_) return true;

    nid_ = {};
    nid_.cbSize = sizeof(nid_);
    nid_.hWnd = owner;
    nid_.uID = kTrayUid;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = callbackMessage;
    nid_.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    StringCchCopyW(nid_.szTip, ARRAYSIZE(nid_.szTip), kTip);

    if (!Shell_NotifyIconW(NIM_ADD, &nid_)) {
        return false;
    }
    nid_.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid_);
    created_ = true;
    return true;
}

void TrayIcon::destroy() noexcept {
    if (created_) {
        Shell_NotifyIconW(NIM_DELETE, &nid_);
        created_ = false;
    }
}

void TrayIcon::recreate() noexcept {
    if (!created_) return;
    Shell_NotifyIconW(NIM_ADD, &nid_);
    Shell_NotifyIconW(NIM_SETVERSION, &nid_);
}

void TrayIcon::showContextMenu(HWND owner, const State& state) noexcept {
    POINT pt{};
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    auto check = [](bool b) -> UINT {
        return MF_STRING | (b ? MF_CHECKED : MF_UNCHECKED);
    };

    AppendMenuW(menu, check(state.altImeEnabled),       IDM_TOGGLE_ENABLED,
                L"Alt キーで IME 切替");
    AppendMenuW(menu, check(state.capsRemapInstalled),  IDM_TOGGLE_CAPS_REMAP,
                L"CapsLock を Ctrl にリマップ  (要再起動)");
    AppendMenuW(menu, check(state.autostartEnabled),    IDM_TOGGLE_AUTOSTART,
                L"Windows 起動時に自動起動");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_EXIT, L"終了");

    SetForegroundWindow(owner);

    TrackPopupMenu(menu,
                   TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN,
                   pt.x, pt.y, 0, owner, nullptr);

    PostMessageW(owner, WM_NULL, 0, 0);
    DestroyMenu(menu);
}
