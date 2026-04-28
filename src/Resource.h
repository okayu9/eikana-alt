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

inline constexpr UINT IDM_TOGGLE_ENABLED    = 1001;
inline constexpr UINT IDM_EXIT              = 1002;
inline constexpr UINT IDM_TOGGLE_CAPS_REMAP = 1003;
inline constexpr UINT IDM_TOGGLE_AUTOSTART  = 1004;

inline constexpr UINT WM_APP_TRAY     = WM_APP + 1;
inline constexpr UINT WM_APP_SOLO_ALT = WM_APP + 2;
