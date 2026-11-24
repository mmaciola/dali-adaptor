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

// INTERNAL INCLUDES
#include <dali/integration-api/adaptor-framework/accessibility/accessibility-bridge.h>
#include <dali/internal/accessibility/bridge/dummy/dummy-atspi.h>

// ============================================================================
// Bridge stubs
//
// Installed on a platform that has no accessibility backend at all. A platform with a
// backend of its own - such as the UI Automation bridge on Windows - keeps the rest of
// dummy-atspi.cpp but defines these itself, so that Bridge::GetCurrentBridge() hands out
// its bridge instead of the dummy one.
// ============================================================================

Dali::SharedPtr<Dali::Integration::Accessibility::Bridge> Dali::Integration::Accessibility::Bridge::GetCurrentBridge()
{
  return Dali::Accessibility::DummyBridge::GetInstance();
}

void Dali::Integration::Accessibility::Bridge::DisableAutoInit()
{
}

void Dali::Integration::Accessibility::Bridge::EnableAutoInit()
{
}

std::string Dali::Integration::Accessibility::Bridge::MakeBusNameForWidget(std::string_view widgetInstanceId, int widgetProcessId)
{
  return std::string{widgetInstanceId};
}
