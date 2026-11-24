#ifndef DALI_INTERNAL_ACCESSIBILITY_UIA_ACCESSIBILITY_H
#define DALI_INTERNAL_ACCESSIBILITY_UIA_ACCESSIBILITY_H

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
#include <cstdint>

// INTERNAL INCLUDES
#include <dali/public-api/dali-adaptor-common.h>

namespace DALI_NAMESPACE::Accessibility::Uia
{
/**
 * @brief Handles a WM_GETOBJECT message addressed to a DALi window.
 *
 * This is the entry point through which UI Automation clients (Narrator, NVDA, JAWS,
 * Inspect.exe) obtain the provider for a window. Only requests for UiaRootObjectId are
 * answered; every other object id is left to the default window procedure.
 *
 * Declared without any Win32 or UI Automation types so that the window system backend
 * does not have to include <uiautomation.h>.
 *
 * @param[in] window The HWND the message was sent to
 * @param[in] wParam The message wParam
 * @param[in] lParam The message lParam, carrying the requested object id
 * @param[out] result The value to return from the window procedure, valid when this
 * function returns true
 * @return True when the message was answered, false when the caller should fall through
 * to the default window procedure
 */
bool HandleGetObject(uintptr_t window, uintptr_t wParam, intptr_t lParam, intptr_t& result);

/**
 * @brief Executes one accessibility request that was marshalled to the event thread.
 *
 * UI Automation calls provider methods on arbitrary RPC threads, but the DALi scene graph
 * may only be read on the event thread, so the providers send themselves here through a
 * WIN_UIA_SYNC_EVENT window message.
 *
 * @param[in] token The request token carried in the message wParam
 */
void ExecuteSyncRequest(uintptr_t token);

} //namespace DALI_NAMESPACE::Accessibility::Uia

#endif // DALI_INTERNAL_ACCESSIBILITY_UIA_ACCESSIBILITY_H
