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

// CLASS HEADER
#include <dali/internal/accessibility/bridge/windows/uia-common.h>

// EXTERNAL INCLUDES
#include <atomic>

// INTERNAL INCLUDES
#include <dali/integration-api/debug.h>

namespace DALI_NAMESPACE::Accessibility::Uia
{
namespace
{
/**
 * @brief How long a marshalled request waits for the event thread.
 *
 * A screen reader keeps querying the provider while the application renders, so a request
 * that cannot be served promptly is better abandoned than left blocking an RPC thread.
 */
constexpr UINT SYNC_REQUEST_TIMEOUT = 2000u;

/**
 * @brief One request marshalled from an RPC thread to the event thread.
 *
 * Lives on the stack of the calling thread for the duration of the SendMessageTimeout()
 * call, which is synchronous, so no ownership is transferred.
 */
struct SyncRequest
{
  const std::function<HRESULT()>* function{nullptr};
  HRESULT                         result{UIA_E_ELEMENTNOTAVAILABLE};
};

std::atomic<HWND> gProviderWindow{nullptr};

} // unnamed namespace

ULONG ComObject::InternalAddRef()
{
  return static_cast<ULONG>(InterlockedIncrement(&mReferenceCount));
}

ULONG ComObject::InternalRelease()
{
  const ULONG referenceCount = static_cast<ULONG>(InterlockedDecrement(&mReferenceCount));
  if(referenceCount == 0u)
  {
    delete this;
  }
  return referenceCount;
}

BSTR MakeBstr(const std::string& utf8)
{
  if(utf8.empty())
  {
    return nullptr;
  }

  const int inputLength = static_cast<int>(utf8.size());
  const int wideLength  = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), inputLength, nullptr, 0);
  if(wideLength <= 0)
  {
    return nullptr;
  }

  BSTR result = SysAllocStringLen(nullptr, static_cast<UINT>(wideLength));
  if(!result)
  {
    return nullptr;
  }

  if(MultiByteToWideChar(CP_UTF8, 0, utf8.data(), inputLength, result, wideLength) != wideLength)
  {
    SysFreeString(result);
    return nullptr;
  }

  return result;
}

HRESULT MakeVariantString(const std::string& utf8, VARIANT* value)
{
  if(utf8.empty())
  {
    value->vt = VT_EMPTY;
    return S_OK;
  }

  BSTR text = MakeBstr(utf8);
  if(!text)
  {
    value->vt = VT_EMPTY;
    return E_OUTOFMEMORY;
  }

  value->vt      = VT_BSTR;
  value->bstrVal = text;
  return S_OK;
}

void MakeVariantBool(bool boolean, VARIANT* value)
{
  value->vt      = VT_BOOL;
  value->boolVal = boolean ? VARIANT_TRUE : VARIANT_FALSE;
}

void MakeVariantInt(int integer, VARIANT* value)
{
  value->vt   = VT_I4;
  value->lVal = integer;
}

HRESULT MakeRuntimeId(const int* values, int count, SAFEARRAY** result)
{
  *result = nullptr;

  SAFEARRAY* array = SafeArrayCreateVector(VT_I4, 0, static_cast<ULONG>(count));
  if(!array)
  {
    return E_OUTOFMEMORY;
  }

  for(LONG index = 0; index < count; ++index)
  {
    int value = values[index];
    if(FAILED(SafeArrayPutElement(array, &index, &value)))
    {
      SafeArrayDestroy(array);
      return E_OUTOFMEMORY;
    }
  }

  *result = array;
  return S_OK;
}

void SetProviderWindow(HWND window)
{
  gProviderWindow.store(window, std::memory_order_relaxed);
}

HWND GetProviderWindow()
{
  return gProviderWindow.load(std::memory_order_relaxed);
}

HRESULT RunOnEventThread(const std::function<HRESULT()>& function)
{
  HWND window = GetProviderWindow();
  if(!window || !IsWindow(window))
  {
    return UIA_E_ELEMENTNOTAVAILABLE;
  }

  if(GetWindowThreadProcessId(window, nullptr) == GetCurrentThreadId())
  {
    return function();
  }

  SyncRequest request;
  request.function = &function;

  DWORD_PTR messageResult = 0u;
  if(SendMessageTimeout(window,
                        WIN_UIA_SYNC_EVENT,
                        reinterpret_cast<WPARAM>(&request),
                        0,
                        SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT,
                        SYNC_REQUEST_TIMEOUT,
                        &messageResult) == 0)
  {
    DALI_LOG_DEBUG_INFO("UIA request was not served: the event thread did not answer\n");
    return UIA_E_ELEMENTNOTAVAILABLE;
  }

  return request.result;
}

void ExecuteSyncRequest(uintptr_t token)
{
  auto* request = reinterpret_cast<SyncRequest*>(token);
  if(request && request->function)
  {
    request->result = (*request->function)();
  }
}

} //namespace DALI_NAMESPACE::Accessibility::Uia
