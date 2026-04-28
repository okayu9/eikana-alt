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

#include "ImeController.h"

#include <windows.h>

#include <array>

namespace {

void sendKeyTap(WORD vk) noexcept {
    std::array<INPUT, 2> inputs = {{
        {.type = INPUT_KEYBOARD, .ki = {.wVk = vk}},
        {.type = INPUT_KEYBOARD, .ki = {.wVk = vk, .dwFlags = KEYEVENTF_KEYUP}},
    }};
    SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}

} // namespace

void ImeController::setEnglish() noexcept {
    sendKeyTap(VK_IME_OFF);
}

void ImeController::setJapanese() noexcept {
    sendKeyTap(VK_IME_ON);
}
