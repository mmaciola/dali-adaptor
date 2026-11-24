#ifndef DALI_INTERNAL_ACCESSIBILITY_UIA_ELEMENT_PROVIDER_H
#define DALI_INTERNAL_ACCESSIBILITY_UIA_ELEMENT_PROVIDER_H

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

// INTERNAL INCLUDES
#include <dali/devel-api/atspi-interfaces/accessible.h>
#include <dali/internal/accessibility/bridge/windows/uia-common.h>

namespace DALI_NAMESPACE::Accessibility::Uia
{
/**
 * @brief Exposes one Accessible to UI Automation.
 *
 * DALi renders its whole scene into a single window, so every Accessible is published as
 * a fragment of that window rather than as a window of its own. A provider holds a weak
 * reference to its Accessible: a UI Automation client may keep the COM object alive long
 * after the actor is gone, and such a stale provider answers UIA_E_ELEMENTNOTAVAILABLE.
 *
 * @note Providers are handed to out-of-process clients, which call them on RPC threads.
 * Every method that reads the scene graph therefore runs its work through
 * RunOnEventThread().
 */
class ElementProvider : public ComObject,
                        public IRawElementProviderSimple,
                        public IRawElementProviderFragment
{
public:
  /**
   * @brief Creates a provider for @p accessible.
   *
   * @param[in] accessible The accessible to expose
   * @param[in] window The window that hosts the accessible
   * @return The provider with a reference count of one, or nullptr when @p accessible is
   * empty. The caller owns the returned reference.
   */
  static ElementProvider* New(const Dali::SharedPtr<Accessible>& accessible, HWND window);

  // IUnknown

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override;
  ULONG STDMETHODCALLTYPE   AddRef() override;
  ULONG STDMETHODCALLTYPE   Release() override;

  // IRawElementProviderSimple

  HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* options) override;
  HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID patternId, IUnknown** provider) override;
  HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID propertyId, VARIANT* value) override;
  HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** provider) override;

  // IRawElementProviderFragment

  HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, IRawElementProviderFragment** provider) override;
  HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** runtimeId) override;
  HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* rectangle) override;
  HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** roots) override;
  HRESULT STDMETHODCALLTYPE SetFocus() override;
  HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** root) override;

protected:
  ElementProvider(const Dali::SharedPtr<Accessible>& accessible, HWND window);
  ~ElementProvider() override = default;

  /**
   * @brief Resolves the accessible this provider was created for.
   *
   * @return The accessible, or an empty pointer when it has been destroyed
   * @note Must be called on the event thread.
   */
  Dali::SharedPtr<Accessible> Lock() const;

  /**
   * @brief Checks whether this provider is the fragment root of its window.
   *
   * The root answers Navigate(Parent) with nullptr and carries the host provider, so that
   * UI Automation joins the DALi fragment tree to the window tree of the desktop.
   *
   * @return True for a fragment root
   */
  virtual bool IsFragmentRoot() const;

  /** The window that hosts the accessible. */
  HWND mWindow;

private:
  Dali::WeakPtr<Accessible> mAccessible;
};

/**
 * @brief Exposes the top level accessible of a window as a UI Automation fragment root.
 *
 * A fragment root is the element a WM_GETOBJECT reply points at; it owns hit testing and
 * focus reporting for everything below it.
 */
class RootProvider final : public ElementProvider,
                           public IRawElementProviderFragmentRoot
{
public:
  /**
   * @brief Creates the fragment root of @p window.
   *
   * @param[in] accessible The top level accessible of the window
   * @param[in] window The window
   * @return The provider with a reference count of one. The caller owns the reference.
   */
  static RootProvider* New(const Dali::SharedPtr<Accessible>& accessible, HWND window);

  // IUnknown

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override;
  ULONG STDMETHODCALLTYPE   AddRef() override;
  ULONG STDMETHODCALLTYPE   Release() override;

  // IRawElementProviderFragmentRoot

  HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** provider) override;
  HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment** provider) override;

private:
  RootProvider(const Dali::SharedPtr<Accessible>& accessible, HWND window);
  ~RootProvider() override = default;

  bool IsFragmentRoot() const override;
};

} //namespace DALI_NAMESPACE::Accessibility::Uia

#endif // DALI_INTERNAL_ACCESSIBILITY_UIA_ELEMENT_PROVIDER_H
