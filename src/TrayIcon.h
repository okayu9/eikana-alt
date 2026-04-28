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

#pragma once

#include <windows.h>
#include <shellapi.h>

class TrayIcon {
public:
    struct State {
        bool altImeEnabled;
        bool capsRemapInstalled;
        bool autostartEnabled;
    };

    [[nodiscard]] bool create(HWND owner, UINT callbackMessage) noexcept;
    void destroy() noexcept;
    void recreate() noexcept;
    void showContextMenu(HWND owner, const State& state) noexcept;

private:
    NOTIFYICONDATAW nid_{};
    bool created_ = false;
};
