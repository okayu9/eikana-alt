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

namespace CapsRemapInstaller {

// True if the CapsLock -> Left Ctrl mapping is currently present in the
// Scancode Map (other unrelated mappings may also be present). HKLM read
// does not require admin.
[[nodiscard]] bool isInstalled() noexcept;

// Spawn this executable elevated (UAC prompt) to add or remove our
// mapping while preserving any existing user mappings. Returns immediately.
void installElevated() noexcept;
void uninstallElevated() noexcept;

// Entry points for the elevated child instance, dispatched by main.cpp
// when it sees the matching command-line flag.
[[nodiscard]] int installAsElevated() noexcept;
[[nodiscard]] int uninstallAsElevated() noexcept;

} // namespace CapsRemapInstaller
