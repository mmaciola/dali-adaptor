#ifndef DALI_INTERNAL_ACCESSIBILITY_UIA_COMMON_H
#define DALI_INTERNAL_ACCESSIBILITY_UIA_COMMON_H

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
#include <windows.h>
// WIN32_LEAN_AND_MEAN keeps windows.h from pulling in COM, which uiautomation.h needs for
// the "interface" keyword, IUnknown, BSTR and SAFEARRAY.
#include <objbase.h>
#include <oleauto.h>
#include <uiautomation.h>
#include <functional>
#include <string>

// INTERNAL INCLUDES
#include <dali/internal/accessibility/bridge/windows/uia-accessibility.h>

namespace DALI_NAMESPACE::Accessibility::Uia
{
/**
 * @brief Reference counted base class for the UI Automation provider objects.
 *
 * The providers are COM objects handed out to out-of-process clients, so their lifetime
 * is controlled by the client and the final Release() may well arrive on an RPC thread.
 */
class ComObject
{
public:
  /**
   * @brief Increments the reference count.
   * @return The new reference count
   */
  ULONG InternalAddRef();

  /**
   * @brief Decrements the reference count and destroys the object when it drops to zero.
   * @return The new reference count
   */
  ULONG InternalRelease();

protected:
  ComObject()          = default;
  virtual ~ComObject() = default;

  ComObject(const ComObject&)            = delete;
  ComObject(ComObject&&)                 = delete;
  ComObject& operator=(const ComObject&) = delete;
  ComObject& operator=(ComObject&&)      = delete;

private:
  LONG mReferenceCount{1};
};

/**
 * @brief Converts a UTF-8 string to a newly allocated BSTR.
 *
 * @param[in] utf8 The string to convert
 * @return The BSTR, which the caller owns, or nullptr when @p utf8 is empty or malformed
 */
BSTR MakeBstr(const std::string& utf8);

/**
 * @brief Stores a UTF-8 string in a VARIANT as VT_BSTR.
 *
 * An empty string yields VT_EMPTY, which tells UI Automation that the property is not
 * supported rather than reporting it as present but blank.
 *
 * @param[in] utf8 The string to store
 * @param[out] value The variant to fill in
 * @return S_OK, or E_OUTOFMEMORY when the string could not be allocated
 */
HRESULT MakeVariantString(const std::string& utf8, VARIANT* value);

/**
 * @brief Stores a boolean in a VARIANT as VT_BOOL.
 *
 * @param[in] boolean The value to store
 * @param[out] value The variant to fill in
 */
void MakeVariantBool(bool boolean, VARIANT* value);

/**
 * @brief Stores an integer in a VARIANT as VT_I4.
 *
 * @param[in] integer The value to store
 * @param[out] value The variant to fill in
 */
void MakeVariantInt(int integer, VARIANT* value);

/**
 * @brief Builds the SAFEARRAY of VT_I4 that GetRuntimeId() has to return.
 *
 * @param[in] values The runtime id elements
 * @param[in] count The number of elements
 * @param[out] result The resulting safe array, which the caller owns
 * @return S_OK on success, E_OUTOFMEMORY otherwise
 */
HRESULT MakeRuntimeId(const int* values, int count, SAFEARRAY** result);

/**
 * @brief Remembers the window that serves accessibility requests.
 *
 * The window identifies the event thread: requests arriving on an RPC thread are
 * marshalled to it, because the DALi scene graph may only be read on the event thread.
 *
 * @param[in] window The window that answered WM_GETOBJECT
 */
void SetProviderWindow(HWND window);

/**
 * @brief Gets the window that serves accessibility requests.
 * @return The window, or nullptr before the first WM_GETOBJECT was answered
 */
HWND GetProviderWindow();

/**
 * @brief Runs @p function on the DALi event thread and returns its result.
 *
 * When the caller already runs on the event thread, @p function is invoked directly.
 *
 * @param[in] function The work to run
 * @return The value returned by @p function, or UIA_E_ELEMENTNOTAVAILABLE when the event
 * thread could not be reached
 */
HRESULT RunOnEventThread(const std::function<HRESULT()>& function);

} //namespace DALI_NAMESPACE::Accessibility::Uia

#endif // DALI_INTERNAL_ACCESSIBILITY_UIA_COMMON_H
