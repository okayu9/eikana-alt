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

#include <string>

namespace ModulePath {

// Returns the full path of the running executable. Handles paths longer
// than MAX_PATH by retrying with a doubled buffer. Empty string on failure.
[[nodiscard]] inline std::wstring get() noexcept {
    std::wstring buf(MAX_PATH, L'\0');
    for (;;) {
        const DWORD len = GetModuleFileNameW(nullptr, buf.data(),
                                             static_cast<DWORD>(buf.size()));
        if (len == 0) return {};
        if (len < buf.size()) {
            buf.resize(len);
            return buf;
        }
        buf.resize(buf.size() * 2);
    }
}

} // namespace ModulePath
