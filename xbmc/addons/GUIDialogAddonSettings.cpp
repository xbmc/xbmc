/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogAddonSettings.h"
#include "filesystem/PluginDirectory.h"
#include "addons/IAddon.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUISettingsSliderControl.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "storage/MediaManager.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIImage.h"
#include "input/Key.h"
#include "filesystem/Directory.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "guilib/GUIKeyboardFactory.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "dialogs/GUIDialogSelect.h"
#include "GUIWindowAddonBrowser.h"
#include "utils/log.h"
#include "URL.h"
#include "utils/XMLUtils.h"
#include "utils/Variant.h"

using namespace ADDON;
using namespace KODI::MESSAGING;
using XFILE::CDirectory;

#define CONTROL_SETTINGS_AREA           2
#define CONTROL_DEFAULT_BUTTON          3
#define CONTROL_DEFAULT_RADIOBUTTON     4
#define CONTROL_DEFAULT_SPIN            5
#define CONTROL_DEFAULT_SEPARATOR       6
#define CONTROL_DEFAULT_LABEL_SEPARATOR 7
#define CONTROL_DEFAULT_SLIDER          8
#define CONTROL_SECTION_AREA            9
#define CONTROL_DEFAULT_SECTION_BUTTON  13

#define ID_BUTTON_OK                    10
#define ID_BUTTON_CANCEL                11
#define ID_BUTTON_DEFAULT               12
#define CONTROL_HEADING_LABEL           20

#define CONTROL_START_SECTION           100
#define CONTROL_START_SETTING           200

CGUIDialogAddonSettings::CGUIDialogAddonSettings()
   : CGUIDialogBoxBase(WINDOW_DIALOG_ADDON_SETTINGS, "DialogAddonSettings.xml")
{
  m_currentSection = 0;
  m_totalSections = 1;
  m_saveToDisk = false;
}

CGUIDialogAddonSettings::~CGUIDialogAddonSettings(void)
{
}

bool CGUIDialogAddonSettings::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      FreeSections();
    }
    break;
    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      bool bCloseDialog = false;

      if (iControl == ID_BUTTON_DEFAULT)
        SetDefaultSettings();
      else if (iControl != ID_BUTTON_OK)
        bCloseDialog = ShowVirtualKeyboard(iControl);

      if (iControl == ID_BUTTON_OK || iControl == ID_BUTTON_CANCEL || bCloseDialog)
      {
        if (iControl == ID_BUTTON_OK || bCloseDialog)
        {
          m_bConfirmed = true;
          SaveSettings();
        }
        Close();
        return true;
      }
    }
    break;
    case GUI_MSG_FOCUSED:
    {
      CGUIDialogBoxBase::OnMessage(message);
      int focusedControl = GetFocusedControlID();
      if (focusedControl >= CONTROL_START_SECTION && focusedControl < (int)(CONTROL_START_SECTION + m_totalSections) &&
          focusedControl - CONTROL_START_SECTION != (int)m_currentSection)
      { // changing section
        UpdateFromControls();
        m_currentSection = focusedControl - CONTROL_START_SECTION;
        CreateControls();
      }
      return true;
    }
    case GUI_MSG_SETTING_UPDATED:
    {
      std::string      id = message.GetStringParam(0);
      std::string value   = message.GetStringParam(1);
      m_settings[id] = value;
      if (GetFocusedControl())
      {
        int iControl = GetFocusedControl()->GetID();
        CreateControls();
        CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(),iControl);
        OnMessage(msg);
      }
      else
        CreateControls();
      return true;
    }
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

bool CGUIDialogAddonSettings::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_DELETE_ITEM)
  {
    CGUIControl* pControl = GetFocusedControl();
    if (pControl)
    {
      int iControl = pControl->GetID();
      int controlId = CONTROL_START_SETTING;
      const TiXmlElement* setting = GetFirstSetting();
      UpdateFromControls();
      while (setting)
      {
        if (controlId == iControl)
        {
          const char* id = setting->Attribute("id");
          const char* value = setting->Attribute("default");
          if (id && value)
            m_settings[id] = value;
          CreateControls();
          CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(),iControl);
          OnMessage(msg);
          return true;
        }
        setting = setting->NextSiblingElement("setting");
        controlId++;
      }
    }
  }
  return CGUIDialogBoxBase::OnAction(action);
}

void CGUIDialogAddonSettings::OnInitWindow()
{
  m_currentSection = 0;
  m_totalSections = 1;
  CreateSections();
  CreateControls();
  CGUIDialogBoxBase::OnInitWindow();

  SET_CONTROL_VISIBLE(ID_BUTTON_OK);
  SET_CONTROL_VISIBLE(ID_BUTTON_CANCEL);
  SET_CONTROL_VISIBLE(ID_BUTTON_DEFAULT);
  SET_CONTROL_VISIBLE(CONTROL_HEADING_LABEL);
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
bool CGUIDialogAddonSettings::ShowAndGetInput(const AddonPtr &addon, bool saveToDisk /* = true */)
{
  if (!addon)
    return false;

  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return false;

  bool ret(false);
  if (addon->HasSettings())
  { 
    // Create the dialog
    CGUIDialogAddonSettings* pDialog = NULL;
    pDialog = (CGUIDialogAddonSettings*) g_windowManager.GetWindow(WINDOW_DIALOG_ADDON_SETTINGS);
    if (!pDialog)
      return false;

    // Set the heading
    std::string heading = StringUtils::Format("$LOCALIZE[10004] - %s", addon->Name().c_str()); // "Settings - AddonName"
    pDialog->m_strHeading = heading;

    pDialog->m_addon = addon;
    pDialog->m_saveToDisk = saveToDisk;
    pDialog->Open();
    ret = true;
  }
  else
  { // addon does not support settings, inform user
    CGUIDialogOK::ShowAndGetInput(CVariant{24000}, CVariant{24030});
  }

  return ret;
}

bool CGUIDialogAddonSettings::ShowVirtualKeyboard(int iControl)
{
  int controlId = CONTROL_START_SETTING;
  bool bCloseDialog = false;

  const TiXmlElement *setting = GetFirstSetting();
  while (setting)
  {
    if (controlId == iControl)
    {
      const CGUIControl* control = GetControl(controlId);
      const std::string id = XMLUtils::GetAttribute(setting, "id");
      const std::string type = XMLUtils::GetAttribute(setting, "type");

      //! @todo Refactor me. Special handling for actions: does not require id attribute.
      if (control && control->GetControlType() == CGUIControl::GUICONTROL_BUTTON && type == "action")
      {
        const char *option = setting->Attribute("option");
        std::string action = XMLUtils::GetAttribute(setting, "action");
        if (!action.empty())
        {
          // replace $CWD with the url of plugin/script
          StringUtils::Replace(action, "$CWD", m_addon->Path());
          StringUtils::Replace(action, "$ID", m_addon->ID());
          if (option)
            bCloseDialog = (strcmpi(option, "close") == 0);
          CApplicationMessenger::GetInstance().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, action);
        }
        break;
      }

      if (control && control->GetControlType() == CGUIControl::GUICONTROL_BUTTON &&
          !id.empty() && !type.empty())
      {
        const char *option = setting->Attribute("option");
        const char *source = setting->Attribute("source");
        std::string value = m_buttonValues[id];
        std::string label = GetString(setting->Attribute("label"));

        if (type == "text")
        {
          // get any options
          bool bHidden  = false;
          bool bEncoded = false;
          if (option)
          {
            bHidden = (strstr(option, "hidden") != NULL);
            bEncoded = (strstr(option, "urlencoded") != NULL);
          }
          if (bEncoded)
            value = CURL::Decode(value);

          if (CGUIKeyboardFactory::ShowAndGetInput(value, CVariant{label}, true, bHidden))
          {
            // if hidden hide input
            if (bHidden)
            {
              std::string hiddenText;
              hiddenText.append(value.size(), L'*');
              ((CGUIButtonControl *)control)->SetLabel2(hiddenText);
            }
            else
              ((CGUIButtonControl*) control)->SetLabel2(value);
            if (bEncoded)
              value = CURL::Encode(value);
          }
        }
        else if (type == "number" && CGUIDialogNumeric::ShowAndGetNumber(value, label))
        {
          ((CGUIButtonControl*) control)->SetLabel2(value);
        }
        else if (type == "ipaddress" && CGUIDialogNumeric::ShowAndGetIPAddress(value, label))
        {
          ((CGUIButtonControl*) control)->SetLabel2(value);
        }
        else if (type == "select")
        {
          CGUIDialogSelect *pDlg = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
          if (pDlg)
          {
            pDlg->SetHeading(CVariant{label});
            pDlg->Reset();

            int selected = -1;
            std::vector<std::string> valuesVec;
            if (setting->Attribute("values"))
              StringUtils::Tokenize(setting->Attribute("values"), valuesVec, "|");
            else if (setting->Attribute("lvalues"))
            { // localize
              StringUtils::Tokenize(setting->Attribute("lvalues"), valuesVec, "|");
              for (unsigned int i = 0; i < valuesVec.size(); i++)
              {
                if (i == (unsigned int)atoi(value.c_str()))
                  selected = i;
                std::string localized = g_localizeStrings.GetAddonString(m_addon->ID(), atoi(valuesVec[i].c_str()));
                if (localized.empty())
                  localized = g_localizeStrings.Get(atoi(valuesVec[i].c_str()));
                valuesVec[i] = localized;
              }
            }
            else if (source)
            {
              valuesVec = GetFileEnumValues(source, XMLUtils::GetAttribute(setting, "mask"), XMLUtils::GetAttribute(setting, "option"));
            }

            for (unsigned int i = 0; i < valuesVec.size(); i++)
            {
              pDlg->Add(valuesVec[i]);
              if (selected == (int)i || (selected < 0 && StringUtils::EqualsNoCase(valuesVec[i], value)))
                pDlg->SetSelected(i); // FIXME: the SetSelected() does not select "i", it always defaults to the first position
            }
            pDlg->Open();
            int iSelected = pDlg->GetSelectedItem();
            if (iSelected >= 0)
            {
              if (setting->Attribute("lvalues"))
                value = StringUtils::Format("%i", iSelected);
              else
                value = valuesVec[iSelected];
              ((CGUIButtonControl*) control)->SetLabel2(valuesVec[iSelected]);
            }
          }
        }
        else if (type == "audio" || type == "video"
              || type == "image" || type == "executable"
              || type == "file"  || type == "folder")
        {
          // setup the shares
          VECSOURCES *shares = NULL;
          if (source && strcmpi(source, "") != 0)
            shares = CMediaSourceSettings::GetInstance().GetSources(source);

          VECSOURCES localShares;
          if (!shares)
          {
            g_mediaManager.GetLocalDrives(localShares);
            if (!source || strcmpi(source, "local") != 0)
              g_mediaManager.GetNetworkLocations(localShares);
          }
          else // always append local drives
          {
            localShares = *shares;
            g_mediaManager.GetLocalDrives(localShares);
          }

          if (type == "folder")
          {
            // get any options
            bool bWriteOnly = false;
            if (option)
              bWriteOnly = (strcmpi(option, "writeable") == 0);

            if (CGUIDialogFileBrowser::ShowAndGetDirectory(localShares, label, value, bWriteOnly))
              ((CGUIButtonControl*) control)->SetLabel2(value);
          }
          else if (type == "image")
          {
            if (CGUIDialogFileBrowser::ShowAndGetImage(localShares, label, value))
              ((CGUIButtonControl*) control)->SetLabel2(value);
          }
          else
          {
            // set the proper mask
            std::string strMask;
            if (setting->Attribute("mask"))
            {
              strMask = setting->Attribute("mask");
              // convert mask qualifiers
              StringUtils::Replace(strMask, "$AUDIO", g_advancedSettings.GetMusicExtensions());
              StringUtils::Replace(strMask, "$VIDEO", g_advancedSettings.m_videoExtensions);
              StringUtils::Replace(strMask, "$IMAGE", g_advancedSettings.m_pictureExtensions);
#if defined(_WIN32_WINNT)
              StringUtils::Replace(strMask, "$EXECUTABLE", ".exe|.bat|.cmd|.py");
#else
              StringUtils::Replace(strMask, "$EXECUTABLE", "");
#endif
            }
            else
            {
              if (type == "video")
                strMask = g_advancedSettings.m_videoExtensions;
              else if (type == "audio")
                strMask = g_advancedSettings.GetMusicExtensions();
              else if (type == "executable")
#if defined(_WIN32_WINNT)
                strMask = ".exe|.bat|.cmd|.py";
#else
                strMask = "";
#endif
            }

            // get any options
            bool bUseThumbs = false;
            bool bUseFileDirectories = false;
            if (option)
            {
              std::vector<std::string> options = StringUtils::Split(option, '|');
              bUseThumbs = find(options.begin(), options.end(), "usethumbs") != options.end();
              bUseFileDirectories = find(options.begin(), options.end(), "treatasfolder") != options.end();
            }

            if (CGUIDialogFileBrowser::ShowAndGetFile(localShares, strMask, label, value, bUseThumbs, bUseFileDirectories))
              ((CGUIButtonControl*) control)->SetLabel2(value);
          }
        }
        else if (type == "date")
        {
          CDateTime date;
          if (!value.empty())
            date.SetFromDBDate(value);
          SYSTEMTIME timedate;
          date.GetAsSystemTime(timedate);
          if(CGUIDialogNumeric::ShowAndGetDate(timedate, label))
          {
            date = timedate;
            value = date.GetAsDBDate();
            ((CGUIButtonControl*) control)->SetLabel2(value);
          }
        }
        else if (type == "time")
        {
          SYSTEMTIME timedate;
          if (value.size() >= 5)
          {
            // assumes HH:MM
            timedate.wHour = atoi(value.substr(0, 2).c_str());
            timedate.wMinute = atoi(value.substr(3, 2).c_str());
          }
          if (CGUIDialogNumeric::ShowAndGetTime(timedate, label))
          {
            value = StringUtils::Format("%02d:%02d", timedate.wHour, timedate.wMinute);
            ((CGUIButtonControl*) control)->SetLabel2(value);
          }
        }
        else if (type == "addon")
        {
          const char *strType = setting->Attribute("addontype");
          if (strType)
          {
            std::vector<std::string> addonTypes = StringUtils::Split(strType, ',');
            std::vector<ADDON::TYPE> types;
            for (std::vector<std::string>::iterator i = addonTypes.begin(); i != addonTypes.end(); ++i)
            {
              StringUtils::Trim(*i);
              ADDON::TYPE type = TranslateType(*i);
              if (type != ADDON_UNKNOWN)
                types.push_back(type);
            }
            if (!types.empty())
            {
              const char *strMultiselect = setting->Attribute("multiselect");
              bool multiSelect = strMultiselect && strcmpi(strMultiselect, "true") == 0;
              if (multiSelect)
              {
                // construct vector of addon IDs (IDs are comma seperated in single string)
                std::vector<std::string> addonIDs = StringUtils::Split(value, ',');
                if (CGUIWindowAddonBrowser::SelectAddonID(types, addonIDs, false) == 1)
                {
                  value = StringUtils::Join(addonIDs, ",");
                  ((CGUIButtonControl*) control)->SetLabel2(GetAddonNames(value));
                }
              }
              else // no need of string splitting/joining if we select only 1 addon
                if (CGUIWindowAddonBrowser::SelectAddonID(types, value, false) == 1)
                  ((CGUIButtonControl*) control)->SetLabel2(GetAddonNames(value));
            }
          }
        }
        m_buttonValues[id] = value;
        break;
      }
    }
    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
  EnableControls();
  return bCloseDialog;
}

void CGUIDialogAddonSettings::UpdateFromControls()
{
  int controlID = CONTROL_START_SETTING;
  const TiXmlElement *setting = GetFirstSetting();
  while (setting)
  {
    const std::string id   = XMLUtils::GetAttribute(setting, "id");
    const std::string type = XMLUtils::GetAttribute(setting, "type");
    const CGUIControl* control = GetControl(controlID++);

    if (control)
    {
      std::string value;
      switch (control->GetControlType())
      {
        case CGUIControl::GUICONTROL_BUTTON:
          value = m_buttonValues[id];
          break;
        case CGUIControl::GUICONTROL_RADIO:
          value = ((CGUIRadioButtonControl*) control)->IsSelected() ? "true" : "false";
          break;
        case CGUIControl::GUICONTROL_SPINEX:
          if (type == "fileenum" || type == "labelenum")
            value = ((CGUISpinControlEx*) control)->GetLabel();
          else
            value = StringUtils::Format("%i", ((CGUISpinControlEx*) control)->GetValue());
          break;
        case CGUIControl::GUICONTROL_SETTINGS_SLIDER:
          {
            std::string option = XMLUtils::GetAttribute(setting, "option");
            if (option.empty() || StringUtils::EqualsNoCase(option, "float"))
              value = StringUtils::Format("%f", ((CGUISettingsSliderControl *)control)->GetFloatValue());
            else
              value = StringUtils::Format("%i", ((CGUISettingsSliderControl *)control)->GetIntValue());
          }
          break;
        default:
          break;
      }
      m_settings[id] = value;
    }

    setting = setting->NextSiblingElement("setting");
  }
}

void CGUIDialogAddonSettings::SaveSettings(void)
{
  UpdateFromControls();

  for (std::map<std::string, std::string>::iterator i = m_settings.begin(); i != m_settings.end(); ++i)
    m_addon->UpdateSetting(i->first, i->second);

  if (m_saveToDisk)
  { 
    m_addon->SaveSettings();
  } 
}

void CGUIDialogAddonSettings::FreeSections()
{
  CGUIControlGroupList *group = dynamic_cast<CGUIControlGroupList *>(GetControl(CONTROL_SECTION_AREA));
  if (group)
  {
    group->FreeResources();
    group->ClearAll();
  }
  m_settings.clear();
  m_buttonValues.clear();
  FreeControls();
}

void CGUIDialogAddonSettings::FreeControls()
{
  // clear the category group
  CGUIControlGroupList *control = dynamic_cast<CGUIControlGroupList *>(GetControl(CONTROL_SETTINGS_AREA));
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
}

void CGUIDialogAddonSettings::CreateSections()
{
  CGUIControlGroupList *group = dynamic_cast<CGUIControlGroupList *>(GetControl(CONTROL_SECTION_AREA));
  CGUIButtonControl *originalButton = dynamic_cast<CGUIButtonControl *>(GetControl(CONTROL_DEFAULT_SECTION_BUTTON));
  if (!m_addon)
    return;

  if (originalButton)
    originalButton->SetVisible(false);

  // clear the category group
  FreeSections();

  // grab our categories
  const TiXmlElement *category = m_addon->GetSettingsXML()->FirstChildElement("category");
  if (!category) // add a default one...
    category = m_addon->GetSettingsXML();
 
  int buttonID = CONTROL_START_SECTION;
  while (category)
  { // add a category
    CGUIButtonControl *button = originalButton ? originalButton->Clone() : NULL;

    std::string label = GetString(category->Attribute("label"));
    if (label.empty())
      label = g_localizeStrings.Get(128);

    if (buttonID >= CONTROL_START_SETTING)
    {
      CLog::Log(LOGERROR, "%s - cannot have more than %d categories - simplify your addon!", __FUNCTION__, CONTROL_START_SETTING - CONTROL_START_SECTION);
      break;
    }

    // add the category button
    if (button && group)
    {
      button->SetID(buttonID++);
      button->SetLabel(label);
      button->SetVisible(true);
      group->AddControl(button);
    }

    // grab a local copy of all the settings in this category
    const TiXmlElement *setting = category->FirstChildElement("setting");
    while (setting)
    {
      const std::string id = XMLUtils::GetAttribute(setting, "id");
      if (!id.empty())
        m_settings[id] = m_addon->GetSetting(id);
      setting = setting->NextSiblingElement("setting");
    }
    category = category->NextSiblingElement("category");
  }
  m_totalSections = buttonID - CONTROL_START_SECTION;
}

void CGUIDialogAddonSettings::CreateControls()
{
  FreeControls();

  CGUISpinControlEx *pOriginalSpin = dynamic_cast<CGUISpinControlEx*>(GetControl(CONTROL_DEFAULT_SPIN));
  CGUIRadioButtonControl *pOriginalRadioButton = dynamic_cast<CGUIRadioButtonControl *>(GetControl(CONTROL_DEFAULT_RADIOBUTTON));
  CGUIButtonControl *pOriginalButton = dynamic_cast<CGUIButtonControl *>(GetControl(CONTROL_DEFAULT_BUTTON));
  CGUIImage *pOriginalImage = dynamic_cast<CGUIImage *>(GetControl(CONTROL_DEFAULT_SEPARATOR));
  CGUILabelControl *pOriginalLabel = dynamic_cast<CGUILabelControl *>(GetControl(CONTROL_DEFAULT_LABEL_SEPARATOR));
  CGUISettingsSliderControl *pOriginalSlider = dynamic_cast<CGUISettingsSliderControl *>(GetControl(CONTROL_DEFAULT_SLIDER));

  if (!m_addon || !pOriginalSpin || !pOriginalRadioButton || !pOriginalButton || !pOriginalImage
               || !pOriginalLabel || !pOriginalSlider)
    return;

  pOriginalSpin->SetVisible(false);
  pOriginalRadioButton->SetVisible(false);
  pOriginalButton->SetVisible(false);
  pOriginalImage->SetVisible(false);
  pOriginalLabel->SetVisible(false);
  pOriginalSlider->SetVisible(false);

  CGUIControlGroupList *group = dynamic_cast<CGUIControlGroupList *>(GetControl(CONTROL_SETTINGS_AREA));
  if (!group)
    return;

  // set our dialog heading
  SET_CONTROL_LABEL(CONTROL_HEADING_LABEL, m_strHeading);

  CGUIControl* pControl = NULL;
  int controlId = CONTROL_START_SETTING;
  const TiXmlElement *setting = GetFirstSetting();
  while (setting)
  {
    const std::string       type = XMLUtils::GetAttribute(setting, "type");
    const std::string         id = XMLUtils::GetAttribute(setting, "id");
    const std::string     values = XMLUtils::GetAttribute(setting, "values");
    const std::string    lvalues = XMLUtils::GetAttribute(setting, "lvalues");
    const std::string    entries = XMLUtils::GetAttribute(setting, "entries");
    const std::string defaultVal = XMLUtils::GetAttribute(setting, "default");
    const std::string subsetting = XMLUtils::GetAttribute(setting, "subsetting");
    const std::string      label = GetString(setting->Attribute("label"), subsetting == "true");

    bool bSort = XMLUtils::GetAttribute(setting, "sort") == "yes";
    if (!type.empty())
    {
      bool isAddonSetting = false;
      if (type == "text"   || type == "ipaddress"
       || type == "number" || type == "video"
       || type == "audio"  || type == "image"
       || type == "folder" || type == "executable"
       || type == "file"   || type == "action"
       || type == "date"   || type == "time"
       || type == "select" || (isAddonSetting = type == "addon"))
      {
        pControl = new CGUIButtonControl(*pOriginalButton);
        if (!pControl) return;
        ((CGUIButtonControl *)pControl)->SetLabel(label);
        if (!id.empty())
        {
          std::string value = m_settings[id];
          m_buttonValues[id] = value;
          // get any option to test for hidden
          const std::string option = XMLUtils::GetAttribute(setting, "option");
          if (option == "urlencoded")
            value = CURL::Decode(value);
          else if (option == "hidden")
          {
            std::string hiddenText;
            hiddenText.append(value.size(), L'*');
            ((CGUIButtonControl *)pControl)->SetLabel2(hiddenText);
          }
          else
          {
            if (isAddonSetting)
              ((CGUIButtonControl *)pControl)->SetLabel2(GetAddonNames(value));
            else if (type == "select" && !lvalues.empty())
            {
              std::vector<std::string> valuesVec = StringUtils::Split(lvalues, '|');
              int selected = atoi(value.c_str());
              if (selected >= 0 && selected < (int)valuesVec.size())
              {
                std::string label = g_localizeStrings.GetAddonString(m_addon->ID(), atoi(valuesVec[selected].c_str()));
                if (label.empty())
                  label = g_localizeStrings.Get(atoi(valuesVec[selected].c_str()));
                ((CGUIButtonControl *)pControl)->SetLabel2(label);
              }
            }
            else
              ((CGUIButtonControl *)pControl)->SetLabel2(value);
          }
        }
        else
          ((CGUIButtonControl *)pControl)->SetLabel2(defaultVal);
      }
      else if (type == "bool" && !id.empty())
      {
        pControl = new CGUIRadioButtonControl(*pOriginalRadioButton);
        if (!pControl) return;
        ((CGUIRadioButtonControl *)pControl)->SetLabel(label);
        ((CGUIRadioButtonControl *)pControl)->SetSelected(m_settings[id] == "true");
      }
      else if ((type == "enum" || type == "labelenum") && !id.empty())
      {
        std::vector<std::string> valuesVec;
        std::vector<std::string> entryVec;

        pControl = new CGUISpinControlEx(*pOriginalSpin);
        if (!pControl) return;
        ((CGUISpinControlEx *)pControl)->SetText(label);

       if (!lvalues.empty())
          StringUtils::Tokenize(lvalues, valuesVec, "|");
        else if (values == "$HOURS")
        {
          for (unsigned int i = 0; i < 24; i++)
          {
            CDateTime time(2000, 1, 1, i, 0, 0);
            valuesVec.push_back(g_infoManager.LocalizeTime(time, TIME_FORMAT_HH_MM_XX));
          }
        }
        else
          StringUtils::Tokenize(values, valuesVec, "|");
        if (!entries.empty())
          StringUtils::Tokenize(entries, entryVec, "|");

        if(bSort && type == "labelenum")
          std::sort(valuesVec.begin(), valuesVec.end(), sortstringbyname());

        for (unsigned int i = 0; i < valuesVec.size(); i++)
        {
          int iAdd = i;
          if (entryVec.size() > i)
            iAdd = atoi(entryVec[i].c_str());
          std::string replace;
          if (!lvalues.empty() && std::all_of(valuesVec[i].begin(), valuesVec[i].end(), ::isdigit))
          {
            replace = g_localizeStrings.GetAddonString(m_addon->ID(), atoi(valuesVec[i].c_str()));
            if (replace.empty())
              replace = g_localizeStrings.Get(atoi(valuesVec[i].c_str()));
            if (replace.empty())
              replace = valuesVec[i];
          }
          else
            replace = valuesVec[i];
          ((CGUISpinControlEx *)pControl)->AddLabel(replace, iAdd);
        }
        if (type == "labelenum")
        { // need to run through all our settings and find the one that matches
          ((CGUISpinControlEx*) pControl)->SetValueFromLabel(m_settings[id]);
        }
        else
          ((CGUISpinControlEx*) pControl)->SetValue(atoi(m_settings[id].c_str()));

      }
      else if (type == "fileenum" && !id.empty())
      {
        pControl = new CGUISpinControlEx(*pOriginalSpin);
        if (!pControl) return;
        ((CGUISpinControlEx *)pControl)->SetText(label);
        ((CGUISpinControlEx *)pControl)->SetFloatValue(1.0f);

        std::vector<std::string> items = GetFileEnumValues(values, XMLUtils::GetAttribute(setting, "mask"), XMLUtils::GetAttribute(setting, "option"));
        for (unsigned int i = 0; i < items.size(); ++i)
        {
          ((CGUISpinControlEx *)pControl)->AddLabel(items[i], i);
          if (StringUtils::EqualsNoCase(items[i], m_settings[id]))
            ((CGUISpinControlEx *)pControl)->SetValue(i);
        }
      }
      // Sample: <setting id="mysettingname" type="rangeofnum" label="30000" rangestart="0" rangeend="100" elements="11" valueformat="30001" default="0" />
      // in strings.xml: <string id="30001">%2.0f mp</string>
      // creates 11 piece, text formated number labels from 0 to 100
      else if (type == "rangeofnum" && !id.empty())
      {
        pControl = new CGUISpinControlEx(*pOriginalSpin);
        if (!pControl)
          return;
        ((CGUISpinControlEx *)pControl)->SetText(label);
        ((CGUISpinControlEx *)pControl)->SetFloatValue(1.0f);

        double rangestart = 0, rangeend = 1;
        setting->Attribute("rangestart", &rangestart);
        setting->Attribute("rangeend", &rangeend);

        int elements = 2;
        setting->Attribute("elements", &elements);

        std::string valueformat;
        if (setting->Attribute("valueformat"))
          valueformat = g_localizeStrings.GetAddonString(m_addon->ID(), atoi(setting->Attribute("valueformat")));
        for (int i = 0; i < elements; i++)
        {
          std::string valuestring;
          if (elements < 2)
            valuestring = StringUtils::Format(valueformat.c_str(), rangestart);
          else
            valuestring = StringUtils::Format(valueformat.c_str(), rangestart+(rangeend-rangestart)/(elements-1)*i);
          ((CGUISpinControlEx *)pControl)->AddLabel(valuestring, i);
        }
        ((CGUISpinControlEx *)pControl)->SetValue(atoi(m_settings[id].c_str()));
      }
      // Sample: <setting id="mysettingname" type="slider" label="30000" range="5,5,60" option="int" default="5"/>
      // to make ints from 5-60 with 5 steps
      else if (type == "slider" && !id.empty())
      {
        pControl = new CGUISettingsSliderControl(*pOriginalSlider);
        if (!pControl) return;
        ((CGUISettingsSliderControl *)pControl)->SetText(label);

        float fMin = 0.0f;
        float fMax = 100.0f;
        float fInc = 1.0f;
        std::vector<std::string> range = StringUtils::Split(XMLUtils::GetAttribute(setting, "range"), ',');
        if (range.size() > 1)
        {
          fMin = (float)atof(range[0].c_str());
          if (range.size() > 2)
          {
            fMax = (float)atof(range[2].c_str());
            fInc = (float)atof(range[1].c_str());
          }
          else
            fMax = (float)atof(range[1].c_str());
        }

        std::string option = XMLUtils::GetAttribute(setting, "option");
        int iType=0;

        if (option.empty() || StringUtils::EqualsNoCase(option, "float"))
          iType = SLIDER_CONTROL_TYPE_FLOAT;
        else if (StringUtils::EqualsNoCase(option, "int"))
          iType = SLIDER_CONTROL_TYPE_INT;
        else if (StringUtils::EqualsNoCase(option, "percent"))
          iType = SLIDER_CONTROL_TYPE_PERCENTAGE;

        ((CGUISettingsSliderControl *)pControl)->SetType(iType);
        ((CGUISettingsSliderControl *)pControl)->SetFloatRange(fMin, fMax);
        ((CGUISettingsSliderControl *)pControl)->SetFloatInterval(fInc);
        ((CGUISettingsSliderControl *)pControl)->SetFloatValue((float)atof(m_settings[id].c_str()));
      }
      else if (type == "lsep")
      {
        pControl = new CGUILabelControl(*pOriginalLabel);
        if (pControl)
          ((CGUILabelControl *)pControl)->SetLabel(label);
      }
      else if (type == "sep")
        pControl = new CGUIImage(*pOriginalImage);
    }

    if (pControl)
    {
      pControl->SetWidth(group->GetWidth());
      pControl->SetVisible(true);
      pControl->SetID(controlId);
      pControl->AllocResources();
      group->AddControl(pControl);
      pControl = NULL;
    }

    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
  EnableControls();
}

std::string CGUIDialogAddonSettings::GetAddonNames(const std::string& addonIDslist) const
{
  std::string retVal;
  std::vector<std::string> addons = StringUtils::Split(addonIDslist, ',');
  for (std::vector<std::string>::const_iterator it = addons.begin(); it != addons.end() ; ++it)
  {
    if (!retVal.empty())
      retVal += ", ";
    AddonPtr addon;
    if (CAddonMgr::GetInstance().GetAddon(*it ,addon))
      retVal += addon->Name();
    else
      retVal += *it;
  }
  return retVal;
}

std::vector<std::string> CGUIDialogAddonSettings::GetFileEnumValues(const std::string &path, const std::string &mask, const std::string &options) const
{
  // Create our base path, used for type "fileenum" settings
  // replace $PROFILE with the profile path of the plugin/script
  std::string fullPath = path;
  if (fullPath.find("$PROFILE") != std::string::npos)
    StringUtils::Replace(fullPath, "$PROFILE", m_addon->Profile());
  else
    fullPath = URIUtils::AddFileToFolder(m_addon->Path(), path);

  bool hideExtensions = StringUtils::EqualsNoCase(options, "hideext");
  // fetch directory
  CFileItemList items;
  if (!mask.empty())
    CDirectory::GetDirectory(fullPath, items, mask, XFILE::DIR_FLAG_NO_FILE_DIRS);
  else
    CDirectory::GetDirectory(fullPath, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS);

  std::vector<std::string> values;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if ((mask == "/" && pItem->m_bIsFolder) || !pItem->m_bIsFolder)
    {
      if (hideExtensions)
        pItem->RemoveExtension();
      values.push_back(pItem->GetLabel());
    }
  }
  return values;
}

// Go over all the settings and set their enabled condition according to the values of the enabled attribute
void CGUIDialogAddonSettings::EnableControls()
{
  int controlId = CONTROL_START_SETTING;
  const TiXmlElement *setting = GetFirstSetting();
  while (setting)
  {
    const CGUIControl* control = GetControl(controlId);
    if (control)
    {
      // set enable status
      const char *enable = setting->Attribute("enable");
      if (enable)
        ((CGUIControl*) control)->SetEnabled(GetCondition(enable, controlId));
      else
        ((CGUIControl*) control)->SetEnabled(true);
      // set visible status
      const char *visible = setting->Attribute("visible");
      if (visible)
        ((CGUIControl*) control)->SetVisible(GetCondition(visible, controlId));
      else
        ((CGUIControl*) control)->SetVisible(true);
    }
    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
}

bool CGUIDialogAddonSettings::GetCondition(const std::string &condition, const int controlId)
{
  if (condition.empty()) return true;

  bool bCondition = true;
  bool bCompare = true;
  bool bControlDependend = false;//flag if the condition depends on another control
  std::vector<std::string> conditionVec;

  if (condition.find("+") != std::string::npos)
    StringUtils::Tokenize(condition, conditionVec, "+");
  else
  {
    bCondition = false;
    bCompare = false;
    StringUtils::Tokenize(condition, conditionVec, "|");
  }

  for (unsigned int i = 0; i < conditionVec.size(); i++)
  {
    std::vector<std::string> condVec;
    if (!TranslateSingleString(conditionVec[i], condVec)) continue;

    const CGUIControl* control2 = GetControl(controlId + atoi(condVec[1].c_str()));
    if (!control2)
      continue;
      
    bControlDependend = true; //once we are here - this condition depends on another control

    std::string value;
    switch (control2->GetControlType())
    {
      case CGUIControl::GUICONTROL_BUTTON:
        value = ((CGUIButtonControl*) control2)->GetLabel2();
        break;
      case CGUIControl::GUICONTROL_RADIO:
        value = ((CGUIRadioButtonControl*) control2)->IsSelected() ? "true" : "false";
        break;
      case CGUIControl::GUICONTROL_SPINEX:
        if (((CGUISpinControlEx*) control2)->GetFloatValue() > 0.0f)
          value = ((CGUISpinControlEx*) control2)->GetLabel();
        else
          value = StringUtils::Format("%i", ((CGUISpinControlEx*) control2)->GetValue());
        break;
      default:
        break;
    }

    if (condVec[0] == "eq")
    {
      if (bCompare)
        bCondition &= StringUtils::EqualsNoCase(value, condVec[2]);
      else
        bCondition |= StringUtils::EqualsNoCase(value, condVec[2]);
    }
    else if (condVec[0] == "!eq")
    {
      if (bCompare)
        bCondition &= !StringUtils::EqualsNoCase(value, condVec[2]);
      else
        bCondition |= !StringUtils::EqualsNoCase(value, condVec[2]);
    }
    else if (condVec[0] == "gt")
    {
      if (bCompare)
        bCondition &= (atoi(value.c_str()) > atoi(condVec[2].c_str()));
      else
        bCondition |= (atoi(value.c_str()) > atoi(condVec[2].c_str()));
    }
    else if (condVec[0] == "lt")
    {
      if (bCompare)
        bCondition &= (atoi(value.c_str()) < atoi(condVec[2].c_str()));
      else
        bCondition |= (atoi(value.c_str()) < atoi(condVec[2].c_str()));
    }
  }
  
  if (!bControlDependend)//if condition doesn't depend on another control - try if its an infobool expression
  {
    bCondition = g_infoManager.EvaluateBool(condition);
  }
  
  return bCondition;
}

bool CGUIDialogAddonSettings::TranslateSingleString(const std::string &strCondition, std::vector<std::string> &condVec)
{
  std::string strTest = strCondition;
  StringUtils::ToLower(strTest);
  StringUtils::Trim(strTest);

  size_t pos1 = strTest.find("(");
  size_t pos2 = strTest.find(",", pos1);
  size_t pos3 = strTest.find(")", pos2);
  if (pos1 != std::string::npos &&
      pos2 != std::string::npos &&
      pos3 != std::string::npos)
  {
    condVec.push_back(strTest.substr(0, pos1));
    condVec.push_back(strTest.substr(pos1 + 1, pos2 - pos1 - 1));
    condVec.push_back(strTest.substr(pos2 + 1, pos3 - pos2 - 1));
    return true;
  }
  return false;
}

std::string CGUIDialogAddonSettings::GetString(const char *value, bool subSetting) const
{
  if (!value)
    return "";
  std::string prefix(subSetting ? "- " : "");
  if (StringUtils::IsNaturalNumber(value))
    return prefix + g_localizeStrings.GetAddonString(m_addon->ID(), atoi(value));
  return prefix + value;
}

// Go over all the settings and set their default values
void CGUIDialogAddonSettings::SetDefaultSettings()
{
  if(!m_addon)
    return;

  const TiXmlElement *category = m_addon->GetSettingsXML()->FirstChildElement("category");
  if (!category) // add a default one...
    category = m_addon->GetSettingsXML();

  while (category)
  {
    const TiXmlElement *setting = category->FirstChildElement("setting");
    while (setting)
    {
      const std::string   id = XMLUtils::GetAttribute(setting, "id");
      const std::string type = XMLUtils::GetAttribute(setting, "type");
      const char *value = setting->Attribute("default");
      if (!id.empty())
      {
        if (value)
          m_settings[id] = value;
        else if (type == "bool")
          m_settings[id] = "false";
        else if (type == "slider" || type == "enum")
          m_settings[id] = "0";
        else
          m_settings[id] = "";
      }
      setting = setting->NextSiblingElement("setting");
    }
    category = category->NextSiblingElement("category");
  }
  CreateControls();
}

const TiXmlElement *CGUIDialogAddonSettings::GetFirstSetting() const
{
  const TiXmlElement *category = m_addon->GetSettingsXML()->FirstChildElement("category");
  if (!category)
    category = m_addon->GetSettingsXML();
  for (unsigned int i = 0; i < m_currentSection && category; i++)
    category = category->NextSiblingElement("category");
  if (category)
    return category->FirstChildElement("setting");
  return NULL;
}

void CGUIDialogAddonSettings::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // update status of current section button
  bool alphaFaded = false;
  CGUIControl *control = GetFirstFocusableControl(CONTROL_START_SECTION + m_currentSection);
  if (control && !control->HasFocus())
  {
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetAlpha(0x80);
      alphaFaded = true;
    }
    else if (control->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetSelected(true);
      alphaFaded = true;
    }
  }
  CGUIDialogBoxBase::DoProcess(currentTime, dirtyregions);
  if (alphaFaded && m_active) // dialog may close
  {
    control->SetFocus(false);
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      ((CGUIButtonControl *)control)->SetAlpha(0xFF);
    else
      ((CGUIButtonControl *)control)->SetSelected(false);
  }
}

std::string CGUIDialogAddonSettings::GetCurrentID() const
{
  if (m_addon)
    return m_addon->ID();
  return "";
}

int CGUIDialogAddonSettings::GetDefaultLabelID(int controlId) const
{
  if (controlId == ID_BUTTON_OK)
    return 186;
  else if (controlId == ID_BUTTON_CANCEL)
    return 222;
  else if (controlId == ID_BUTTON_DEFAULT)
    return 409;

  return CGUIDialogBoxBase::GetDefaultLabelID(controlId);
}
