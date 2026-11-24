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
#include <dali/internal/accessibility/bridge/windows/uia-bridge.h>

// EXTERNAL INCLUDES
#include <dali/public-api/actors/actor.h>
#include <dali/public-api/adaptor-framework/window.h>
#include <dali/public-api/object/any.h>
#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

// INTERNAL INCLUDES
#include <dali/devel-api/adaptor-framework/actor-accessible.h>
#include <dali/devel-api/adaptor-framework/environment-variable.h>
#include <dali/integration-api/debug.h>
#include <dali/internal/accessibility/bridge/dummy/dummy-atspi.h>
#include <dali/internal/accessibility/bridge/windows/uia-common.h>
#include <dali/internal/accessibility/bridge/windows/uia-element-provider.h>
#include <dali/internal/system/common/environment-variables.h>
#include <dali/internal/window-system/windows/platform-implement-win.h>

using Dali::Integration::Accessibility::Bridge;

namespace DALI_NAMESPACE::Accessibility::Uia
{
namespace
{
/**
 * @brief Publishes the DALi accessibility tree through UI Automation.
 *
 * Unlike the AT-SPI bridge, which has to register itself on a bus and mirror the tree over
 * D-Bus, a UI Automation provider is pulled: the client asks the window for a provider
 * (WM_GETOBJECT) and walks the tree from there. The bridge therefore only has to own the
 * accessible objects, know which accessible is the root of which window, and - from the
 * event support stage onwards - raise the events that tell a client to query again.
 */
class BridgeUia : public Bridge
{
public:
  BridgeUia()           = default;
  ~BridgeUia() override = default;

  /**
   * @brief Brings the bridge up when a UI Automation client appears.
   *
   * Called while answering WM_GETOBJECT, which is how a client announces itself. Bringing
   * the bridge up emits the enabled signal, upon which every window registers its root
   * layer through AddTopLevelWindow(), so the tree is ready by the time this returns.
   */
  void EnsureUp()
  {
    if(!mIsApplicationRunning || mIsScreenReaderSuppressed)
    {
      return;
    }

    ForceUp();
  }

  /**
   * @brief Gets the accessibles registered as the root of a window.
   * @return The top level accessibles, in registration order
   */
  const std::vector<Accessible*>& GetTopLevelWindows() const
  {
    return mTopLevelWindows;
  }

  // Accessible registry

  bool AddAccessible(uint32_t actorId, Dali::SharedPtr<Accessible> accessible) override
  {
    mAccessibles[actorId] = std::move(accessible);
    return true;
  }

  void RemoveAccessible(uint32_t actorId) override
  {
    mAccessibles.erase(actorId);
  }

  Dali::SharedPtr<Accessible> GetAccessible(Dali::Actor actor) const override
  {
    if(!actor)
    {
      return {};
    }

    const uint32_t actorId = static_cast<uint32_t>(actor.GetProperty<int>(Dali::Actor::Property::ID));
    const auto     iter    = mAccessibles.find(actorId);

    return iter != mAccessibles.end() ? iter->second : Dali::SharedPtr<Accessible>();
  }

  Dali::SharedPtr<Accessible> GetAccessible(const std::string& path) const override
  {
    char*          end     = nullptr;
    const uint32_t actorId = static_cast<uint32_t>(std::strtoul(path.c_str(), &end, 10));
    if(end == path.c_str() || (end && *end != 0))
    {
      return {};
    }

    const auto iter = mAccessibles.find(actorId);

    return iter != mAccessibles.end() ? iter->second : Dali::SharedPtr<Accessible>();
  }

  bool ShouldIncludeHidden() const override
  {
    // UI Automation has no equivalent of the AT-SPI "include hidden" query: an element
    // that is not shown is reported through the IsOffscreen property instead.
    return false;
  }

  // Tree roots

  void AddTopLevelWindow(Accessible* object) override
  {
    if(!object || std::find(mTopLevelWindows.begin(), mTopLevelWindows.end(), object) != mTopLevelWindows.end())
    {
      return;
    }

    mTopLevelWindows.push_back(object);
    SetIsOnRootLevel(object);
  }

  void RemoveTopLevelWindow(Accessible* object) override
  {
    mTopLevelWindows.erase(std::remove(mTopLevelWindows.begin(), mTopLevelWindows.end(), object), mTopLevelWindows.end());
  }

  Accessible* GetApplication() const override
  {
    // UI Automation models the application through the window host provider, so the tree
    // has no application node of its own.
    return nullptr;
  }

  Accessible* FindByPath(const std::string& path) const override
  {
    auto accessible = GetAccessible(path);
    return accessible ? accessible.Get() : nullptr;
  }

  void SetApplicationName(std::string name) override
  {
    mApplicationName = std::move(name);
  }

  void SetToolkitName(std::string_view toolkitName) override
  {
    mToolkitName = std::string{toolkitName};
  }

  // Activation

  void Initialize() override
  {
    // The bridge is brought up by the first WM_GETOBJECT rather than here: a UI Automation
    // provider is only ever asked for by a client, and answering that message is also the
    // point at which the window serving accessibility requests becomes known.
    mIsApplicationRunning = true;
  }

  void Terminate() override
  {
    ForceDown();
    mIsApplicationRunning = false;
  }

  ForceUpResult ForceUp() override
  {
    const ForceUpResult result = Bridge::ForceUp();
    if(result == ForceUpResult::ALREADY_UP)
    {
      return result;
    }

    mIsEnabled = true;
    DALI_LOG_RELEASE_INFO("UI Automation bridge is up\n");

    // Every window registers its root layer in response to this.
    mEnabledSignal.Emit();

    return ForceUpResult::JUST_STARTED;
  }

  void ForceDown() override
  {
    if(!mData)
    {
      return;
    }

    mIsEnabled = false;
    mDisabledSignal.Emit();

    mTopLevelWindows.clear();
    mAccessibles.clear();
    mData = {};
  }

  void ApplicationPaused() override
  {
    mIsApplicationRunning = false;
  }

  void ApplicationResumed() override
  {
    mIsApplicationRunning = true;
  }

  void SuppressScreenReader(bool suppress) override
  {
    if(mIsScreenReaderSuppressed == suppress)
    {
      return;
    }

    mIsScreenReaderSuppressed = suppress;
    if(suppress)
    {
      ForceDown();
    }
  }

  bool GetScreenReaderEnabled() override
  {
    // UI Automation does not distinguish a screen reader from any other client, so the
    // answer is whether anything at all is listening.
    return !mIsScreenReaderSuppressed && UiaClientsAreListening() != FALSE;
  }

  bool IsEnabled() override
  {
    return mIsEnabled;
  }

  const std::string& GetBusName() const override
  {
    // A bus name is an AT-SPI concept; UI Automation addresses a provider by its window.
    static const std::string busName{};
    return busName;
  }

  // Window notifications
  //
  // UI Automation learns about these through the window itself, so nothing has to be
  // forwarded until the bridge raises automation events.

  void WindowCreated(Dali::Window window) override
  {
  }

  void WindowShown(Dali::Window window) override
  {
  }

  void WindowHidden(Dali::Window window) override
  {
  }

  void WindowFocused(Dali::Window window) override
  {
  }

  void WindowUnfocused(Dali::Window window) override
  {
  }

  void WindowMinimized(Dali::Window window) override
  {
  }

  void WindowRestored(Dali::Window window, Dali::Devel::Accessibility::WindowRestoreType detail) override
  {
  }

  void WindowMaximized(Dali::Window window) override
  {
  }

  // Events
  //
  // A UI Automation client re-queries a property when it is told that the property
  // changed, so these turn into UiaRaiseAutomationEvent() calls once event support lands.

  void EmitCursorMoved(Accessible* obj, unsigned int cursorPosition) override
  {
  }

  void EmitActiveDescendantChanged(Accessible* obj, Accessible* child) override
  {
  }

  void EmitTextChanged(Accessible* obj, Dali::Devel::Accessibility::TextChangedState state, unsigned int position, unsigned int length, const std::string& content) override
  {
  }

  void EmitMovedOutOfScreen(Accessible* obj, Dali::Devel::Accessibility::ScreenRelativeMoveType type) override
  {
  }

  void EmitScrollStarted(Accessible* obj) override
  {
  }

  void EmitScrollFinished(Accessible* obj) override
  {
  }

  void EmitStateChanged(Dali::SharedPtr<Accessible> obj, Dali::Integration::Accessibility::State state, int newValue, int reserved) override
  {
  }

  void Emit(Accessible* obj, Dali::Devel::Accessibility::WindowEvent event, unsigned int detail) override
  {
  }

  void Emit(Dali::SharedPtr<Accessible> obj, Dali::Devel::Accessibility::ObjectPropertyChangeEvent event) override
  {
  }

  void EmitBoundsChanged(Dali::SharedPtr<Accessible> obj, Dali::BoundsInteger rect) override
  {
  }

  void EmitPostRender(Dali::SharedPtr<Accessible> obj) override
  {
  }

  bool EmitKeyEvent(Dali::KeyEvent keyEvent, std::function<void(Dali::KeyEvent, bool)> callback) override
  {
    // A UI Automation client cannot consume a key event on the application's behalf, so
    // every key stays with the application.
    return false;
  }

  // Direct reading
  //
  // Mapped onto UiaRaiseNotificationEvent() once event support lands; a UI Automation
  // client owns its speech queue, so pausing and stopping it is not offered to providers.

  void Say(const std::string& text, bool discardable, std::function<void(std::string)> callback) override
  {
  }

  void Pause() override
  {
  }

  void Resume() override
  {
  }

  void StopReading(bool alsoNonDiscardable) override
  {
  }

  // Tizen widget embedding
  //
  // Sockets and bus names only exist to let one AT-SPI application graft its tree into
  // another; a UI Automation client crosses a process boundary through the window tree.

  void RegisterDefaultLabel(Dali::Actor actor) override
  {
  }

  void UnregisterDefaultLabel(Dali::Actor actor) override
  {
  }

  Accessible* GetDefaultLabel(Accessible* root) override
  {
    return nullptr;
  }

  Dali::Devel::Accessibility::Address EmbedSocket(const Dali::Devel::Accessibility::Address& plug, const Dali::Devel::Accessibility::Address& socket) override
  {
    return {};
  }

  void UnembedSocket(const Dali::Devel::Accessibility::Address& plug, const Dali::Devel::Accessibility::Address& socket) override
  {
  }

  void SetSocketOffset(ProxyAccessible* socket, std::int32_t x, std::int32_t y) override
  {
  }

  void SetExtentsOffset(std::int32_t x, std::int32_t y) override
  {
  }

  void SetPreferredBusName(std::string_view preferredBusName) override
  {
  }

private:
  std::map<uint32_t, Dali::SharedPtr<Accessible>> mAccessibles;      ///< Actor id to accessible
  std::vector<Accessible*>                        mTopLevelWindows;  ///< Root layer of each window
  std::string                                     mApplicationName;
  std::string                                     mToolkitName{"dali"};
  bool                                            mIsEnabled{false};
  bool                                            mIsApplicationRunning{false};
  bool                                            mIsScreenReaderSuppressed{false};
};

/**
 * @brief Gets the bridge as a UI Automation bridge.
 *
 * @return The bridge, or nullptr when accessibility is disabled and the dummy bridge is
 * installed instead
 */
BridgeUia* GetBridgeUia()
{
  auto bridge = Bridge::GetCurrentBridge();

  return bridge ? dynamic_cast<BridgeUia*>(bridge.Get()) : nullptr;
}

/**
 * @brief Gets the native handle of the window @p actor belongs to.
 *
 * @param[in] actor The actor
 * @return The HWND as an integer, or 0 when the actor is not on a window
 */
uintptr_t GetNativeWindowHandle(Dali::Actor actor)
{
  if(!actor)
  {
    return 0u;
  }

  Dali::Window window = Dali::Window::Get(actor);
  if(!window)
  {
    return 0u;
  }

  Dali::Any handle = window.GetNativeHandle();

  return handle.IsType<WinWindowHandle>() ? static_cast<uintptr_t>(Dali::AnyCast<WinWindowHandle>(handle)) : 0u;
}

bool gInitializedBridge = false;

/**
 * @brief Creates the bridge instance.
 *
 * @return The UI Automation bridge, or the dummy bridge when accessibility was disabled
 * through the environment
 */
Dali::SharedPtr<Bridge> CreateBridge()
{
  gInitializedBridge = true;

  const char* disabled = Dali::EnvironmentVariable::GetEnvironmentVariable(DALI_ENV_DISABLE_ATSPI);
  if(disabled && std::atoi(disabled) != 0)
  {
    DALI_LOG_DEBUG_INFO("Accessibility disabled by environment. Return dummy instance\n");
    return Dali::Accessibility::DummyBridge::GetInstance();
  }

  return Dali::MakeShared<BridgeUia>();
}

} // unnamed namespace

Dali::SharedPtr<Accessible> ToOwningPtr(Accessible* accessible)
{
  if(auto* actorAccessible = dynamic_cast<ActorAccessible*>(accessible))
  {
    return actorAccessible->SharedFromThis();
  }

  return {};
}

Dali::SharedPtr<Accessible> FindWindowAccessible(uintptr_t window)
{
  BridgeUia* bridge = GetBridgeUia();
  if(!bridge || window == 0u)
  {
    return {};
  }

  for(Accessible* candidate : bridge->GetTopLevelWindows())
  {
    if(candidate && GetNativeWindowHandle(candidate->GetInternalActor()) == window)
    {
      return ToOwningPtr(candidate);
    }
  }

  return {};
}

bool HandleGetObject(uintptr_t window, uintptr_t wParam, intptr_t lParam, intptr_t& result)
{
  if(static_cast<long>(lParam) != static_cast<long>(UiaRootObjectId))
  {
    // Clients that ask for a client area or a menu object are served by the window itself.
    return false;
  }

  BridgeUia* bridge = GetBridgeUia();
  if(!bridge)
  {
    return false;
  }

  HWND windowHandle = reinterpret_cast<HWND>(window);

  // Remember the window before bringing the bridge up: the accessibles registered in
  // response are read through requests marshalled to this window's thread.
  SetProviderWindow(windowHandle);
  bridge->EnsureUp();

  auto accessible = FindWindowAccessible(window);
  if(!accessible)
  {
    DALI_LOG_DEBUG_INFO("No accessible is registered for the window that was asked for a provider\n");
    return false;
  }

  RootProvider* root = RootProvider::New(accessible, windowHandle);
  if(!root)
  {
    return false;
  }

  result = static_cast<intptr_t>(UiaReturnRawElementProvider(windowHandle, static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam), static_cast<IRawElementProviderSimple*>(root)));
  root->Release();

  return true;
}

} //namespace DALI_NAMESPACE::Accessibility::Uia

// Dali::Integration::Accessibility::Bridge class implementation

Dali::SharedPtr<Bridge> Bridge::GetCurrentBridge()
{
  static Dali::SharedPtr<Bridge> bridge;

  // Guard the rare case of a call after the bridge destructor ran: a static SharedPtr is
  // not cleared when the static destructors run.
  if(DALI_UNLIKELY(Bridge::IsTerminated()))
  {
    return Dali::SharedPtr<Bridge>();
  }

  if(bridge)
  {
    return bridge;
  }

  if(mAutoInitState == AutoInitState::ENABLED)
  {
    bridge = Dali::Accessibility::Uia::CreateBridge();

    const char* suppressScreenReader = Dali::EnvironmentVariable::GetEnvironmentVariable(DALI_ENV_SUPPRESS_SCREEN_READER);
    if(suppressScreenReader && std::atoi(suppressScreenReader) != 0)
    {
      bridge->SuppressScreenReader(true);
    }

    return bridge;
  }

  return Dali::Accessibility::DummyBridge::GetInstance();
}

void Bridge::DisableAutoInit()
{
  if(Dali::Accessibility::Uia::gInitializedBridge)
  {
    DALI_LOG_ERROR("Bridge::DisableAutoInit() called after bridge auto-initialization");
  }

  mAutoInitState = AutoInitState::DISABLED;
}

void Bridge::EnableAutoInit()
{
  mAutoInitState = AutoInitState::ENABLED;

  if(Dali::Accessibility::Uia::gInitializedBridge)
  {
    return;
  }

  auto bridge = Bridge::GetCurrentBridge();
  if(DALI_LIKELY(bridge))
  {
    bridge->Initialize();
  }
}

std::string Bridge::MakeBusNameForWidget(std::string_view widgetInstanceId, int widgetProcessId)
{
  // Widgets are embedded over AT-SPI only, so there is no name to make up here.
  return std::string{widgetInstanceId};
}
