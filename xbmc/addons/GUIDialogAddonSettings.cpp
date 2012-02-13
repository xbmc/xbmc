/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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
#include "filesystem/Directory.h"
#include "video/VideoInfoScanner.h"
#include "addons/Scraper.h"
#include "guilib/GUIWindowManager.h"
#include "Application.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "FileItem.h"
#include "settings/Settings.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "dialogs/GUIDialogSelect.h"
#include "GUIWindowAddonBrowser.h"
#include "utils/log.h"

using namespace std;
using namespace ADDON;
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

#define CONTROL_START_SETTING           100
#define CONTROL_START_SECTION           200

CGUIDialogAddonSettings::CGUIDialogAddonSettings()
   : CGUIDialogBoxBase(WINDOW_DIALOG_ADDON_SETTINGS, "DialogAddonSettings.xml")
{
  m_currentSection = 0;
  m_totalSections = 1;
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
      FreeControls();
      FreeSections();
    }
    break;
    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      bool bCloseDialog = false;

      if (iControl == ID_BUTTON_DEFAULT)
        SetDefaults();
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
      CStdString      id = message.GetStringParam(0);
      CStdString value   = message.GetStringParam(1);
      m_settings[id] = value;
      if (GetFocusedControl())
      {
        int iControl = GetFocusedControl()->GetID();
        CreateControls();
        CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(),iControl);
        OnMessage(msg);
      }
      return true;
    }
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

bool CGUIDialogAddonSettings::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_DELETE_ITEM)
  {
    int iControl = GetFocusedControl()->GetID();
    int controlId = CONTROL_START_SETTING;
    const TiXmlElement* setting = GetFirstSetting();
    UpdateFromControls();
    while (setting)
    {
      if (controlId == iControl)
      {
        const char* id = setting->Attribute("id");
        const char* value = setting->Attribute("default");
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

  return CGUIDialogBoxBase::OnAction(action);
}

void CGUIDialogAddonSettings::OnInitWindow()
{
  m_currentSection = 0;
  m_totalSections = 1;
  CreateSections();
  CreateControls();
  CGUIDialogBoxBase::OnInitWindow();
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
bool CGUIDialogAddonSettings::ShowAndGetInput(const AddonPtr &addon, bool saveToDisk /* = true */)
{
  if (!addon)
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
    CStdString heading;
    heading.Format("$LOCALIZE[10004] - %s", addon->Name().c_str()); // "Settings - AddonName"
    pDialog->m_strHeading = heading;

    pDialog->m_changed = false;
    pDialog->m_addon = addon;
    pDialog->m_saveToDisk = saveToDisk;
    pDialog->DoModal();
    ret = true;
  }
  else
  { // addon does not support settings, inform user
    CGUIDialogOK::ShowAndGetInput(24000,0,24030,0);
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
      if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      {
        const char *id = setting->Attribute("id");
        const char *type = setting->Attribute("type");
        const char *option = setting->Attribute("option");
        const char *source = setting->Attribute("source");
        CStdString value = m_buttonValues[id];
        CStdString label = GetString(setting->Attribute("label"));

        if (strcmp(type, "text") == 0)
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
            CURL::Decode(value);

          if (CGUIDialogKeyboard::ShowAndGetInput(value, label, true, bHidden))
          {
            // if hidden hide input
            if (bHidden)
            {
              CStdString hiddenText;
              hiddenText.append(value.size(), L'*');
              ((CGUIButtonControl *)control)->SetLabel2(hiddenText);
            }
            else
              ((CGUIButtonControl*) control)->SetLabel2(value);
            if (bEncoded)
              CURL::Encode(value);
          }
        }
        else if (strcmp(type, "number") == 0 && CGUIDialogNumeric::ShowAndGetNumber(value, label))
        {
          ((CGUIButtonControl*) control)->SetLabel2(value);
        }
        else if (strcmp(type, "ipaddress") == 0 && CGUIDialogNumeric::ShowAndGetIPAddress(value, label))
        {
          ((CGUIButtonControl*) control)->SetLabel2(value);
        }
        else if (strcmpi(type, "select") == 0)
        {
          CGUIDialogSelect *pDlg = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
          if (pDlg)
          {
            pDlg->SetHeading(label.c_str());
            pDlg->Reset();

            vector<CStdString> valuesVec;
            if (setting->Attribute("values"))
              CUtil::Tokenize(setting->Attribute("values"), valuesVec, "|");
            else if (setting->Attribute("lvalues"))
            { // localize
              CUtil::Tokenize(setting->Attribute("lvalues"), valuesVec, "|");
              for (unsigned int i = 0; i < valuesVec.size(); i++)
              {
                CStdString value = m_addon->GetString(atoi(valuesVec[i]));
                if (value.IsEmpty())
                  value = g_localizeStrings.Get(atoi(valuesVec[i]));
                valuesVec[i] = value;
              }
            }
            else if (source)
            {
              valuesVec = GetFileEnumValues(source, setting->Attribute("mask"), setting->Attribute("option"));
            }

            for (unsigned int i = 0; i < valuesVec.size(); i++)
            {
              pDlg->Add(valuesVec[i]);
              if (valuesVec[i].Equals(value))
                pDlg->SetSelected(i); // FIXME: the SetSelected() does not select "i", it always defaults to the first position
            }
            pDlg->DoModal();
            int iSelected = pDlg->GetSelectedLabel();
            if (iSelected >= 0)
            {
              value = valuesVec[iSelected];
              ((CGUIButtonControl*) control)->SetLabel2(value);
            }
          }
        }
        else if (strcmpi(type, "audio") == 0 || strcmpi(type, "video") == 0 ||
          strcmpi(type, "image") == 0 || strcmpi(type, "executable") == 0 ||
          strcmpi(type, "file") == 0 || strcmpi(type, "folder") == 0)
        {
          // setup the shares
          VECSOURCES *shares = NULL;
          if (source && strcmpi(source, "") != 0)
            shares = g_settings.GetSourcesFromType(source);

          VECSOURCES localShares;
          if (!shares)
          {
            VECSOURCES networkShares;
            g_mediaManager.GetLocalDrives(localShares);
            if (!source || strcmpi(source, "local") != 0)
              g_mediaManager.GetNetworkLocations(localShares);
          }
          else // always append local drives
          {
            localShares = *shares;
            g_mediaManager.GetLocalDrives(localShares);
          }

          if (strcmpi(type, "folder") == 0)
          {
            // get any options
            bool bWriteOnly = false;
            if (option)
              bWriteOnly = (strcmpi(option, "writeable") == 0);

            if (CGUIDialogFileBrowser::ShowAndGetDirectory(localShares, label, value, bWriteOnly))
              ((CGUIButtonControl*) control)->SetLabel2(value);
          }
          else if (strcmpi(type, "image") == 0)
          {
            if (CGUIDialogFileBrowser::ShowAndGetImage(localShares, label, value))
              ((CGUIButtonControl*) control)->SetLabel2(value);
          }
          else
          {
            // set the proper mask
            CStdString strMask;
            if (setting->Attribute("mask"))
            {
              strMask = setting->Attribute("mask");
              // convert mask qualifiers
              strMask.Replace("$AUDIO", g_settings.m_musicExtensions);
              strMask.Replace("$VIDEO", g_settings.m_videoExtensions);
              strMask.Replace("$IMAGE", g_settings.m_pictureExtensions);
#if defined(_WIN32_WINNT)
              strMask.Replace("$EXECUTABLE", ".exe|.bat|.cmd|.py");
#else
              strMask.Replace("$EXECUTABLE", "");
#endif
            }
            else
            {
              if (strcmpi(type, "video") == 0)
                strMask = g_settings.m_videoExtensions;
              else if (strcmpi(type, "audio") == 0)
                strMask = g_settings.m_musicExtensions;
              else if (strcmpi(type, "executable") == 0)
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
              vector<CStdString> options;
              StringUtils::SplitString(option, "|", options);
              bUseThumbs = find(options.begin(), options.end(), "usethumbs") != options.end();
              bUseFileDirectories = find(options.begin(), options.end(), "treatasfolder") != options.end();
            }

            if (CGUIDialogFileBrowser::ShowAndGetFile(localShares, strMask, label, value, bUseThumbs, bUseFileDirectories))
              ((CGUIButtonControl*) control)->SetLabel2(value);
          }
        }
        else if (strcmpi(type, "action") == 0)
        {
          CStdString action = setting->Attribute("action");
          if (!action.IsEmpty())
          {
            // replace $CWD with the url of plugin/script
            action.Replace("$CWD", m_addon->Path());
            action.Replace("$ID", m_addon->ID());
            if (option)
              bCloseDialog = (strcmpi(option, "close") == 0);
            g_application.getApplicationMessenger().ExecBuiltIn(action);
          }
        }
        else if (strcmp(type, "date") == 0)
        {
          CDateTime date;
          if (!value.IsEmpty())
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
        else if (strcmp(type, "time") == 0)
        {
          SYSTEMTIME timedate;
          if (!value.IsEmpty())
          {
            // assumes HH:MM
            timedate.wHour = atoi(value.Left(2));
            timedate.wMinute = atoi(value.Right(2));
          }
          if (CGUIDialogNumeric::ShowAndGetTime(timedate, label))
          {
            value.Format("%02d:%02d", timedate.wHour, timedate.wMinute);
            ((CGUIButtonControl*) control)->SetLabel2(value);
          }
        }
        else if (strcmp(type, "addon") == 0)
        {
          const char *strType = setting->Attribute("addontype");
          if (strType)
          {
            CStdStringArray addonTypes;
            StringUtils::SplitString(strType, ",", addonTypes);
            vector<ADDON::TYPE> types;
            for (unsigned int i = 0 ; i < addonTypes.size() ; i++)
            {
              ADDON::TYPE type = TranslateType(addonTypes[i].Trim());
              if (type != ADDON_UNKNOWN)
                types.push_back(type);
            }
            if (types.size() > 0)
            {
              const char *strMultiselect = setting->Attribute("multiselect");
              bool multiSelect = strMultiselect && strcmpi(strMultiselect, "true") == 0;
              if (multiSelect)
              {
                // construct vector of addon IDs (IDs are comma seperated in single string)
                CStdStringArray addonIDs;
                StringUtils::SplitString(value, ",", addonIDs);
                if (CGUIWindowAddonBrowser::SelectAddonID(types, addonIDs, false) == 1)
                {
                  StringUtils::JoinString(addonIDs, ",", value);
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
    CStdString id = setting->Attribute("id");
    const char *type = setting->Attribute("type");
    const CGUIControl* control = GetControl(controlID++);

    if (control)
    {
      CStdString value;
      switch (control->GetControlType())
      {
        case CGUIControl::GUICONTROL_BUTTON:
          value = m_buttonValues[id];
          break;
        case CGUIControl::GUICONTROL_RADIO:
          value = ((CGUIRadioButtonControl*) control)->IsSelected() ? "true" : "false";
          break;
        case CGUIControl::GUICONTROL_SPINEX:
          if (strcmpi(type, "fileenum") == 0 || strcmpi(type, "labelenum") == 0)
            value = ((CGUISpinControlEx*) control)->GetLabel();
          else
            value.Format("%i", ((CGUISpinControlEx*) control)->GetValue());
          break;
        case CGUIControl::GUICONTROL_SETTINGS_SLIDER:
          value.Format("%f", ((CGUISettingsSliderControl *)control)->GetFloatValue());
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

  for (map<CStdString, CStdString>::iterator i = m_settings.begin(); i != m_settings.end(); ++i)
    m_addon->UpdateSetting(i->first, i->second);

  if (m_saveToDisk)
    m_addon->SaveSettings();
}

void CGUIDialogAddonSettings::FreeSections()
{
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_SETTINGS_AREA);
  if (group)
  {
    group->FreeResources();
    group->ClearAll();
  }
  m_settings.clear();
  m_buttonValues.clear();
}

void CGUIDialogAddonSettings::FreeControls()
{
  // clear the category group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(CONTROL_SETTINGS_AREA);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
}

void CGUIDialogAddonSettings::CreateSections()
{
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_SECTION_AREA);
  CGUIButtonControl *originalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_SECTION_BUTTON);
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

    CStdString label = GetString(category->Attribute("label"));
    if (label.IsEmpty())
      label = g_localizeStrings.Get(128);

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
      const char *id = setting->Attribute("id");
      if (id)
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

  CGUISpinControlEx *pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  CGUIRadioButtonControl *pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  CGUIButtonControl *pOriginalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  CGUIImage *pOriginalImage = (CGUIImage *)GetControl(CONTROL_DEFAULT_SEPARATOR);
  CGUILabelControl *pOriginalLabel = (CGUILabelControl *)GetControl(CONTROL_DEFAULT_LABEL_SEPARATOR);
  CGUISettingsSliderControl *pOriginalSlider = (CGUISettingsSliderControl *)GetControl(CONTROL_DEFAULT_SLIDER);

  if (!m_addon || !pOriginalSpin || !pOriginalRadioButton || !pOriginalButton || !pOriginalImage
               || !pOriginalLabel || !pOriginalSlider)
    return;

  pOriginalSpin->SetVisible(false);
  pOriginalRadioButton->SetVisible(false);
  pOriginalButton->SetVisible(false);
  pOriginalImage->SetVisible(false);
  pOriginalLabel->SetVisible(false);
  pOriginalSlider->SetVisible(false);

  // clear the category group
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_SETTINGS_AREA);
  if (!group)
    return;

  // set our dialog heading
  SET_CONTROL_LABEL(CONTROL_HEADING_LABEL, m_strHeading);

  CGUIControl* pControl = NULL;
  int controlId = CONTROL_START_SETTING;
  const TiXmlElement *setting = GetFirstSetting();
  while (setting)
  {
    const char *type = setting->Attribute("type");
    const char *id = setting->Attribute("id");
    CStdString values;
    if (setting->Attribute("values"))
      values = setting->Attribute("values");
    CStdString lvalues;
    if (setting->Attribute("lvalues"))
      lvalues = setting->Attribute("lvalues");
    CStdString entries;
    if (setting->Attribute("entries"))
      entries = setting->Attribute("entries");
    CStdString defaultValue;
    if (setting->Attribute("default"))
      defaultValue= setting->Attribute("default");
    const char *subsetting = setting->Attribute("subsetting");
    CStdString label = GetString(setting->Attribute("label"), subsetting && 0 == strcmpi(subsetting, "true"));

    bool bSort=false;
    const char *sort = setting->Attribute("sort");
    if (sort && (strcmp(sort, "yes") == 0))
      bSort=true;

    if (type)
    {
      bool isAddonSetting = false;
      if (strcmpi(type, "text") == 0 || strcmpi(type, "ipaddress") == 0 ||
        strcmpi(type, "number") == 0 ||strcmpi(type, "video") == 0 ||
        strcmpi(type, "audio") == 0 || strcmpi(type, "image") == 0 ||
        strcmpi(type, "folder") == 0 || strcmpi(type, "executable") == 0 ||
        strcmpi(type, "file") == 0 || strcmpi(type, "action") == 0 ||
        strcmpi(type, "date") == 0 || strcmpi(type, "time") == 0 ||
        strcmpi(type, "select") == 0 || (isAddonSetting = strcmpi(type, "addon") == 0))
      {
        pControl = new CGUIButtonControl(*pOriginalButton);
        if (!pControl) return;
        ((CGUIButtonControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
        ((CGUIButtonControl *)pControl)->SetLabel(label);
        if (id)
        {
          CStdString value = m_settings[id];
          m_buttonValues[id] = value;
          // get any option to test for hidden
          const char *option = setting->Attribute("option");
          if (option && (strstr(option, "urlencoded")))
            CURL::Decode(value);
          if (option && (strstr(option, "hidden")))
          {
            CStdString hiddenText;
            hiddenText.append(value.size(), L'*');
            ((CGUIButtonControl *)pControl)->SetLabel2(hiddenText);
          }
          else
          {
            if (isAddonSetting)
              ((CGUIButtonControl *)pControl)->SetLabel2(GetAddonNames(value));
            else
              ((CGUIButtonControl *)pControl)->SetLabel2(value);
          }
        }
        else
          ((CGUIButtonControl *)pControl)->SetLabel2(defaultValue);
      }
      else if (strcmpi(type, "bool") == 0)
      {
        pControl = new CGUIRadioButtonControl(*pOriginalRadioButton);
        if (!pControl) return;
        ((CGUIRadioButtonControl *)pControl)->SetLabel(label);
        ((CGUIRadioButtonControl *)pControl)->SetSelected(m_settings[id] == "true");
      }
      else if (strcmpi(type, "enum") == 0 || strcmpi(type, "labelenum") == 0)
      {
        vector<CStdString> valuesVec;
        vector<CStdString> entryVec;

        pControl = new CGUISpinControlEx(*pOriginalSpin);
        if (!pControl) return;
        ((CGUISpinControlEx *)pControl)->SetText(label);

        if (!lvalues.IsEmpty())
          CUtil::Tokenize(lvalues, valuesVec, "|");
        else if (values.Equals("$HOURS"))
        {
          for (unsigned int i = 0; i < 24; i++)
          {
            CDateTime time(2000, 1, 1, i, 0, 0);
            valuesVec.push_back(g_infoManager.LocalizeTime(time, TIME_FORMAT_HH_MM_XX));
          }
        }
        else
          CUtil::Tokenize(values, valuesVec, "|");
        if (!entries.IsEmpty())
          CUtil::Tokenize(entries, entryVec, "|");

        if(bSort && strcmpi(type, "labelenum") == 0)
          std::sort(valuesVec.begin(), valuesVec.end(), sortstringbyname());

        for (unsigned int i = 0; i < valuesVec.size(); i++)
        {
          int iAdd = i;
          if (entryVec.size() > i)
            iAdd = atoi(entryVec[i]);
          if (!lvalues.IsEmpty())
          {
            CStdString replace = m_addon->GetString(atoi(valuesVec[i]));
            if (replace.IsEmpty())
              replace = g_localizeStrings.Get(atoi(valuesVec[i]));
            ((CGUISpinControlEx *)pControl)->AddLabel(replace, iAdd);
          }
          else
            ((CGUISpinControlEx *)pControl)->AddLabel(valuesVec[i], iAdd);
        }
        if (strcmpi(type, "labelenum") == 0)
        { // need to run through all our settings and find the one that matches
          ((CGUISpinControlEx*) pControl)->SetValueFromLabel(m_settings[id]);
        }
        else
          ((CGUISpinControlEx*) pControl)->SetValue(atoi(m_settings[id]));

      }
      else if (strcmpi(type, "fileenum") == 0)
      {
        pControl = new CGUISpinControlEx(*pOriginalSpin);
        if (!pControl) return;
        ((CGUISpinControlEx *)pControl)->SetText(label);
        ((CGUISpinControlEx *)pControl)->SetFloatValue(1.0f);

        vector<CStdString> items = GetFileEnumValues(values, setting->Attribute("mask"), setting->Attribute("option"));
        for (unsigned int i = 0; i < items.size(); ++i)
        {
          ((CGUISpinControlEx *)pControl)->AddLabel(items[i], i);
          if (items[i].Equals(m_settings[id]))
            ((CGUISpinControlEx *)pControl)->SetValue(i);
        }
      }
      // Sample: <setting id="mysettingname" type="rangeofnum" label="30000" rangestart="0" rangeend="100" elements="11" valueformat="30001" default="0" />
      // in strings.xml: <string id="30001">%2.0f mp</string>
      // creates 11 piece, text formated number labels from 0 to 100
      else if (strcmpi(type, "rangeofnum") == 0)
      {
        pControl = new CGUISpinControlEx(*pOriginalSpin);
        if (!pControl)
          return;
        ((CGUISpinControlEx *)pControl)->SetText(label);
        ((CGUISpinControlEx *)pControl)->SetFloatValue(1.0f);

        double rangestart = 0;
        if (setting->Attribute("rangestart"))
          rangestart = atof(setting->Attribute("rangestart"));
        double rangeend = 1;
        if (setting->Attribute("rangeend"))
          rangeend = atof(setting->Attribute("rangeend"));
        int elements = 2;
        if (setting->Attribute("elements"))
          elements = atoi(setting->Attribute("elements"));
        CStdString valueformat;
        if (setting->Attribute("valueformat"))
          valueformat = m_addon->GetString(atoi(setting->Attribute("valueformat")));
        for (int i = 0; i < elements; i++)
        {
          CStdString valuestring;
          if (elements < 2)
            valuestring.Format(valueformat.c_str(), rangestart);
          else
            valuestring.Format(valueformat.c_str(), rangestart+(rangeend-rangestart)/(elements-1)*i);
          ((CGUISpinControlEx *)pControl)->AddLabel(valuestring, i);
        }
        ((CGUISpinControlEx *)pControl)->SetValue(atoi(m_settings[id]));
      }
      // Sample: <setting id="mysettingname" type="slider" label="30000" range="5,5,60" option="int" default="5"/>
      // to make ints from 5-60 with 5 steps
      else if (strcmpi(type, "slider") == 0)
      {
        pControl = new CGUISettingsSliderControl(*pOriginalSlider);
        if (!pControl) return;
        ((CGUISettingsSliderControl *)pControl)->SetText(label);

        float fMin = 0.0f;
        float fMax = 100.0f;
        float fInc = 1.0f;
        vector<CStdString> range;
        StringUtils::SplitString(setting->Attribute("range"), ",", range);
        if (range.size() > 1)
        {
          fMin = (float)atof(range[0]);
          if (range.size() > 2)
          {
            fMax = (float)atof(range[2]);
            fInc = (float)atof(range[1]);
          }
          else
            fMax = (float)atof(range[1]);
        }

        CStdString option = setting->Attribute("option");
        int iType=0;

        if (option.size() == 0 || option.CompareNoCase("float") == 0)
          iType = SPIN_CONTROL_TYPE_FLOAT;
        else if (option.CompareNoCase("int") == 0)
          iType = SPIN_CONTROL_TYPE_INT;
        else if (option.CompareNoCase("percent") == 0)
          iType = 0;

        ((CGUISettingsSliderControl *)pControl)->SetType(iType);
        ((CGUISettingsSliderControl *)pControl)->SetFloatRange(fMin, fMax);
        ((CGUISettingsSliderControl *)pControl)->SetFloatInterval(fInc);
        ((CGUISettingsSliderControl *)pControl)->SetFloatValue((float)atof(m_settings[id]));
      }
      else if (strcmpi(type, "lsep") == 0)
      {
        pControl = new CGUILabelControl(*pOriginalLabel);
        if (pControl)
          ((CGUILabelControl *)pControl)->SetLabel(label);
      }
      else if (strcmpi(type, "sep") == 0)
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
    if (controlId >= CONTROL_START_SECTION)
    {
      CLog::Log(LOGERROR, "%s - cannot have more than %d controls per category - simplify your addon!", __FUNCTION__, CONTROL_START_SECTION - CONTROL_START_SETTING);
      break;
    }
  }
  EnableControls();
}

CStdString CGUIDialogAddonSettings::GetAddonNames(const CStdString& addonIDslist) const
{
  CStdString retVal;
  CStdStringArray addons;
  StringUtils::SplitString(addonIDslist, ",", addons);
  for (CStdStringArray::const_iterator it = addons.begin(); it != addons.end() ; it ++)
  {
    if (!retVal.IsEmpty())
      retVal += ", ";
    AddonPtr addon;
    if (CAddonMgr::Get().GetAddon(*it ,addon))
      retVal += addon->Name();
    else
      retVal += *it;
  }
  return retVal;
}

vector<CStdString> CGUIDialogAddonSettings::GetFileEnumValues(const CStdString &path, const CStdString &mask, const CStdString &options) const
{
  // Create our base path, used for type "fileenum" settings
  // replace $PROFILE with the profile path of the plugin/script
  CStdString fullPath = path;
  if (fullPath.Find("$PROFILE") >= 0)
    fullPath.Replace("$PROFILE", m_addon->Profile());
  else
    fullPath = URIUtils::AddFileToFolder(m_addon->Path(), path);

  bool hideExtensions = (options.CompareNoCase("hideext") == 0);
  // fetch directory
  CFileItemList items;
  if (!mask.IsEmpty())
    CDirectory::GetDirectory(fullPath, items, mask, false);
  else
    CDirectory::GetDirectory(fullPath, items, "", false);

  vector<CStdString> values;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if ((mask.Equals("/") && pItem->m_bIsFolder) || !pItem->m_bIsFolder)
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
      if (setting->Attribute("enable"))
        ((CGUIControl*) control)->SetEnabled(GetCondition(setting->Attribute("enable"), controlId));
      else
        ((CGUIControl*) control)->SetEnabled(true);
      // set visible status
      if (setting->Attribute("visible"))
        ((CGUIControl*) control)->SetVisible(GetCondition(setting->Attribute("visible"), controlId));
      else
        ((CGUIControl*) control)->SetVisible(true);
    }
    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
}

bool CGUIDialogAddonSettings::GetCondition(const CStdString &condition, const int controlId)
{
  if (condition.IsEmpty()) return true;

  bool bCondition = true;
  bool bCompare = true;
  bool bControlDependend = false;//flag if the condition depends on another control
  vector<CStdString> conditionVec;

  if (condition.Find("+") >= 0)
    CUtil::Tokenize(condition, conditionVec, "+");
  else
  {
    bCondition = false;
    bCompare = false;
    CUtil::Tokenize(condition, conditionVec, "|");
  }

  for (unsigned int i = 0; i < conditionVec.size(); i++)
  {
    vector<CStdString> condVec;
    if (!TranslateSingleString(conditionVec[i], condVec)) continue;

    const CGUIControl* control2 = GetControl(controlId + atoi(condVec[1]));
    if (!control2)
      continue;
      
    bControlDependend = true; //once we are here - this condition depends on another control

    CStdString value;
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
          value.Format("%i", ((CGUISpinControlEx*) control2)->GetValue());
        break;
      default:
        break;
    }

    if (condVec[0].Equals("eq"))
    {
      if (bCompare)
        bCondition &= value.Equals(condVec[2]);
      else
        bCondition |= value.Equals(condVec[2]);
    }
    else if (condVec[0].Equals("!eq"))
    {
      if (bCompare)
        bCondition &= !value.Equals(condVec[2]);
      else
        bCondition |= !value.Equals(condVec[2]);
    }
    else if (condVec[0].Equals("gt"))
    {
      if (bCompare)
        bCondition &= (atoi(value) > atoi(condVec[2]));
      else
        bCondition |= (atoi(value) > atoi(condVec[2]));
    }
    else if (condVec[0].Equals("lt"))
    {
      if (bCompare)
        bCondition &= (atoi(value) < atoi(condVec[2]));
      else
        bCondition |= (atoi(value) < atoi(condVec[2]));
    }
  }
  
  if (!bControlDependend)//if condition doesn't depend on another control - try if its an infobool expression
  {
    bCondition = g_infoManager.EvaluateBool(condition);
  }
  
  return bCondition;
}

bool CGUIDialogAddonSettings::TranslateSingleString(const CStdString &strCondition, vector<CStdString> &condVec)
{
  CStdString strTest = strCondition;
  strTest.ToLower();
  strTest.TrimLeft(" ");
  strTest.TrimRight(" ");

  int pos1 = strTest.Find("(");
  int pos2 = strTest.Find(",");
  int pos3 = strTest.Find(")");
  if (pos1 >= 0 && pos2 > pos1 && pos3 > pos2)
  {
    condVec.push_back(strTest.Left(pos1));
    condVec.push_back(strTest.Mid(pos1 + 1, pos2 - pos1 - 1));
    condVec.push_back(strTest.Mid(pos2 + 1, pos3 - pos2 - 1));
    return true;
  }
  return false;
}

CStdString CGUIDialogAddonSettings::GetString(const char *value, bool subSetting) const
{
  if (!value)
    return "";
  int id = atoi(value);
  CStdString prefix(subSetting ? "- " : "");
  if (id > 0)
    return prefix + m_addon->GetString(id);
  return prefix + value;
}

// Go over all the settings and set their default values
void CGUIDialogAddonSettings::SetDefaults()
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
      const char *id = setting->Attribute("id");
      const char *type = setting->Attribute("type");
      const char *value = setting->Attribute("default");
      if (id)
      {
        if (value)
          m_settings[id] = value;
        else if (type && 0 == strcmpi(type, "bool"))
          m_settings[id] = "false";
        else if (type && (0 == strcmpi(type, "slider") || 0 == strcmpi(type, "enum")))
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

CStdString CGUIDialogAddonSettings::GetCurrentID() const
{
  if (m_addon)
    return m_addon->ID();
  return "";
}
