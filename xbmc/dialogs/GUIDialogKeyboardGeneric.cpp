/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "interfaces/AnnouncementManager.h"
#include "input/XBMC_vkeys.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIWindowManager.h"
#include "input/KeyboardLayoutManager.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogOK.h"
#include "GUIDialogKeyboardGeneric.h"
#include "settings/Settings.h"
#include "utils/RegExp.h"
#include "ApplicationMessenger.h"
#include "windowing/WindowingFactory.h"

#define BUTTON_ID_OFFSET      100
#define BUTTONS_PER_ROW        20
#define BUTTONS_MAX_ROWS        4

#define CTL_BUTTON_DONE       300
#define CTL_BUTTON_CANCEL     301
#define CTL_BUTTON_SHIFT      302
#define CTL_BUTTON_CAPS       303
#define CTL_BUTTON_SYMBOLS    304
#define CTL_BUTTON_LEFT       305
#define CTL_BUTTON_RIGHT      306
#define CTL_BUTTON_IP_ADDRESS 307
#define CTL_BUTTON_CLEAR      308
#define CTL_BUTTON_LAYOUT     309

#define CTL_LABEL_HEADING     311
#define CTL_EDIT              312

#define CTL_BUTTON_BACKSPACE    8
#define CTL_BUTTON_SPACE       32

#define SEARCH_DELAY         1000

CGUIDialogKeyboardGeneric::CGUIDialogKeyboardGeneric()
: CGUIDialog(WINDOW_DIALOG_KEYBOARD, "DialogKeyboard.xml")
, CGUIKeyboard()
, m_pCharCallback(NULL)
{
  m_bIsConfirmed = false;
  m_bShift = false;
  m_hiddenInput = false;
  m_keyType = LOWER;
  m_currentLayout = 0;
  m_loadType = KEEP_IN_MEMORY;
  m_isKeyboardNavigationMode = false;
  m_previouslyFocusedButton = 0;
}

void CGUIDialogKeyboardGeneric::OnWindowLoaded()
{
  g_Windowing.EnableTextInput(false);
  CGUIDialog::OnWindowLoaded();
}

void CGUIDialogKeyboardGeneric::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  m_bIsConfirmed = false;
  m_isKeyboardNavigationMode = false;

  // fill in the keyboard layouts
  m_currentLayout = 0;
  m_layouts.clear();
  const KeyboardLayouts& keyboardLayouts = CKeyboardLayoutManager::Get().GetLayouts();
  std::vector<CVariant> layoutNames = CSettings::Get().GetList("locale.keyboardlayouts");

  for (std::vector<CVariant>::const_iterator layoutName = layoutNames.begin(); layoutName != layoutNames.end(); ++layoutName)
  {
    KeyboardLayouts::const_iterator keyboardLayout = keyboardLayouts.find(layoutName->asString());
    if (keyboardLayout != keyboardLayouts.end())
      m_layouts.push_back(keyboardLayout->second);
  }

  // set alphabetic (capitals)
  UpdateButtons();

  // set heading
  if (!m_strHeading.empty())
  {
    SET_CONTROL_LABEL(CTL_LABEL_HEADING, m_strHeading);
    SET_CONTROL_VISIBLE(CTL_LABEL_HEADING);
  }
  else
  {
    SET_CONTROL_HIDDEN(CTL_LABEL_HEADING);
  }
  // set type
  {
    CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CTL_EDIT, m_hiddenInput ? CGUIEditControl::INPUT_TYPE_PASSWORD : CGUIEditControl::INPUT_TYPE_TEXT);
    OnMessage(msg);
  }
  SetEditText(m_text);

  CVariant data;
  data["title"] = m_strHeading;
  data["type"] = !m_hiddenInput ? "keyboard" : "password";
  data["value"] = GetText();
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Input, "xbmc", "OnInputRequested", data);
}

bool CGUIDialogKeyboardGeneric::OnAction(const CAction &action)
{
  bool handled = true;
  if (action.GetID() == ACTION_ENTER || (m_isKeyboardNavigationMode && action.GetID() == ACTION_SELECT_ITEM))
    OnOK();
  else if (action.GetID() == ACTION_SHIFT)
    OnShift();
  else if (action.GetID() == ACTION_SYMBOLS)
    OnSymbols();
  // don't handle move left/right and select in the edit control
  else if (!m_isKeyboardNavigationMode &&
           (action.GetID() == ACTION_MOVE_LEFT ||
           action.GetID() == ACTION_MOVE_RIGHT ||
           action.GetID() == ACTION_SELECT_ITEM))
    handled = false;
  else
  {
    handled = false;
    // send action to edit control
    CGUIControl *edit = GetControl(CTL_EDIT);
    if (edit)
      handled = edit->OnAction(action);
    if (!handled && action.GetID() >= KEY_VKEY && action.GetID() < KEY_ASCII)
    {
      BYTE b = action.GetID() & 0xFF;
      if (b == XBMCVK_TAB)
      {
        // Toggle left/right key mode
        m_isKeyboardNavigationMode = !m_isKeyboardNavigationMode;
        if (m_isKeyboardNavigationMode)
        {
          m_previouslyFocusedButton = GetFocusedControlID();
          SET_CONTROL_FOCUS(edit->GetID(), 0);
        }
        else
          SET_CONTROL_FOCUS(m_previouslyFocusedButton, 0);
        handled = true;
      }
    }
  }

  if (!handled) // unhandled by us - let's see if the baseclass wants it
    handled = CGUIDialog::OnAction(action);

  return handled;
}

bool CGUIDialogKeyboardGeneric::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      switch (iControl)
      {
      case CTL_BUTTON_DONE:
        OnOK();
        break;
      case CTL_BUTTON_CANCEL:
        Close();
        break;
      case CTL_BUTTON_SHIFT:
        OnShift();
        break;
      case CTL_BUTTON_CAPS:
        if (m_keyType == LOWER)
          m_keyType = CAPS;
        else if (m_keyType == CAPS)
          m_keyType = LOWER;
        UpdateButtons();
        break;
      case CTL_BUTTON_LAYOUT:
        OnLayout();
        break;
      case CTL_BUTTON_SYMBOLS:
        OnSymbols();
        break;
      case CTL_BUTTON_LEFT:
        MoveCursor( -1);
        break;
      case CTL_BUTTON_RIGHT:
        MoveCursor(1);
        break;
      case CTL_BUTTON_IP_ADDRESS:
        OnIPAddress();
        break;
      case CTL_BUTTON_CLEAR:
        SetEditText("");
        break;
      case CTL_EDIT:
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CTL_EDIT);
        OnMessage(msg);
        // update callback I guess?
        if (m_pCharCallback)
        { // we did _something_, so make sure our search message filter is reset
          m_pCharCallback(this, msg.GetLabel());
        }
        m_text = msg.GetLabel();
        return true;
      }
      default:
        OnClickButton(iControl);
        break;
      }
    }
    break;

  case GUI_MSG_SET_TEXT:
  case GUI_MSG_INPUT_TEXT_EDIT:
    {
      // the edit control only handles these messages if it is either focues
      // or its specific control ID is set in the message. As neither is the
      // case here (focus is on one of the keyboard buttons) we have to force
      // the control ID of the message to the control ID of the edit control
      // (unfortunately we have to create a whole copy of the message object for that)
      CGUIMessage messageCopy(message.GetMessage(), message.GetSenderId(), CTL_EDIT, message.GetParam1(), message.GetParam2(), message.GetItem());
      messageCopy.SetLabel(message.GetLabel());

      // ensure this goes to the edit control
      CGUIControl *edit = GetControl(CTL_EDIT);
      if (edit)
        edit->OnMessage(messageCopy);

      // close the dialog if requested
      if (message.GetMessage() == GUI_MSG_SET_TEXT && message.GetParam1() > 0)
        OnOK();
      return true;
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogKeyboardGeneric::SetEditText(const std::string &text)
{
  CGUIMessage msg(GUI_MSG_SET_TEXT, GetID(), CTL_EDIT);
  msg.SetLabel(text);
  OnMessage(msg);
}

void CGUIDialogKeyboardGeneric::SetText(const std::string& text)
{
  m_text = text;
}

const std::string &CGUIDialogKeyboardGeneric::GetText() const
{
  return m_text;
}

void CGUIDialogKeyboardGeneric::Character(const std::string &ch)
{
  if (ch.empty()) return;

  // send text to edit control
  CGUIControl *edit = GetControl(CTL_EDIT);
  if (edit)
  {
    CAction action(ACTION_INPUT_TEXT);
    action.SetText(ch);
    edit->OnAction(action);
  }
}

void CGUIDialogKeyboardGeneric::Backspace()
{
  // send action to edit control
  CGUIControl *edit = GetControl(CTL_EDIT);
  if (edit)
    edit->OnAction(CAction(ACTION_BACKSPACE));
}

void CGUIDialogKeyboardGeneric::OnClickButton(int iButtonControl)
{
  if (iButtonControl == CTL_BUTTON_BACKSPACE)
  {
    Backspace();
  }
  else if (iButtonControl == CTL_BUTTON_SPACE)
  {
    Character(" ");
  }
  else
  {
    const CGUIControl* pButton = GetControl(iButtonControl);
    if (pButton)
    {
      Character(pButton->GetDescription());
      // reset the shift keys
      if (m_bShift) OnShift();
    }
  }
}

void CGUIDialogKeyboardGeneric::UpdateButtons()
{
  SET_CONTROL_SELECTED(GetID(), CTL_BUTTON_SHIFT, m_bShift);
  SET_CONTROL_SELECTED(GetID(), CTL_BUTTON_CAPS, m_keyType == CAPS);
  SET_CONTROL_SELECTED(GetID(), CTL_BUTTON_SYMBOLS, m_keyType == SYMBOLS);

  if (m_currentLayout >= m_layouts.size())
    m_currentLayout = 0;
  CKeyboardLayout layout = m_layouts.empty() ? CKeyboardLayout() : m_layouts[m_currentLayout];
  SET_CONTROL_LABEL(CTL_BUTTON_LAYOUT, layout.GetName());

  unsigned int modifiers = CKeyboardLayout::ModifierKeyNone;
  if ((m_keyType == CAPS && !m_bShift) || (m_keyType == LOWER && m_bShift))
    modifiers |= CKeyboardLayout::ModifierKeyShift;
  if (m_keyType == SYMBOLS)
  {
    modifiers |= CKeyboardLayout::ModifierKeySymbol;
    if (m_bShift)
      modifiers |= CKeyboardLayout::ModifierKeyShift;
  }

  for (unsigned int row = 0; row < BUTTONS_MAX_ROWS; row++)
  {
    for (unsigned int column = 0; column < BUTTONS_PER_ROW; column++)
    {
      int buttonID = (row * BUTTONS_PER_ROW) + column + BUTTON_ID_OFFSET;
      std::string label = layout.GetCharAt(row, column, modifiers);
      SetControlLabel(buttonID, label);
      if (!label.empty())
        SET_CONTROL_VISIBLE(buttonID);
      else
        SET_CONTROL_HIDDEN(buttonID);
    }
  }
}

void CGUIDialogKeyboardGeneric::OnDeinitWindow(int nextWindowID)
{
  // call base class
  CGUIDialog::OnDeinitWindow(nextWindowID);
  // reset the heading (we don't always have this)
  m_strHeading = "";

  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Input, "xbmc", "OnInputFinished");
}

void CGUIDialogKeyboardGeneric::MoveCursor(int iAmount)
{
  CGUIControl *edit = GetControl(CTL_EDIT);
  if (edit)
    edit->OnAction(CAction(iAmount < 0 ? ACTION_CURSOR_LEFT : ACTION_CURSOR_RIGHT));
}

void CGUIDialogKeyboardGeneric::OnLayout()
{
  m_currentLayout++;
  if (m_currentLayout >= m_layouts.size())
    m_currentLayout = 0;
  UpdateButtons();
}

void CGUIDialogKeyboardGeneric::OnSymbols()
{
  if (m_keyType == SYMBOLS)
    m_keyType = LOWER;
  else
    m_keyType = SYMBOLS;
  UpdateButtons();
}

void CGUIDialogKeyboardGeneric::OnShift()
{
  m_bShift = !m_bShift;
  UpdateButtons();
}

void CGUIDialogKeyboardGeneric::OnIPAddress()
{
  // find any IP address in the current string if there is any
  // We match to #.#.#.#
  std::string text = GetText();
  std::string ip;
  CRegExp reg;
  reg.RegComp("[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+");
  int start = reg.RegFind(text.c_str());
  int length = 0;
  if (start > -1)
  {
    length = reg.GetSubLength(0);
    ip = text.substr(start, length);
  }
  else
    start = text.size();
  if (CGUIDialogNumeric::ShowAndGetIPAddress(ip, g_localizeStrings.Get(14068)))
    SetEditText(text.substr(0, start) + ip.c_str() + text.substr(start + length));
}

void CGUIDialogKeyboardGeneric::SetControlLabel(int id, const std::string &label)
{ // find all controls with this id, and set all their labels
  CGUIMessage message(GUI_MSG_LABEL_SET, GetID(), id);
  message.SetLabel(label);
  for (unsigned int i = 0; i < m_children.size(); i++)
  {
    if (m_children[i]->GetID() == id || m_children[i]->IsGroup())
      m_children[i]->OnMessage(message);
  }
}

void CGUIDialogKeyboardGeneric::OnOK()
{
  m_bIsConfirmed = true;
  Close();
}

void CGUIDialogKeyboardGeneric::SetHeading(const std::string &heading)
{
  m_strHeading = heading;
}

int CGUIDialogKeyboardGeneric::GetWindowId() const
{
  return GetID();
}

void CGUIDialogKeyboardGeneric::Cancel()
{
  m_bIsConfirmed = false;
  Close();
}

bool CGUIDialogKeyboardGeneric::ShowAndGetInput(char_callback_t pCallback, const std::string &initialString, std::string &typedString, const std::string &heading, bool bHiddenInput)
{
  CGUIDialogKeyboardGeneric *pKeyboard = (CGUIDialogKeyboardGeneric*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);

  if (!pKeyboard)
    return false;

  m_pCharCallback = pCallback;
  // setup keyboard
  pKeyboard->Initialize();
  pKeyboard->SetHeading(heading);
  pKeyboard->SetHiddenInput(bHiddenInput);
  pKeyboard->SetText(initialString);
  // do this using a thread message to avoid render() conflicts
  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_KEYBOARD, g_windowManager.GetActiveWindow()};
  CApplicationMessenger::Get().SendMessage(tMsg, true);
  pKeyboard->Close();

  // If have text - update this.
  if (pKeyboard->IsConfirmed())
  {
    typedString = pKeyboard->GetText();
    return true;
  }
  else return false;
}
