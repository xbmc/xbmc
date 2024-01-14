/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogKeyboardGeneric.h"

#include "GUIDialogNumeric.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/InputCodingTable.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/keyboard/KeyIDs.h"
#include "input/keyboard/KeyboardLayoutManager.h"
#include "input/keyboard/XBMC_vkeys.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "speech/ISpeechRecognition.h"
#include "speech/ISpeechRecognitionListener.h"
#include "speech/SpeechRecognitionErrors.h"
#include "utils/CharsetConverter.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

#include <mutex>

using namespace KODI;
using namespace MESSAGING;

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
#define CTL_BUTTON_REVEAL     310
#define CTL_LABEL_HEADING     311
#define CTL_EDIT              312
#define CTL_LABEL_HZCODE      313
#define CTL_LABEL_HZLIST      314

#define CTL_BUTTON_BACKSPACE    8
#define CTL_BUTTON_SPACE       32

#define SEARCH_DELAY         1000

class CSpeechRecognitionListener : public speech::ISpeechRecognitionListener
{
public:
  CSpeechRecognitionListener(int dialogId) : m_dialogId(dialogId) {}

  void OnReadyForSpeech() override
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                          g_localizeStrings.Get(39177), // Speech to text
                                          g_localizeStrings.Get(39179)); // Listening...
  }

  void OnError(int recognitionError) override
  {
    uint32_t msgId = 0;
    switch (recognitionError)
    {
      case speech::RecognitionError::SERVICE_NOT_AVAILABLE:
        msgId = 39178; // Speech recognition service not available
        break;
      case speech::RecognitionError::NO_MATCH:
        msgId = 39180; // No recognition result matched
        break;
      case speech::RecognitionError::INSUFFICIENT_PERMISSIONS:
        msgId = 39185; // Insufficient permissions for speech recognition
        break;
      default:
        msgId = 39181; // Speech recognition error
        break;
    }

    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error,
                                          g_localizeStrings.Get(39177), // Speech to text
                                          g_localizeStrings.Get(msgId));
  }

  void OnResults(const std::vector<std::string>& results) override
  {
    if (!results.empty())
    {
      CGUIMessage msg(GUI_MSG_SET_TEXT, m_dialogId, CTL_EDIT);
      msg.SetLabel(results.front());

      // dispatch to GUI thread
      CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, m_dialogId);
    }
  }

private:
  const int m_dialogId{0};
};

CGUIDialogKeyboardGeneric::CGUIDialogKeyboardGeneric()
: CGUIDialog(WINDOW_DIALOG_KEYBOARD, "DialogKeyboard.xml")
, CGUIKeyboard()
, m_pCharCallback(NULL)
{
  m_bIsConfirmed = false;
  m_bShift = false;
  m_hiddenInput = false;
  m_keyType = KEY_TYPE::LOWER;
  m_currentLayout = 0;
  m_loadType = KEEP_IN_MEMORY;
  m_isKeyboardNavigationMode = false;
  m_previouslyFocusedButton = 0;
  m_pos = 0;
  m_listwidth = 600;
}

void CGUIDialogKeyboardGeneric::OnWindowLoaded()
{
  CGUIEditControl *edit = static_cast<CGUIEditControl*>(GetControl(CTL_EDIT));
  if (edit)
  {
    // add control CTL_LABEL_HZCODE and CTL_LABEL_HZLIST if not exist
    CGUIControlGroup *ParentControl = static_cast<CGUIControlGroup*>(edit->GetParentControl());
    CLabelInfo labelInfo = edit->GetLabelInfo();
    float px = edit->GetXPosition();
    float py = edit->GetYPosition();
    float pw = edit->GetWidth();
    float ph = edit->GetHeight();

    CGUILabelControl* control = static_cast<CGUILabelControl*>(GetControl(CTL_LABEL_HZCODE));
    if (!control)
    {
      control = new CGUILabelControl(GetID(), CTL_LABEL_HZCODE, px, py + ph, 90, 30, labelInfo, false, false);
      ParentControl->AddControl(control);
    }

    control = static_cast<CGUILabelControl*>(GetControl(CTL_LABEL_HZLIST));
    if (!control)
    {
      labelInfo.align = XBFONT_CENTER_Y;
      control = new CGUILabelControl(GetID(), CTL_LABEL_HZLIST, px + 95, py + ph, pw - 95, 30, labelInfo, false, false);
      ParentControl->AddControl(control);
    }
  }

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
  const KEYBOARD::KeyboardLayouts& keyboardLayouts =
      CServiceBroker::GetKeyboardLayoutManager()->GetLayouts();
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  std::vector<CVariant> layoutNames = settings->GetList(CSettings::SETTING_LOCALE_KEYBOARDLAYOUTS);
  std::string activeLayout = settings->GetString(CSettings::SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT);

  for (const auto& layoutName : layoutNames)
  {
    const auto keyboardLayout = keyboardLayouts.find(layoutName.asString());
    if (keyboardLayout != keyboardLayouts.end())
    {
      m_layouts.emplace_back(keyboardLayout->second);
      if (layoutName.asString() == activeLayout)
        m_currentLayout = m_layouts.size() - 1;
    }
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
  if (m_hiddenInput)
  {
    SET_CONTROL_VISIBLE(CTL_BUTTON_REVEAL);
    SET_CONTROL_LABEL(CTL_BUTTON_REVEAL, g_localizeStrings.Get(12308));
  }
  else
    SET_CONTROL_HIDDEN(CTL_BUTTON_REVEAL);

  SetEditText(m_text);

  // get HZLIST label options
  CGUILabelControl* pEdit = static_cast<CGUILabelControl*>(GetControl(CTL_LABEL_HZLIST));
  CLabelInfo labelInfo = pEdit->GetLabelInfo();
  m_listfont = labelInfo.font;
  m_listwidth = pEdit->GetWidth();
  m_hzcode.clear();
  m_words.clear();
  SET_CONTROL_LABEL(CTL_LABEL_HZCODE, "");
  SET_CONTROL_LABEL(CTL_LABEL_HZLIST, "");

  CVariant data;
  data["title"] = m_strHeading;
  data["type"] = !m_hiddenInput ? "keyboard" : "password";
  data["value"] = GetText();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Input, "OnInputRequested", data);
}

bool CGUIDialogKeyboardGeneric::OnAction(const CAction &action)
{
  int actionId = action.GetID();
  bool handled = true;
  if (actionId == (KEY_VKEY | XBMCVK_BACK))
    Backspace();
  else if (actionId == ACTION_ENTER ||
           (actionId == ACTION_SELECT_ITEM && (m_isKeyboardNavigationMode || GetFocusedControlID() == CTL_EDIT)))
    OnOK();
  else if (actionId == ACTION_SHIFT)
    OnShift();
  else if (actionId == ACTION_SYMBOLS)
    OnSymbols();
  // don't handle move left/right and select in the edit control
  else if (!m_isKeyboardNavigationMode &&
           (actionId == ACTION_MOVE_LEFT ||
           actionId == ACTION_MOVE_RIGHT ||
           actionId == ACTION_SELECT_ITEM))
    handled = false;
  else if (actionId == ACTION_VOICE_RECOGNIZE)
    OnVoiceRecognition();
  else
  {
    std::wstring wch = L"";
    wch.insert(wch.begin(), action.GetUnicode());
    std::string ch;
    g_charsetConverter.wToUTF8(wch, ch);
    handled = CodingCharacter(ch);
    if (!handled)
    {
      // send action to edit control
      CGUIControl *edit = GetControl(CTL_EDIT);
      if (edit)
        handled = edit->OnAction(action);
      if (!handled && actionId >= KEY_VKEY && actionId < KEY_UNICODE)
      {
        unsigned char b = actionId & 0xFF;
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
        if (m_keyType == KEY_TYPE::LOWER)
          m_keyType = KEY_TYPE::CAPS;
        else if (m_keyType == KEY_TYPE::CAPS)
          m_keyType = KEY_TYPE::LOWER;
        UpdateButtons();
        break;
      case CTL_BUTTON_LAYOUT:
        OnLayout();
        break;
      case CTL_BUTTON_REVEAL:
        OnReveal();
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
    {
      // the edit control only handles these messages if it is either focused
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
  case GUI_MSG_CODINGTABLE_LOOKUP_COMPLETED:
    {
      const std::string& code = message.GetStringParam();
      if (code == m_hzcode)
      {
        int response = message.GetParam1();
        auto words = m_codingtable->GetResponse(response);
        m_words.insert(m_words.end(), words.begin(), words.end());
        ShowWordList(0);
      }
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
  if (!CodingCharacter(ch))
    NormalCharacter(ch);
}

void CGUIDialogKeyboardGeneric::NormalCharacter(const std::string &ch)
{
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
  if (m_codingtable && m_hzcode.length() > 0)
  {
    std::wstring tmp;
    g_charsetConverter.utf8ToW(m_hzcode, tmp);
    tmp.erase(tmp.length() - 1, 1);
    g_charsetConverter.wToUTF8(tmp, m_hzcode);

    switch (m_codingtable->GetType())
    {
    case IInputCodingTable::TYPE_WORD_LIST:
      SetControlLabel(CTL_LABEL_HZCODE, m_hzcode);
      ChangeWordList(0);
      break;

    case IInputCodingTable::TYPE_CONVERT_STRING:
      SetEditText(m_codingtable->ConvertString(m_hzcode));
      break;
    }
  }
  else
  {
    // send action to edit control
    CGUIControl *edit = GetControl(CTL_EDIT);
    if (edit)
      edit->OnAction(CAction(ACTION_BACKSPACE));

    if (m_codingtable && m_codingtable->GetType() == IInputCodingTable::TYPE_CONVERT_STRING)
      m_codingtable->SetTextPrev(GetText());
  }
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
    // Do not register input for buttons with id >= 500
    if (pButton && iButtonControl < 500)
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
  SET_CONTROL_SELECTED(GetID(), CTL_BUTTON_CAPS, m_keyType == KEY_TYPE::CAPS);
  SET_CONTROL_SELECTED(GetID(), CTL_BUTTON_SYMBOLS, m_keyType == KEY_TYPE::SYMBOLS);

  if (m_currentLayout >= m_layouts.size())
    m_currentLayout = 0;
  KEYBOARD::CKeyboardLayout layout =
      m_layouts.empty() ? KEYBOARD::CKeyboardLayout() : m_layouts[m_currentLayout];
  m_codingtable = layout.GetCodingTable();
  if (m_codingtable && !m_codingtable->IsInitialized())
    m_codingtable->Initialize();

  bool bShowWordList = false;
  if (m_codingtable)
  {
    switch (m_codingtable->GetType())
    {
    case IInputCodingTable::TYPE_WORD_LIST:
      bShowWordList = true;
      break;

    case IInputCodingTable::TYPE_CONVERT_STRING:
      m_codingtable->SetTextPrev(GetText());
      m_hzcode.clear();
      break;
    }
  }

  if (bShowWordList)
  {
    SET_CONTROL_VISIBLE(CTL_LABEL_HZCODE);
    SET_CONTROL_VISIBLE(CTL_LABEL_HZLIST);
  }
  else
  {
    SET_CONTROL_HIDDEN(CTL_LABEL_HZCODE);
    SET_CONTROL_HIDDEN(CTL_LABEL_HZLIST);
  }
  SET_CONTROL_LABEL(CTL_BUTTON_LAYOUT, layout.GetName());

  unsigned int modifiers = KEYBOARD::CKeyboardLayout::ModifierKeyNone;
  if ((m_keyType == KEY_TYPE::CAPS && !m_bShift) || (m_keyType == KEY_TYPE::LOWER && m_bShift))
    modifiers |= KEYBOARD::CKeyboardLayout::ModifierKeyShift;
  if (m_keyType == KEY_TYPE::SYMBOLS)
  {
    modifiers |= KEYBOARD::CKeyboardLayout::ModifierKeySymbol;
    if (m_bShift)
      modifiers |= KEYBOARD::CKeyboardLayout::ModifierKeyShift;
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
  for (auto& layout : m_layouts)
  {
    auto codingTable = layout.GetCodingTable();
    if (codingTable && codingTable->IsInitialized())
      codingTable->Deinitialize();
  }
  // call base class
  CGUIDialog::OnDeinitWindow(nextWindowID);
  // reset the heading (we don't always have this)
  m_strHeading = "";

  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Input, "OnInputFinished");
}

void CGUIDialogKeyboardGeneric::MoveCursor(int iAmount)
{
  if (m_codingtable && m_words.size())
    ChangeWordList(iAmount);
  else
  {
    CGUIControl *edit = GetControl(CTL_EDIT);
    if (edit)
      edit->OnAction(CAction(iAmount < 0 ? ACTION_CURSOR_LEFT : ACTION_CURSOR_RIGHT));
  }
}

void CGUIDialogKeyboardGeneric::OnLayout()
{
  m_currentLayout++;
  if (m_currentLayout >= m_layouts.size())
    m_currentLayout = 0;
  KEYBOARD::CKeyboardLayout layout =
      m_layouts.empty() ? KEYBOARD::CKeyboardLayout() : m_layouts[m_currentLayout];
  CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT, layout.GetName());
  UpdateButtons();
}

void CGUIDialogKeyboardGeneric::OnSymbols()
{
  if (m_keyType == KEY_TYPE::SYMBOLS)
    m_keyType = KEY_TYPE::LOWER;
  else
    m_keyType = KEY_TYPE::SYMBOLS;
  UpdateButtons();
}

void CGUIDialogKeyboardGeneric::OnReveal()
{
  m_hiddenInput = !m_hiddenInput;
  SET_CONTROL_LABEL(CTL_BUTTON_REVEAL, g_localizeStrings.Get(m_hiddenInput ? 12308 : 12309));
  CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CTL_EDIT,
                  m_hiddenInput ? CGUIEditControl::INPUT_TYPE_PASSWORD
                                : CGUIEditControl::INPUT_TYPE_TEXT);
  OnMessage(msg);
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
    SetEditText(text.substr(0, start) + ip + text.substr(start + length));
}

void CGUIDialogKeyboardGeneric::OnVoiceRecognition()
{
  const auto speechRecognition = CServiceBroker::GetSpeechRecognition();
  if (speechRecognition)
  {
    if (!m_speechRecognitionListener)
      m_speechRecognitionListener = std::make_shared<CSpeechRecognitionListener>(GetID());

    speechRecognition->StartSpeechRecognition(m_speechRecognitionListener);
  }
  else
  {
    CLog::LogF(LOGWARNING, "No voice recognition implementation available.");
  }
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
  CGUIDialogKeyboardGeneric *pKeyboard = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogKeyboardGeneric>(WINDOW_DIALOG_KEYBOARD);

  if (!pKeyboard)
    return false;

  m_pCharCallback = pCallback;
  // setup keyboard
  pKeyboard->Initialize();
  pKeyboard->SetHeading(heading);
  pKeyboard->SetHiddenInput(bHiddenInput);
  pKeyboard->SetText(initialString);
  pKeyboard->Open();
  pKeyboard->Close();

  // If have text - update this.
  if (pKeyboard->IsConfirmed())
  {
    typedString = pKeyboard->GetText();
    return true;
  }
  else return false;
}

float CGUIDialogKeyboardGeneric::GetStringWidth(const std::wstring & utf16)
{
  vecText utf32;

  utf32.resize(utf16.size());
  for (unsigned int i = 0; i < utf16.size(); i++)
    utf32[i] = utf16[i];

  return m_listfont->GetTextWidth(utf32);
}

void CGUIDialogKeyboardGeneric::ChangeWordList(int direct)
{
  if (direct == 0)
  {
    m_pos = 0;
    m_words.clear();
    m_codingtable->GetWordListPage(m_hzcode, true);
  }
  else
  {
    ShowWordList(direct);
    if (direct > 0 && m_pos + m_num == static_cast<int>(m_words.size()))
      m_codingtable->GetWordListPage(m_hzcode, false);
  }
}

void CGUIDialogKeyboardGeneric::ShowWordList(int direct)
{
  std::unique_lock<CCriticalSection> lock(m_CS);
  std::wstring hzlist = L"";
  CServiceBroker::GetWinSystem()->GetGfxContext().SetScalingResolution(m_coordsRes, true);
  float width = m_listfont->GetCharWidth(L'<') + m_listfont->GetCharWidth(L'>');
  float spacewidth = m_listfont->GetCharWidth(L' ');
  float numwidth = m_listfont->GetCharWidth(L'1') + m_listfont->GetCharWidth(L'.');
  int i;

  if (direct >= 0)
  {
    if (direct > 0)
      m_pos += m_num;
    if (m_pos > static_cast<int>(m_words.size()) - 1)
      m_pos = 0;
    for (i = 0; m_pos + i < static_cast<int>(m_words.size()); i++)
    {
      if ((i > 0 && width + GetStringWidth(m_words[m_pos + i]) + numwidth > m_listwidth) || i > 9)
        break;
      hzlist.insert(hzlist.length(), 1, (wchar_t)(i + 48));
      hzlist.insert(hzlist.length(), 1, L'.');
      hzlist.append(m_words[m_pos + i]);
      hzlist.insert(hzlist.length(), 1, L' ');
      width += GetStringWidth(m_words[m_pos + i]) + numwidth + spacewidth;
    }
    m_num = i;
  }
  else
  {
    if (m_pos == 0)
      return;
    for (i = 1; i <= 10; i++)
    {
      if (m_pos - i < 0 || (i > 1 && width + GetStringWidth(m_words[m_pos - i]) + numwidth > m_listwidth))
        break;
      width += GetStringWidth(m_words[m_pos - i]) + numwidth + spacewidth;
    }
    m_num = --i;
    m_pos -= m_num;
    for (i = 0; i < m_num; i++)
    {
      hzlist.insert(hzlist.length(), 1, (wchar_t)(i + 48));
      hzlist.insert(hzlist.length(), 1, L'.');
      hzlist.append(m_words[m_pos + i]);
      hzlist.insert(hzlist.length(), 1, L' ');
    }
  }
  hzlist.erase(hzlist.find_last_not_of(L' ') + 1);
  if (m_pos > 0)
    hzlist.insert(0, 1, L'<');
  if (m_pos + m_num < static_cast<int>(m_words.size()))
    hzlist.insert(hzlist.length(), 1, L'>');
  std::string utf8String;
  g_charsetConverter.wToUTF8(hzlist, utf8String);
  SET_CONTROL_LABEL(CTL_LABEL_HZLIST, utf8String);
}

bool CGUIDialogKeyboardGeneric::CodingCharacter(const std::string &ch)
{
  if (!m_codingtable)
    return false;

  switch (m_codingtable->GetType())
  {
  case IInputCodingTable::TYPE_CONVERT_STRING:
    if (!ch.empty() && ch[0] != 0)
    {
      m_hzcode += ch;
      SetEditText(m_codingtable->ConvertString(m_hzcode));
      return true;
    }
    break;

  case IInputCodingTable::TYPE_WORD_LIST:
    if (m_codingtable->GetCodeChars().find(ch) != std::string::npos)
    {
      m_hzcode += ch;
      SetControlLabel(CTL_LABEL_HZCODE, m_hzcode);
      ChangeWordList(0);
      return true;
    }
    if (ch[0] >= '0' && ch[0] <= '9')
    {
      int i = m_pos + (int)ch[0] - 48;
      if (i < (m_pos + m_num))
      {
        m_hzcode = "";
        SetControlLabel(CTL_LABEL_HZCODE, m_hzcode);
        std::string utf8String;
        g_charsetConverter.wToUTF8(m_words[i], utf8String);
        NormalCharacter(utf8String);
      }
      return true;
    }
    break;
  }

  return false;
}
