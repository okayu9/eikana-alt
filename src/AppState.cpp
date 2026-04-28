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

#include "AppState.h"

#include <windows.h>

#include <atomic>

namespace {

constinit std::atomic<bool> g_enabled{true};

constexpr wchar_t kSubKey[]     = L"Software\\eikana-alt";
constexpr wchar_t kValEnabled[] = L"Enabled";

void persist() noexcept {
    DWORD value = g_enabled.load(std::memory_order_relaxed) ? 1u : 0u;
    RegSetKeyValueW(HKEY_CURRENT_USER, kSubKey, kValEnabled,
                    REG_DWORD, &value, sizeof(value));
}

} // namespace

bool AppState::isEnabled() noexcept {
    return g_enabled.load(std::memory_order_relaxed);
}

void AppState::setEnabled(bool value) noexcept {
    g_enabled.store(value, std::memory_order_relaxed);
    persist();
}

bool AppState::toggle() noexcept {
    const bool now = !g_enabled.load(std::memory_order_relaxed);
    g_enabled.store(now, std::memory_order_relaxed);
    persist();
    return now;
}

void AppState::load() noexcept {
    DWORD value = 0;
    DWORD size = sizeof(value);
    const LSTATUS s = RegGetValueW(HKEY_CURRENT_USER, kSubKey, kValEnabled,
                                   RRF_RT_REG_DWORD, nullptr, &value, &size);
    if (s == ERROR_SUCCESS) {
        g_enabled.store(value != 0, std::memory_order_relaxed);
    } else {
        persist();
    }
}
