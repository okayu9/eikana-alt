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

// Pure state machine that detects "solo" left/right Alt presses.
// A solo press is an Alt key-down followed by a key-up of the same Alt
// with no other key (including the other Alt) pressed in between.
class AltSoloDetector {
public:
    enum class SoloKey { LeftAlt, RightAlt };
    using Callback = void (*)(SoloKey) noexcept;

    void setCallback(Callback cb) noexcept { callback_ = cb; }

    // Process a low-level keyboard event.
    // Returns true if the event should be swallowed (the caller is
    // responsible for any required key injection after swallowing).
    // The callback fires when a solo Alt up is recognized; the matching
    // Alt up event is reported as "swallow = true".
    [[nodiscard]] bool onKeyEvent(WPARAM wParam, const KBDLLHOOKSTRUCT& kbd) noexcept;

    // Clear all internal flags. Call when re-enabling the hook to avoid
    // acting on key state captured before disable.
    void reset() noexcept;

private:
    struct AltState {
        bool down = false;
        bool combined = false;
    };

    AltState lAlt_;
    AltState rAlt_;
    Callback callback_ = nullptr;
};
