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
#include <dali/internal/accessibility/bridge/windows/uia-element-provider.h>

// EXTERNAL INCLUDES
#include <dali/public-api/actors/actor.h>
#include <algorithm>
#include <vector>

// INTERNAL INCLUDES
#include <dali/internal/accessibility/bridge/windows/uia-bridge.h>
#include <dali/internal/accessibility/bridge/windows/uia-mapping.h>

namespace DALI_NAMESPACE::Accessibility::Uia
{
namespace
{
using Dali::Devel::Accessibility::CoordinateType;
using Dali::Integration::Accessibility::Role;
using Dali::Integration::Accessibility::State;

/**
 * @brief Creates the fragment that represents @p accessible.
 *
 * @param[in] accessible The accessible to expose
 * @param[in] window The window that hosts the accessible
 * @return The fragment with a reference count of one, or nullptr
 */
IRawElementProviderFragment* MakeFragment(Accessible* accessible, HWND window)
{
  auto owning = ToOwningPtr(accessible);
  if(!owning)
  {
    return nullptr;
  }

  return static_cast<IRawElementProviderFragment*>(ElementProvider::New(owning, window));
}

/**
 * @brief Gets the identifier that distinguishes @p accessible from every other element.
 *
 * @param[in] accessible The accessible
 * @return The actor id, or 0 when the accessible has no actor
 */
int GetElementId(Accessible& accessible)
{
  Dali::Actor actor = accessible.GetInternalActor();

  return actor ? actor.GetProperty<int>(Dali::Actor::Property::ID) : 0;
}

/**
 * @brief Gets the extents of @p accessible in screen coordinates.
 *
 * Accessible::GetExtents(SCREEN) offsets by Window::GetPositionSize(), which on Windows
 * reports the origin of the window frame while the actors are laid out in the client
 * area, so the client origin is resolved here instead.
 *
 * @param[in] accessible The accessible
 * @param[in] window The window that hosts the accessible
 * @return The extents in screen coordinates
 */
Dali::Bounds GetScreenExtents(Accessible& accessible, HWND window)
{
  Dali::Bounds extents = accessible.GetExtents(CoordinateType::WINDOW);

  POINT origin{0, 0};
  if(ClientToScreen(window, &origin))
  {
    extents.x += static_cast<float>(origin.x);
    extents.y += static_cast<float>(origin.y);
  }

  return extents;
}

/**
 * @brief Checks whether a point lies inside @p extents.
 *
 * @param[in] extents The rectangle, in any coordinate system
 * @param[in] x The horizontal coordinate, in the same system
 * @param[in] y The vertical coordinate, in the same system
 * @return True when the point is inside
 */
bool Contains(const Dali::Bounds& extents, float x, float y)
{
  return x >= extents.x && x < extents.x + extents.width &&
         y >= extents.y && y < extents.y + extents.height;
}

/**
 * @brief Finds the deepest visible descendant of @p accessible under a point.
 *
 * Children are visited back to front, because a DALi actor is drawn on top of the
 * siblings that precede it.
 *
 * @param[in] accessible The accessible to search below
 * @param[in] x The horizontal coordinate, in window coordinates
 * @param[in] y The vertical coordinate, in window coordinates
 * @return The descendant under the point, or nullptr when no child covers it
 */
Accessible* HitTest(Accessible& accessible, float x, float y)
{
  std::vector<Accessible*> children = accessible.GetChildren();

  for(auto iter = children.rbegin(); iter != children.rend(); ++iter)
  {
    Accessible* child = *iter;
    if(!child || !child->GetStates()[State::SHOWING])
    {
      continue;
    }

    if(Contains(child->GetExtents(CoordinateType::WINDOW), x, y))
    {
      Accessible* deeper = HitTest(*child, x, y);
      return deeper ? deeper : child;
    }
  }

  return nullptr;
}

/**
 * @brief Gets the title of @p window.
 *
 * Used as the name of a fragment root whose accessible has none, so that a screen reader
 * still announces something when the window is activated.
 *
 * @param[in] window The window
 * @return The title in UTF-8, or an empty string
 */
std::string GetWindowTitle(HWND window)
{
  const int wideLength = GetWindowTextLengthW(window);
  if(wideLength <= 0)
  {
    return {};
  }

  std::wstring wide(static_cast<size_t>(wideLength) + 1u, 0);
  const int    copied = GetWindowTextW(window, &wide[0], wideLength + 1);
  if(copied <= 0)
  {
    return {};
  }

  const int required = WideCharToMultiByte(CP_UTF8, 0, wide.data(), copied, nullptr, 0, nullptr, nullptr);
  if(required <= 0)
  {
    return {};
  }

  std::string title(static_cast<size_t>(required), 0);
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), copied, &title[0], required, nullptr, nullptr);

  return title;
}

} // unnamed namespace

ElementProvider::ElementProvider(const Dali::SharedPtr<Accessible>& accessible, HWND window)
: mWindow(window),
  mAccessible(accessible)
{
}

ElementProvider* ElementProvider::New(const Dali::SharedPtr<Accessible>& accessible, HWND window)
{
  if(!accessible)
  {
    return nullptr;
  }

  return new ElementProvider(accessible, window);
}

Dali::SharedPtr<Accessible> ElementProvider::Lock() const
{
  return mAccessible.Lock();
}

bool ElementProvider::IsFragmentRoot() const
{
  return false;
}

HRESULT STDMETHODCALLTYPE ElementProvider::QueryInterface(REFIID iid, void** object)
{
  if(!object)
  {
    return E_POINTER;
  }

  *object = nullptr;
  if(iid == IID_IUnknown || iid == IID_IRawElementProviderSimple)
  {
    *object = static_cast<IRawElementProviderSimple*>(this);
  }
  else if(iid == IID_IRawElementProviderFragment)
  {
    *object = static_cast<IRawElementProviderFragment*>(this);
  }
  else
  {
    return E_NOINTERFACE;
  }

  AddRef();
  return S_OK;
}

ULONG STDMETHODCALLTYPE ElementProvider::AddRef()
{
  return InternalAddRef();
}

ULONG STDMETHODCALLTYPE ElementProvider::Release()
{
  return InternalRelease();
}

HRESULT STDMETHODCALLTYPE ElementProvider::get_ProviderOptions(ProviderOptions* options)
{
  if(!options)
  {
    return E_POINTER;
  }

  // The scene graph is read on the event thread, to which the providers marshal
  // themselves, so UI Automation must not be asked to do any threading on their behalf.
  *options = ProviderOptions_ServerSideProvider;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE ElementProvider::GetPatternProvider(PATTERNID patternId, IUnknown** provider)
{
  if(!provider)
  {
    return E_POINTER;
  }

  // Control patterns follow from the interfaces an Accessible implements (Action, Value,
  // Text and the rest), which this provider does not publish yet.
  *provider = nullptr;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE ElementProvider::GetPropertyValue(PROPERTYID propertyId, VARIANT* value)
{
  if(!value)
  {
    return E_POINTER;
  }

  VariantInit(value);

  return RunOnEventThread([this, propertyId, value]() -> HRESULT
  {
    auto accessible = Lock();
    if(!accessible)
    {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }

    const Role role = accessible->GetRole();

    switch(propertyId)
    {
      case UIA_NamePropertyId:
      {
        std::string name = accessible->GetName();
        if(name.empty() && IsFragmentRoot())
        {
          name = GetWindowTitle(mWindow);
        }
        return MakeVariantString(name, value);
      }

      case UIA_FullDescriptionPropertyId:
      case UIA_HelpTextPropertyId:
      {
        return MakeVariantString(accessible->GetDescription(), value);
      }

      case UIA_ControlTypePropertyId:
      {
        MakeVariantInt(static_cast<int>(GetControlType(role)), value);
        return S_OK;
      }

      case UIA_LocalizedControlTypePropertyId:
      {
        return MakeVariantString(GetLocalizedControlType(role), value);
      }

      case UIA_ClassNamePropertyId:
      {
        return MakeVariantString(accessible->GetRoleName(), value);
      }

      case UIA_AutomationIdPropertyId:
      {
        Dali::Actor actor = accessible->GetInternalActor();
        return MakeVariantString(actor ? actor.GetProperty<std::string>(Dali::Actor::Property::NAME) : std::string{}, value);
      }

      case UIA_ProviderDescriptionPropertyId:
      {
        return MakeVariantString("DALi", value);
      }

      case UIA_IsControlElementPropertyId:
      case UIA_IsContentElementPropertyId:
      {
        MakeVariantBool(true, value);
        return S_OK;
      }

      case UIA_IsPasswordPropertyId:
      {
        MakeVariantBool(role == Role::PASSWORD_TEXT, value);
        return S_OK;
      }

      case UIA_IsEnabledPropertyId:
      {
        auto states = accessible->GetStates();
        MakeVariantBool(states[State::ENABLED] && states[State::SENSITIVE], value);
        return S_OK;
      }

      case UIA_IsKeyboardFocusablePropertyId:
      {
        MakeVariantBool(accessible->GetStates()[State::FOCUSABLE], value);
        return S_OK;
      }

      case UIA_HasKeyboardFocusPropertyId:
      {
        MakeVariantBool(accessible->GetStates()[State::FOCUSED], value);
        return S_OK;
      }

      case UIA_IsOffscreenPropertyId:
      {
        MakeVariantBool(!accessible->GetStates()[State::SHOWING], value);
        return S_OK;
      }

      default:
      {
        // VT_EMPTY tells UI Automation that the property is not supported, so that it
        // falls back to its own default rather than reporting a blank value.
        return S_OK;
      }
    }
  });
}

HRESULT STDMETHODCALLTYPE ElementProvider::get_HostRawElementProvider(IRawElementProviderSimple** provider)
{
  if(!provider)
  {
    return E_POINTER;
  }

  *provider = nullptr;

  // Only the fragment root is backed by a window; the fragments below it are drawn by
  // DALi and have no window of their own.
  if(!IsFragmentRoot())
  {
    return S_OK;
  }

  return UiaHostProviderFromHwnd(mWindow, provider);
}

HRESULT STDMETHODCALLTYPE ElementProvider::Navigate(NavigateDirection direction, IRawElementProviderFragment** provider)
{
  if(!provider)
  {
    return E_POINTER;
  }

  *provider = nullptr;

  return RunOnEventThread([this, direction, provider]() -> HRESULT
  {
    auto accessible = Lock();
    if(!accessible)
    {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }

    Accessible* target = nullptr;

    switch(direction)
    {
      case NavigateDirection_Parent:
      {
        // Whatever sits above a fragment root is reached through the host provider.
        if(!IsFragmentRoot())
        {
          target = accessible->GetParent();
        }
        break;
      }

      case NavigateDirection_FirstChild:
      {
        std::vector<Accessible*> children = accessible->GetChildren();
        if(!children.empty())
        {
          target = children.front();
        }
        break;
      }

      case NavigateDirection_LastChild:
      {
        std::vector<Accessible*> children = accessible->GetChildren();
        if(!children.empty())
        {
          target = children.back();
        }
        break;
      }

      case NavigateDirection_NextSibling:
      case NavigateDirection_PreviousSibling:
      {
        Accessible* parent = IsFragmentRoot() ? nullptr : accessible->GetParent();
        if(!parent)
        {
          break;
        }

        std::vector<Accessible*> siblings = parent->GetChildren();
        auto                     self     = std::find(siblings.begin(), siblings.end(), accessible.Get());
        if(self == siblings.end())
        {
          break;
        }

        if(direction == NavigateDirection_NextSibling)
        {
          if(std::next(self) != siblings.end())
          {
            target = *std::next(self);
          }
        }
        else if(self != siblings.begin())
        {
          target = *std::prev(self);
        }
        break;
      }

      default:
      {
        break;
      }
    }

    if(target)
    {
      *provider = MakeFragment(target, mWindow);
    }

    return S_OK;
  });
}

HRESULT STDMETHODCALLTYPE ElementProvider::GetRuntimeId(SAFEARRAY** runtimeId)
{
  if(!runtimeId)
  {
    return E_POINTER;
  }

  *runtimeId = nullptr;

  // A fragment root is identified by its window, which UI Automation already knows.
  if(IsFragmentRoot())
  {
    return S_OK;
  }

  return RunOnEventThread([this, runtimeId]() -> HRESULT
  {
    auto accessible = Lock();
    if(!accessible)
    {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }

    // UiaAppendRuntimeId asks UI Automation to prepend the runtime id of the fragment
    // root, which makes the actor id unique across windows and processes.
    const int identifier[] = {UiaAppendRuntimeId, GetElementId(*accessible)};

    return MakeRuntimeId(identifier, 2, runtimeId);
  });
}

HRESULT STDMETHODCALLTYPE ElementProvider::get_BoundingRectangle(UiaRect* rectangle)
{
  if(!rectangle)
  {
    return E_POINTER;
  }

  *rectangle = UiaRect{0.0, 0.0, 0.0, 0.0};

  return RunOnEventThread([this, rectangle]() -> HRESULT
  {
    auto accessible = Lock();
    if(!accessible)
    {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }

    const Dali::Bounds extents = GetScreenExtents(*accessible, mWindow);

    rectangle->left   = static_cast<double>(extents.x);
    rectangle->top    = static_cast<double>(extents.y);
    rectangle->width  = static_cast<double>(extents.width);
    rectangle->height = static_cast<double>(extents.height);

    return S_OK;
  });
}

HRESULT STDMETHODCALLTYPE ElementProvider::GetEmbeddedFragmentRoots(SAFEARRAY** roots)
{
  if(!roots)
  {
    return E_POINTER;
  }

  // DALi does not host foreign fragment roots inside its scene.
  *roots = nullptr;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE ElementProvider::SetFocus()
{
  return RunOnEventThread([this]() -> HRESULT
  {
    auto accessible = Lock();
    if(!accessible)
    {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }

    accessible->GrabFocus();
    return S_OK;
  });
}

HRESULT STDMETHODCALLTYPE ElementProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** root)
{
  if(!root)
  {
    return E_POINTER;
  }

  *root = nullptr;

  if(IsFragmentRoot())
  {
    return QueryInterface(IID_IRawElementProviderFragmentRoot, reinterpret_cast<void**>(root));
  }

  HWND window = mWindow;

  return RunOnEventThread([window, root]() -> HRESULT
  {
    auto accessible = FindWindowAccessible(reinterpret_cast<uintptr_t>(window));
    if(!accessible)
    {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }

    *root = static_cast<IRawElementProviderFragmentRoot*>(RootProvider::New(accessible, window));
    return S_OK;
  });
}

RootProvider::RootProvider(const Dali::SharedPtr<Accessible>& accessible, HWND window)
: ElementProvider(accessible, window)
{
}

RootProvider* RootProvider::New(const Dali::SharedPtr<Accessible>& accessible, HWND window)
{
  if(!accessible)
  {
    return nullptr;
  }

  return new RootProvider(accessible, window);
}

bool RootProvider::IsFragmentRoot() const
{
  return true;
}

HRESULT STDMETHODCALLTYPE RootProvider::QueryInterface(REFIID iid, void** object)
{
  if(!object)
  {
    return E_POINTER;
  }

  if(iid == IID_IRawElementProviderFragmentRoot)
  {
    *object = static_cast<IRawElementProviderFragmentRoot*>(this);
    AddRef();
    return S_OK;
  }

  return ElementProvider::QueryInterface(iid, object);
}

ULONG STDMETHODCALLTYPE RootProvider::AddRef()
{
  return InternalAddRef();
}

ULONG STDMETHODCALLTYPE RootProvider::Release()
{
  return InternalRelease();
}

HRESULT STDMETHODCALLTYPE RootProvider::ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** provider)
{
  if(!provider)
  {
    return E_POINTER;
  }

  *provider = nullptr;

  return RunOnEventThread([this, x, y, provider]() -> HRESULT
  {
    auto accessible = Lock();
    if(!accessible)
    {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }

    POINT origin{0, 0};
    if(!ClientToScreen(mWindow, &origin))
    {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }

    const float windowX = static_cast<float>(x) - static_cast<float>(origin.x);
    const float windowY = static_cast<float>(y) - static_cast<float>(origin.y);

    if(Accessible* target = HitTest(*accessible, windowX, windowY))
    {
      *provider = MakeFragment(target, mWindow);
    }

    // An empty reply means the point belongs to the fragment root itself.
    return S_OK;
  });
}

HRESULT STDMETHODCALLTYPE RootProvider::GetFocus(IRawElementProviderFragment** provider)
{
  if(!provider)
  {
    return E_POINTER;
  }

  // Focus is reported once the bridge tracks the highlighted and key input focused actor.
  *provider = nullptr;
  return S_OK;
}

} //namespace DALI_NAMESPACE::Accessibility::Uia
