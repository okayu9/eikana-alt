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

#include "AltSoloDetector.h"

bool AltSoloDetector::onKeyEvent(WPARAM wParam, const KBDLLHOOKSTRUCT& kbd) noexcept {
    const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
    const bool isUp   = (wParam == WM_KEYUP   || wParam == WM_SYSKEYUP);
    const DWORD vk    = kbd.vkCode;

    if (isDown) {
        if (vk == VK_LMENU) {
            if (!lAlt_.down) {
                lAlt_.down = true;
                lAlt_.combined = rAlt_.down;
                if (rAlt_.down) {
                    rAlt_.combined = true;
                }
            }
        } else if (vk == VK_RMENU) {
            if (!rAlt_.down) {
                rAlt_.down = true;
                rAlt_.combined = lAlt_.down;
                if (lAlt_.down) {
                    lAlt_.combined = true;
                }
            }
        } else {
            if (lAlt_.down) lAlt_.combined = true;
            if (rAlt_.down) rAlt_.combined = true;
        }
        return false;
    }

    if (isUp) {
        if (vk == VK_LMENU) {
            const bool wasSolo = lAlt_.down && !lAlt_.combined;
            lAlt_.down = false;
            lAlt_.combined = false;
            if (wasSolo && callback_) {
                callback_(SoloKey::LeftAlt);
                return true;
            }
            return false;
        }
        if (vk == VK_RMENU) {
            const bool wasSolo = rAlt_.down && !rAlt_.combined;
            rAlt_.down = false;
            rAlt_.combined = false;
            if (wasSolo && callback_) {
                callback_(SoloKey::RightAlt);
                return true;
            }
            return false;
        }
    }

    return false;
}

void AltSoloDetector::reset() noexcept {
    lAlt_ = {};
    rAlt_ = {};
}
