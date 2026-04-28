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

// Thin wrapper around a global WH_KEYBOARD_LL hook.
// The handler is invoked re-entrantly from any nested message pump
// running on the installing thread (typically the main message loop).
namespace KeyboardHook {

// Returns true to swallow the event (block both the next hook and the target).
using Handler = bool (*)(WPARAM, const KBDLLHOOKSTRUCT&) noexcept;

[[nodiscard]] bool install(Handler handler) noexcept;
void uninstall() noexcept;

} // namespace KeyboardHook
