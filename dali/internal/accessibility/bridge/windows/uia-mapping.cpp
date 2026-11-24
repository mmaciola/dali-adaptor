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
#include <dali/internal/accessibility/bridge/windows/uia-mapping.h>

// EXTERNAL INCLUDES
#include <windows.h>
// @see uia-common.h for why COM has to be pulled in by hand here.
#include <objbase.h>
#include <uiautomation.h>

namespace DALI_NAMESPACE::Accessibility::Uia
{
using Dali::Integration::Accessibility::Role;

long GetControlType(Role role)
{
  switch(role)
  {
    // Text
    case Role::ACCELERATOR_LABEL:
    case Role::CAPTION:
    case Role::DEFINITION:
    case Role::DESCRIPTION_TERM:
    case Role::DESCRIPTION_VALUE:
    case Role::FOOTNOTE:
    case Role::HEADING:
    case Role::LABEL:
    case Role::MATH:
    case Role::MATH_FRACTION:
    case Role::MATH_ROOT:
    case Role::PARAGRAPH:
    case Role::STATIC:
    case Role::SUBSCRIPT:
    case Role::SUPERSCRIPT:
      return UIA_TextControlTypeId;

    // Editable text
    case Role::AUTOCOMPLETE:
    case Role::DATE_EDITOR:
    case Role::EDITBAR:
    case Role::ENTRY:
    case Role::PASSWORD_TEXT:
    case Role::TERMINAL:
    case Role::TEXT:
      return UIA_EditControlTypeId;

    // Buttons
    case Role::PUSH_BUTTON:
    case Role::SWITCH:
    case Role::TOGGLE_BUTTON:
      return UIA_ButtonControlTypeId;

    case Role::PUSH_BUTTON_MENU:
      return UIA_SplitButtonControlTypeId;

    case Role::CHECK_BOX:
      return UIA_CheckBoxControlTypeId;

    case Role::RADIO_BUTTON:
      return UIA_RadioButtonControlTypeId;

    // Menus
    case Role::MENU:
    case Role::POPUP_MENU:
      return UIA_MenuControlTypeId;

    case Role::MENU_BAR:
      return UIA_MenuBarControlTypeId;

    case Role::CHECK_MENU_ITEM:
    case Role::MENU_ITEM:
    case Role::RADIO_MENU_ITEM:
    case Role::TEAROFF_MENU_ITEM:
      return UIA_MenuItemControlTypeId;

    // Lists, tables and trees
    case Role::DESCRIPTION_LIST:
    case Role::LIST:
    case Role::LIST_BOX:
      return UIA_ListControlTypeId;

    case Role::LIST_ITEM:
      return UIA_ListItemControlTypeId;

    case Role::TABLE:
      return UIA_TableControlTypeId;

    case Role::TABLE_CELL:
    case Role::TABLE_ROW:
      return UIA_DataItemControlTypeId;

    case Role::COLUMN_HEADER:
    case Role::ROW_HEADER:
    case Role::TABLE_COLUMN_HEADER:
    case Role::TABLE_ROW_HEADER:
      return UIA_HeaderItemControlTypeId;

    case Role::TREE:
    case Role::TREE_TABLE:
      return UIA_TreeControlTypeId;

    case Role::TREE_ITEM:
      return UIA_TreeItemControlTypeId;

    case Role::COMBO_BOX:
      return UIA_ComboBoxControlTypeId;

    // Tabs
    case Role::PAGE_TAB:
      return UIA_TabItemControlTypeId;

    case Role::PAGE_TAB_LIST:
      return UIA_TabControlTypeId;

    // Values
    case Role::DIAL:
    case Role::RATING:
    case Role::SLIDER:
      return UIA_SliderControlTypeId;

    case Role::SPIN_BUTTON:
      return UIA_SpinnerControlTypeId;

    case Role::LEVEL_BAR:
    case Role::PROGRESS_BAR:
      return UIA_ProgressBarControlTypeId;

    case Role::SCROLL_BAR:
      return UIA_ScrollBarControlTypeId;

    // Images
    case Role::ANIMATION:
    case Role::ARROW:
    case Role::CANVAS:
    case Role::CHART:
    case Role::DESKTOP_ICON:
    case Role::ICON:
    case Role::IMAGE:
    case Role::IMAGE_MAP:
      return UIA_ImageControlTypeId;

    // Windows and dialogs
    case Role::APPLICATION:
    case Role::COLOR_CHOOSER:
    case Role::DIALOG:
    case Role::FILE_CHOOSER:
    case Role::FONT_CHOOSER:
    case Role::FRAME:
    case Role::INPUT_METHOD_WINDOW:
    case Role::INTERNAL_FRAME:
    case Role::WINDOW:
      return UIA_WindowControlTypeId;

    case Role::TITLE_BAR:
      return UIA_TitleBarControlTypeId;

    // Containers that hold other controls rather than content
    case Role::DESKTOP_FRAME:
    case Role::DIRECTORY_PANE:
    case Role::EMBEDDED:
    case Role::FILLER:
    case Role::GLASS_PANE:
    case Role::LAYERED_PANE:
    case Role::OPTION_PANE:
    case Role::PANEL:
    case Role::ROOT_PANE:
    case Role::SCROLL_PANE:
    case Role::SPLIT_PANE:
    case Role::VIEWPORT:
      return UIA_PaneControlTypeId;

    // Groupings that carry meaning for a screen reader
    case Role::ALERT:
    case Role::ARTICLE:
    case Role::AUDIO:
    case Role::BLOCK_QUOTE:
    case Role::COMMENT:
    case Role::CONTENT_DELETION:
    case Role::CONTENT_INSERTION:
    case Role::FOOTER:
    case Role::FORM:
    case Role::FOCUS_TRAVERSABLE:
    case Role::GROUPING:
    case Role::HEADER:
    case Role::INFO_BAR:
    case Role::LANDMARK:
    case Role::LOG:
    case Role::MARK:
    case Role::MARQUEE:
    case Role::NOTIFICATION:
    case Role::PAGE:
    case Role::SECTION:
    case Role::SUGGESTION:
    case Role::TIMER:
    case Role::VIDEO:
      return UIA_GroupControlTypeId;

    // Documents
    case Role::DOCUMENT_EMAIL:
    case Role::DOCUMENT_FRAME:
    case Role::DOCUMENT_PRESENTATION:
    case Role::DOCUMENT_SPREADSHEET:
    case Role::DOCUMENT_TEXT:
    case Role::DOCUMENT_WEB:
    case Role::HTML_CONTAINER:
      return UIA_DocumentControlTypeId;

    case Role::LINK:
      return UIA_HyperlinkControlTypeId;

    case Role::SEPARATOR:
    case Role::RULER:
      return UIA_SeparatorControlTypeId;

    case Role::STATUS_BAR:
      return UIA_StatusBarControlTypeId;

    case Role::TOOL_BAR:
      return UIA_ToolBarControlTypeId;

    case Role::TOOL_TIP:
      return UIA_ToolTipControlTypeId;

    case Role::CALENDAR:
      return UIA_CalendarControlTypeId;

    default:
      // DRAWING_AREA, EXTENDED, INVALID, REDUNDANT_OBJECT, UNKNOWN and anything added to
      // the role vocabulary after this mapping was written.
      return UIA_CustomControlTypeId;
  }
}

std::string GetLocalizedControlType(Role role)
{
  // Only roles whose control type does not already name them need an override: UI
  // Automation announces the control type on its own for everything else.
  switch(role)
  {
    case Role::ALERT:
      return "alert";
    case Role::ARTICLE:
      return "article";
    case Role::AUDIO:
      return "audio";
    case Role::BLOCK_QUOTE:
      return "block quote";
    case Role::COMMENT:
      return "comment";
    case Role::DRAWING_AREA:
      return "drawing area";
    case Role::FOOTER:
      return "footer";
    case Role::FORM:
      return "form";
    case Role::HEADER:
      return "header";
    case Role::HEADING:
      return "heading";
    case Role::INFO_BAR:
      return "info bar";
    case Role::LANDMARK:
      return "landmark";
    case Role::LOG:
      return "log";
    case Role::MARQUEE:
      return "marquee";
    case Role::NOTIFICATION:
      return "notification";
    case Role::PARAGRAPH:
      return "paragraph";
    case Role::SECTION:
      return "section";
    case Role::SUGGESTION:
      return "suggestion";
    case Role::SWITCH:
      return "switch";
    case Role::TIMER:
      return "timer";
    case Role::TOGGLE_BUTTON:
      return "toggle button";
    case Role::VIDEO:
      return "video";
    default:
      return {};
  }
}

bool IsEditableTextRole(Role role)
{
  switch(role)
  {
    case Role::AUTOCOMPLETE:
    case Role::DATE_EDITOR:
    case Role::EDITBAR:
    case Role::ENTRY:
    case Role::PASSWORD_TEXT:
    case Role::TERMINAL:
    case Role::TEXT:
      return true;
    default:
      return false;
  }
}

} //namespace DALI_NAMESPACE::Accessibility::Uia
