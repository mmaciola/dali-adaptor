#ifndef DALI_INTERNAL_ACCESSIBILITY_UIA_MAPPING_H
#define DALI_INTERNAL_ACCESSIBILITY_UIA_MAPPING_H

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
#include <string>

// INTERNAL INCLUDES
#include <dali/integration-api/adaptor-framework/accessibility/accessibility-integ.h>

namespace DALI_NAMESPACE::Accessibility::Uia
{
/**
 * @brief Converts a DALi accessibility role to a UI Automation control type.
 *
 * DALi models roles with the AT-SPI vocabulary, which is finer grained than the UI
 * Automation control types, so several roles share one control type. Roles without any
 * reasonable equivalent become UIA_CustomControlTypeId and rely on
 * GetLocalizedControlType() to stay meaningful to a screen reader.
 *
 * @param[in] role The DALi role
 * @return The matching UIA_*ControlTypeId
 */
long GetControlType(Dali::Integration::Accessibility::Role role);

/**
 * @brief Gets the spoken control type name for roles that UI Automation cannot name.
 *
 * UI Automation derives the localized control type from the control type itself, which is
 * wrong for a role mapped onto a generic control type - a screen reader would announce
 * "custom" or "group" instead of what the element is.
 *
 * @param[in] role The DALi role
 * @return The name to announce, or an empty string to keep the UI Automation default
 *
 * @note The returned text is not translated. @see Accessible::GetLocalizedRoleName()
 */
std::string GetLocalizedControlType(Dali::Integration::Accessibility::Role role);

/**
 * @brief Checks whether a role describes text that the user can edit.
 *
 * @param[in] role The DALi role
 * @return True for the editable text roles
 */
bool IsEditableTextRole(Dali::Integration::Accessibility::Role role);

} //namespace DALI_NAMESPACE::Accessibility::Uia

#endif // DALI_INTERNAL_ACCESSIBILITY_UIA_MAPPING_H
