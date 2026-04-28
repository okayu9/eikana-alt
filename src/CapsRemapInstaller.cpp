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

#include "CapsRemapInstaller.h"

#include "ModulePath.h"

#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>

#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr wchar_t kKey[]   = L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout";
constexpr wchar_t kValue[] = L"Scancode Map";

constexpr WORD kOurTarget = 0x001D; // Left Ctrl
constexpr WORD kOurSource = 0x003A; // CapsLock

using Mapping = std::pair<WORD, WORD>; // (target_scancode, source_scancode)

// Reads the entire Scancode Map and returns the list of mappings.
// On a missing or unreadable value the list is empty (caller treats as
// "no remappings present"). Returns false on a hard error reading.
bool readMap(std::vector<Mapping>& out) noexcept {
    out.clear();

    DWORD size = 0;
    DWORD type = 0;
    LSTATUS s = RegGetValueW(HKEY_LOCAL_MACHINE, kKey, kValue,
                             RRF_RT_REG_BINARY, &type, nullptr, &size);
    if (s == ERROR_FILE_NOT_FOUND) return true;
    if (s != ERROR_SUCCESS && s != ERROR_MORE_DATA) return false;
    if (size < 16) return true; // malformed; treat as empty

    std::vector<BYTE> buf(size);
    s = RegGetValueW(HKEY_LOCAL_MACHINE, kKey, kValue,
                     RRF_RT_REG_BINARY, &type, buf.data(), &size);
    if (s != ERROR_SUCCESS) return false;

    DWORD count = 0;
    std::memcpy(&count, buf.data() + 8, sizeof(count));
    if (count < 1 || count > (size - 12) / 4 + 1) return true; // malformed

    for (DWORD i = 0; i + 1 < count; ++i) {
        const DWORD offset = 12 + i * 4;
        if (offset + 4 > size) break;
        WORD target = 0;
        WORD source = 0;
        std::memcpy(&target, buf.data() + offset,     sizeof(target));
        std::memcpy(&source, buf.data() + offset + 2, sizeof(source));
        out.emplace_back(target, source);
    }
    return true;
}

// Writes the given mappings as the Scancode Map value. If empty, the value
// is deleted entirely.
[[nodiscard]] LSTATUS writeMap(const std::vector<Mapping>& mappings) noexcept {
    if (mappings.empty()) {
        const LSTATUS s = RegDeleteKeyValueW(HKEY_LOCAL_MACHINE, kKey, kValue);
        return (s == ERROR_FILE_NOT_FOUND) ? ERROR_SUCCESS : s;
    }

    const DWORD count = static_cast<DWORD>(mappings.size()) + 1; // +1 terminator
    const DWORD totalSize = 12 + count * 4;
    std::vector<BYTE> buf(totalSize, 0);

    std::memcpy(buf.data() + 8, &count, sizeof(count));
    for (size_t i = 0; i < mappings.size(); ++i) {
        const DWORD offset = 12 + static_cast<DWORD>(i) * 4;
        std::memcpy(buf.data() + offset,     &mappings[i].first,  sizeof(WORD));
        std::memcpy(buf.data() + offset + 2, &mappings[i].second, sizeof(WORD));
    }
    // Terminator at the end stays zero.

    return RegSetKeyValueW(HKEY_LOCAL_MACHINE, kKey, kValue,
                           REG_BINARY, buf.data(), totalSize);
}

bool isOurMapping(const Mapping& m) noexcept {
    return m.first == kOurTarget && m.second == kOurSource;
}

void spawnElevatedSelf(const wchar_t* args) noexcept {
    const std::wstring exePath = ModulePath::get();
    if (exePath.empty()) return;

    SHELLEXECUTEINFOW info = {};
    info.cbSize = sizeof(info);
    info.lpVerb = L"runas";
    info.lpFile = exePath.c_str();
    info.lpParameters = args;
    info.nShow = SW_HIDE;
    ShellExecuteExW(&info);
}

void showInfo(const wchar_t* text) noexcept {
    MessageBoxW(nullptr, text, L"eikana-alt", MB_OK | MB_ICONINFORMATION);
}

void showError(const wchar_t* action, LSTATUS status) noexcept {
    wchar_t msg[256];
    StringCchPrintfW(msg, ARRAYSIZE(msg),
                     L"CapsLock リマップの%sに失敗しました (エラー %ld)。\n"
                     L"管理者権限が必要です。",
                     action, status);
    MessageBoxW(nullptr, msg, L"eikana-alt", MB_OK | MB_ICONERROR);
}

} // namespace

bool CapsRemapInstaller::isInstalled() noexcept {
    std::vector<Mapping> mappings;
    if (!readMap(mappings)) return false;
    for (const auto& m : mappings) {
        if (isOurMapping(m)) return true;
    }
    return false;
}

void CapsRemapInstaller::installElevated() noexcept {
    spawnElevatedSelf(L"--install-caps-remap");
}

void CapsRemapInstaller::uninstallElevated() noexcept {
    spawnElevatedSelf(L"--uninstall-caps-remap");
}

int CapsRemapInstaller::installAsElevated() noexcept {
    std::vector<Mapping> mappings;
    if (!readMap(mappings)) {
        showError(L"設定", ERROR_INVALID_DATA);
        return 1;
    }

    bool already = false;
    for (const auto& m : mappings) {
        if (isOurMapping(m)) { already = true; break; }
    }
    if (!already) {
        mappings.emplace_back(kOurTarget, kOurSource);
    }

    const LSTATUS s = writeMap(mappings);
    if (s != ERROR_SUCCESS) {
        showError(L"設定", s);
        return 1;
    }
    showInfo(L"CapsLock を Ctrl にリマップしました。\n\n"
             L"変更を反映するには Windows を再起動 "
             L"(またはサインアウト→サインイン) してください。");
    return 0;
}

int CapsRemapInstaller::uninstallAsElevated() noexcept {
    std::vector<Mapping> mappings;
    if (!readMap(mappings)) {
        showError(L"解除", ERROR_INVALID_DATA);
        return 1;
    }

    std::vector<Mapping> kept;
    kept.reserve(mappings.size());
    for (const auto& m : mappings) {
        if (!isOurMapping(m)) kept.push_back(m);
    }

    const LSTATUS s = writeMap(kept);
    if (s != ERROR_SUCCESS) {
        showError(L"解除", s);
        return 1;
    }
    showInfo(L"CapsLock のリマップを解除しました。\n\n"
             L"変更を反映するには Windows を再起動 "
             L"(またはサインアウト→サインイン) してください。");
    return 0;
}
