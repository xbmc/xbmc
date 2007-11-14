/*
 *  Copyright (C) 2005-2007 Team XboxMediaCenter
 *  http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogPluginSettings.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogFileBrowser.h"
#include "GUIControlGroupList.h"
#include "Util.h"
#include "MediaManager.h"
#include "GUIRadioButtonControl.h"
#include "GUISpinControlEx.h"

#define CONTROL_AREA                  2
#define CONTROL_DEFAULT_BUTTON        3
#define CONTROL_DEFAULT_RADIOBUTTON   4
#define CONTROL_DEFAULT_SPIN          5
#define CONTROL_DEFAULT_SEPARATOR     6
#define ID_BUTTON_OK                  10
#define ID_BUTTON_CANCEL              11
#define CONTROL_HEADING_LABEL         20
#define CONTROL_START_CONTROL         100

CGUIDialogPluginSettings::CGUIDialogPluginSettings()
   : CGUIDialogBoxBase(WINDOW_DIALOG_PLUGIN_SETTINGS, "DialogPluginSettings.xml")
{}

CGUIDialogPluginSettings::~CGUIDialogPluginSettings(void)
{}

bool CGUIDialogPluginSettings::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);

      m_settings.Load(m_url);

      FreeControls();
      CreateControls();
      break;
    }

    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (iControl == ID_BUTTON_OK)
        SaveSettings();
      else
        ShowVirtualKeyboard(iControl);

      if (iControl == ID_BUTTON_OK || iControl == ID_BUTTON_CANCEL)
      {
        m_bConfirmed = true;
        Close();
        return true;
      }
    }
    break;
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
void CGUIDialogPluginSettings::ShowAndGetInput(CURL& url)
{
  m_url = url;

  // Path where the plugin resides
  CStdString pathToPlugin = "Q:\\plugins\\";
  CUtil::AddFileToFolder(pathToPlugin, url.GetHostName(), pathToPlugin);
  CUtil::AddFileToFolder(pathToPlugin, url.GetFileName(), pathToPlugin);

  // Remove the slash at end, makes xbox and linux compatible
  CUtil::RemoveSlashAtEnd(pathToPlugin);

  // Path where the language strings reside
  CStdString pathToLanguageFile = pathToPlugin;
  CStdString pathToFallbackLanguageFile = pathToPlugin;
  CUtil::AddFileToFolder(pathToLanguageFile, "resources", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "resources", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, "language", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "language", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, g_guiSettings.GetString("locale.language"), pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "english", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, "strings.xml", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "strings.xml", pathToFallbackLanguageFile);

  // Load the strings temporarily
  g_localizeStringsTemp.Load(pathToLanguageFile, pathToFallbackLanguageFile);

  // Create the dialog
  CGUIDialog* pDialog = (CGUIDialog*) m_gWindowManager.GetWindow(WINDOW_DIALOG_PLUGIN_SETTINGS);
  pDialog->DoModal();

  // Unload the temporary strings
  g_localizeStringsTemp.Clear();

  return;
}

void CGUIDialogPluginSettings::ShowVirtualKeyboard(int iControl)
{
  int controlId = CONTROL_START_CONTROL;
  TiXmlElement *setting = m_settings.GetPluginRoot()->FirstChildElement("setting");
  while (setting)
  {
    const char *type = setting->Attribute("type");
    const char *option = setting->Attribute("option");
    const char *source = setting->Attribute("source");
    const CGUIControl* control = GetControl(controlId);
    if (controlId == iControl && control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
    {
      CStdString value = ((CGUIButtonControl*) control)->GetLabel2();

      if (strcmp(type, "text") == 0)
      {
        // get any options
        bool bHidden = false;
        if (option)
          bHidden = (strcmp(option, "hidden") == 0);

        if (CGUIDialogKeyboard::ShowAndGetInput(value, ((CGUIButtonControl*) control)->GetLabel(), true, bHidden))
          ((CGUIButtonControl*) control)->SetLabel2(value);
      }
      else if (strcmp(type, "integer") == 0 && CGUIDialogNumeric::ShowAndGetNumber(value, ((CGUIButtonControl*) control)->GetLabel()))
      {
        ((CGUIButtonControl*) control)->SetLabel2(value);
      }
      else if (strcmp(type, "ipaddress") == 0 && CGUIDialogNumeric::ShowAndGetIPAddress(value, ((CGUIButtonControl*) control)->GetLabel()))
      {
        ((CGUIButtonControl*) control)->SetLabel2(value);
      }
      else if (strcmpi(type, "video") == 0 || strcmpi(type, "music") == 0 ||
        strcmpi(type, "pictures") == 0 || strcmpi(type, "programs") == 0 || 
        strcmpi(type, "folder") == 0 || strcmpi(type, "files") == 0)
      {
        // setup the shares
        VECSHARES *shares = NULL;
        if (!source || strcmpi(source, "") == 0)
          shares = g_settings.GetSharesFromType(type);
        else
          shares = g_settings.GetSharesFromType(source);

        if (!shares)
        {
          VECSHARES localShares, networkShares;
          g_mediaManager.GetLocalDrives(localShares);
          if (!source || strcmpi(source, "local") != 0)
            g_mediaManager.GetNetworkLocations(networkShares);
          localShares.insert(localShares.end(), networkShares.begin(), networkShares.end());
          shares = &localShares;
        }
        
        if (strcmpi(type, "folder") == 0)
        {
          // get any options
          bool bWriteOnly = false;
          if (option)
            bWriteOnly = (strcmpi(option, "writeable") == 0);

          if (CGUIDialogFileBrowser::ShowAndGetDirectory(*shares, ((CGUIButtonControl*) control)->GetLabel(), value, bWriteOnly))
            ((CGUIButtonControl*) control)->SetLabel2(value);
        }
        else if (strcmpi(type, "picture") == 0)
        {
          if (CGUIDialogFileBrowser::ShowAndGetImage(*shares, ((CGUIButtonControl*) control)->GetLabel(), value))
            ((CGUIButtonControl*) control)->SetLabel2(value);
        }
        else
        {
          // set the proper mask
          CStdString strMask;
          if (strcmpi(type, "video") == 0)
            strMask = g_stSettings.m_videoExtensions;
          else if (strcmpi(type, "music") == 0)
            strMask = g_stSettings.m_musicExtensions;
          else if (strcmpi(type, "programs") == 0)
            strMask = ".xbe|.py";

          // get any options
          bool bUseThumbs = false;
          bool bUseFileDirectories = false;
          if (option)
          {
            bUseThumbs = (strcmpi(option, "usethumbs") == 0 || strcmpi(option, "usethumbs|treatasfolder") == 0);
            bUseFileDirectories = (strcmpi(option, "treatasfolder") == 0 || strcmpi(option, "usethumbs|treatasfolder") == 0);
          }

          if (CGUIDialogFileBrowser::ShowAndGetFile(*shares, strMask, ((CGUIButtonControl*) control)->GetLabel(), value))
            ((CGUIButtonControl*) control)->SetLabel2(value);
        }
      }
    }
    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
  EnableControls();
}

// Go over all the settings and set their values according to the values of the GUI components
bool CGUIDialogPluginSettings::SaveSettings(void)
{
  // Retrieve all the values from the GUI components and put them in the model
  int controlId = CONTROL_START_CONTROL;
  TiXmlElement *setting = m_settings.GetPluginRoot()->FirstChildElement("setting");
  while (setting)
  {
    CStdString id = setting->Attribute("id");
    const CGUIControl* control = GetControl(controlId);

    CStdString value;
    switch (control->GetControlType())
    {
      case CGUIControl::GUICONTROL_BUTTON:
        value = ((CGUIButtonControl*) control)->GetLabel2();
        break;
      case CGUIControl::GUICONTROL_RADIO:
        value = ((CGUIRadioButtonControl*) control)->IsSelected() ? "true" : "false";
        break;
      case CGUIControl::GUICONTROL_SPINEX:
        value.Format("%i", ((CGUISpinControlEx*) control)->GetValue());
        break;
      default:
        break;
    }
    m_settings.Set(id, value);

    setting = setting->NextSiblingElement("setting");
    controlId++;
  }

  // Save the model into an XML file
  return m_settings.Save();
}

void CGUIDialogPluginSettings::FreeControls()
{
  // clear the category group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(CONTROL_AREA);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
}

void CGUIDialogPluginSettings::CreateControls()
{
  CGUISpinControlEx *pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  CGUIRadioButtonControl *pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  CGUIButtonControl *pOriginalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  CGUIImage *pOriginalImage = (CGUIImage *)GetControl(CONTROL_DEFAULT_SEPARATOR);

  if (!pOriginalSpin || !pOriginalRadioButton || !pOriginalButton || !pOriginalImage)
    return;

  pOriginalSpin->SetVisible(false);
  pOriginalRadioButton->SetVisible(false);
  pOriginalButton->SetVisible(false);
  pOriginalImage->SetVisible(false);

  // clear the category group
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_AREA);
  if (!group)
    return;

  // set our dialog heading
  CStdString strHeading = m_url.GetFileName();
  CUtil::RemoveSlashAtEnd(strHeading);
  strHeading.Format("$LOCALIZE[1045] - %s", strHeading.c_str());
  SET_CONTROL_LABEL(CONTROL_HEADING_LABEL, strHeading);

  CGUIControl* pControl = NULL;
  int controlId = CONTROL_START_CONTROL;
  TiXmlElement *setting = m_settings.GetPluginRoot()->FirstChildElement("setting");
  while (setting)
  {
    const char *type = setting->Attribute("type");
    const char *id = setting->Attribute("id");
    CStdString values = setting->Attribute("values");
    CStdString lvalues = setting->Attribute("lvalues");

    CStdString label;
    label.Format("$LOCALIZE[%s]", setting->Attribute("label"));

    if (strcmpi(type, "text") == 0 || strcmpi(type, "ipaddress") == 0 ||
      strcmpi(type, "integer") == 0 || strcmpi(type, "video") == 0 ||
      strcmpi(type, "music") == 0 || strcmpi(type, "pictures") == 0 ||
      strcmpi(type, "folder") == 0 || strcmpi(type, "programs") == 0 ||
      strcmpi(type, "files") == 0)
    {
      pControl = new CGUIButtonControl(*pOriginalButton);
      if (!pControl) return;
      ((CGUIButtonControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
      ((CGUIButtonControl *)pControl)->SetLabel(label);
      ((CGUIButtonControl *)pControl)->SetLabel2(m_settings.Get(id));
    }
    else if (strcmpi(type, "bool") == 0)
    {
      pControl = new CGUIRadioButtonControl(*pOriginalRadioButton);
      if (!pControl) return;
      ((CGUIRadioButtonControl *)pControl)->SetLabel(label);
      ((CGUIRadioButtonControl *)pControl)->SetSelected(m_settings.Get(id) == "true");
    }
    else if (strcmpi(type, "enum") == 0)
    {
      vector<CStdString> valuesVec;

      pControl = new CGUISpinControlEx(*pOriginalSpin);
      if (!pControl) return;
      ((CGUISpinControlEx *)pControl)->SetText(label);

      if (!lvalues.IsEmpty())
        CUtil::Tokenize(lvalues, valuesVec, "|");
      else
        CUtil::Tokenize(values, valuesVec, "|");

      for (unsigned int i = 0; i < valuesVec.size(); i++)
      {
        if (!lvalues.IsEmpty())
        {
          CStdString replace = g_localizeStringsTemp.Get(atoi(valuesVec[i]));
          if (replace.IsEmpty())
            replace = g_localizeStrings.Get(atoi(valuesVec[i]));
          ((CGUISpinControlEx *)pControl)->AddLabel(replace, i);
        }
        else
          ((CGUISpinControlEx *)pControl)->AddLabel(valuesVec[i], i);
      }
      ((CGUISpinControlEx *)pControl)->SetValue(atoi(m_settings.Get(id)));
    }
    else if (strcmpi(type, "sep") == 0 && pOriginalImage)
      pControl = new CGUIImage(*pOriginalImage);

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

// Go over all the settings and set their enabled condition according to the values of the enabled attribute
void CGUIDialogPluginSettings::EnableControls()
{
  int controlId = CONTROL_START_CONTROL;
  TiXmlElement *setting = m_settings.GetPluginRoot()->FirstChildElement("setting");
  while (setting)
  {
    CStdString enable = setting->Attribute("enable");
    if (!enable.IsEmpty())
    {
      vector<CStdString> enableCondVec;
      CUtil::Tokenize(enable, enableCondVec, "+");
      
      bool bEnable = true;
      
      const CGUIControl* control = GetControl(controlId);

      for (unsigned int i = 0; i < enableCondVec.size(); i++)
      {
        vector<CStdString> enableVec;
        if (!TranslateSingleString(enableCondVec[i], enableVec)) continue;

        const CGUIControl* control2 = GetControl(controlId + atoi(enableVec[1]));

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
            value.Format("%i", ((CGUISpinControlEx*) control2)->GetValue());
            break;
          default:
            break;
        }
        if (enableVec[0].Equals("eq"))
          bEnable &= value.Equals(enableVec[2]);
        else if (enableVec[0].Equals("!eq"))
          bEnable &= !value.Equals(enableVec[2]);
        else if (enableVec[0].Equals("gt"))
          bEnable &= (atoi(value) > atoi(enableVec[2]));
        else if (enableVec[0].Equals("lt"))
          bEnable &= (atoi(value) < atoi(enableVec[2]));
      }
      ((CGUIButtonControl*) control)->SetEnabled(bEnable);
    }

    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
}

bool CGUIDialogPluginSettings::TranslateSingleString(const CStdString &strCondition, vector<CStdString> &enableVec)
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
    enableVec.push_back(strTest.Left(pos1));
    enableVec.push_back(strTest.Mid(pos1 + 1, pos2 - pos1 - 1));
    enableVec.push_back(strTest.Mid(pos2 + 1, pos3 - pos2 - 1));
    return true;
  }
  return false;
}

CURL CGUIDialogPluginSettings::m_url;
