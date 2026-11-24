#ifndef DALI_INTERNAL_ACCESSIBILITY_UIA_BRIDGE_H
#define DALI_INTERNAL_ACCESSIBILITY_UIA_BRIDGE_H

/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// EXTERNAL INCLUDES
#include <dali/public-api/common/shared-ptr.h>
#include <cstdint>

// INTERNAL INCLUDES
#include <dali/devel-api/atspi-interfaces/accessible.h>
#include <dali/internal/accessibility/bridge/windows/uia-accessibility.h>

namespace DALI_NAMESPACE::Accessibility::Uia
{
/**
 * @brief Finds the top level accessible that belongs to a window.
 *
 * The bridge keeps the accessibles registered by Window::OnAccessibilityEnabled(), each of
 * which is the root layer of one DALi window. They are matched against @p window through
 * the native handle of the window the actor lives in.
 *
 * @param[in] window The HWND to look up
 * @return The top level accessible of that window, or an empty pointer when the window has
 * not registered one (yet)
 *
 * @note Must be called on the event thread.
 */
Dali::SharedPtr<Accessible> FindWindowAccessible(uintptr_t window);

/**
 * @brief Recovers an owning pointer to an accessible the tree handed out as a raw pointer.
 *
 * The accessibility tree is traversed through raw pointers, but a provider has to keep a
 * weak reference to survive the actor it describes, which requires shared ownership.
 *
 * @param[in] accessible The accessible to look up
 * @return The owning pointer, or an empty pointer for an accessible that is not part of an
 * actor tree (and therefore cannot be published to UI Automation)
 */
Dali::SharedPtr<Accessible> ToOwningPtr(Accessible* accessible);

} //namespace DALI_NAMESPACE::Accessibility::Uia

#endif // DALI_INTERNAL_ACCESSIBILITY_UIA_BRIDGE_H
