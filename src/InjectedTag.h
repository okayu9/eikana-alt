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

#include <basetsd.h>

// Marker placed in dwExtraInfo of synthetic keyboard events that this
// process generates, so our own low-level hook can identify and ignore
// them instead of feeding them back through the Alt-detection state.
namespace Injected {

inline constexpr ULONG_PTR Tag = 0xE1CA1A;

} // namespace Injected
