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

#include "Autostart.h"

#include "ModulePath.h"

#include <windows.h>

#include <string>

namespace {

constexpr wchar_t kKey[]   = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kValue[] = L"eikana-alt";

} // namespace

bool Autostart::isEnabled() noexcept {
    DWORD size = 0;
    return RegGetValueW(HKEY_CURRENT_USER, kKey, kValue,
                        RRF_RT_REG_SZ, nullptr, nullptr, &size) == ERROR_SUCCESS;
}

bool Autostart::setEnabled(bool enable) noexcept {
    if (!enable) {
        const LSTATUS s = RegDeleteKeyValueW(HKEY_CURRENT_USER, kKey, kValue);
        return s == ERROR_SUCCESS || s == ERROR_FILE_NOT_FOUND;
    }

    const std::wstring exePath = ModulePath::get();
    if (exePath.empty()) return false;

    std::wstring quoted;
    quoted.reserve(exePath.size() + 2);
    quoted.push_back(L'"');
    quoted.append(exePath);
    quoted.push_back(L'"');

    const DWORD bytes = static_cast<DWORD>((quoted.size() + 1) * sizeof(wchar_t));
    return RegSetKeyValueW(HKEY_CURRENT_USER, kKey, kValue,
                           REG_SZ, quoted.c_str(), bytes) == ERROR_SUCCESS;
}
