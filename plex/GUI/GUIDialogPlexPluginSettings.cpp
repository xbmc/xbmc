//
//  GUIDialogPlexPluginSettings.cpp
//  Plex
//
//  Created by Jamie Kirkpatrick on 03/02/2011.
//  Copyright 2011 Kirk Consulting Limited. All rights reserved.
//

#include "Application.h"
#include "GUIControlGroupList.h"
#include "GUIDialogPlexPluginSettings.h"
#include "GUIDialogOK.h"
#include "GUIKeyboardFactory.h"
#include "GUIDialogNumeric.h"
#include "GUIImage.h"
#include "GUILabelControl.h"
#include "GUIRadioButtonControl.h"
#include "GUISpinControlEx.h"
#include "GUIEditControl.h"
#include "Util.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "FileSystem/PlexDirectory.h"
#include "LocalizeStrings.h"
#include "Settings.h"
#include "ApplicationMessenger.h"
#include "utils/XBMCTinyXML.h"

#define CONTROL_AREA                    2
#define CONTROL_DEFAULT_BUTTON          3
#define CONTROL_DEFAULT_RADIOBUTTON     4
#define CONTROL_DEFAULT_SPIN            5
#define CONTROL_DEFAULT_SEPARATOR       6
#define CONTROL_DEFAULT_LABEL_SEPARATOR 7
#define ID_BUTTON_OK                    10
#define ID_BUTTON_CANCEL                11
#define ID_BUTTON_DEFAULT               12
#define CONTROL_HEADING_LABEL           20
#define CONTROL_START_CONTROL           100

using namespace std;
using namespace XFILE;

////////////////////////////////////////////////////////////////////////////////
void CPlexPluginSettings::Set(const CStdString& key, const CStdString& value)
{
  if (key == "") return;
  
  // Try to find the setting and change its value
  if (!m_userXmlDoc.RootElement())
  {
    TiXmlElement node("settings");
    m_userXmlDoc.InsertEndChild(node);
  }
  TiXmlElement *setting = m_userXmlDoc.RootElement()->FirstChildElement("setting");
  while (setting)
  {
    const char *id = setting->Attribute("id");
    if (id && strcmpi(id, key) == 0)
    {
      setting->SetAttribute("value", value.c_str());
      return;
    }
    
    setting = setting->NextSiblingElement("setting");
  }
  
  // Setting not found, add it
  TiXmlElement nodeSetting("setting");
  nodeSetting.SetAttribute("id", key.c_str());
  nodeSetting.SetAttribute("value", value.c_str());
  m_userXmlDoc.RootElement()->InsertEndChild(nodeSetting);
}

////////////////////////////////////////////////////////////////////////////////
CStdString CPlexPluginSettings::Get(const CStdString& key)
{
  if (m_userXmlDoc.RootElement())
  {
    // Try to find the setting and return its value
    TiXmlElement *setting = m_userXmlDoc.RootElement()->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      if (id && strcmpi(id, key) == 0)
        return setting->Attribute("value");
      
      setting = setting->NextSiblingElement("setting");
    }
  }
  
  if (m_pluginXmlDoc.RootElement())
  {
    // Try to find the setting in the plugin and return its default value
    TiXmlElement* setting = m_pluginXmlDoc.RootElement()->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      if (id && strcmpi(id, key) == 0 && setting->Attribute("default"))
        return setting->Attribute("default");
      
      setting = setting->NextSiblingElement("setting");
    }
  }
  
  // Otherwise return empty string
  return "";
}

////////////////////////////////////////////////////////////////////////////////
bool CPlexPluginSettings::Load(TiXmlElement* root)
{
  m_pluginXmlDoc.Clear();
  m_userXmlDoc.Clear();
  
  // This holds the settings.
  TiXmlElement xmlSettingsElement("settings");
  TiXmlNode* pSettings = m_pluginXmlDoc.InsertEndChild(xmlSettingsElement);
  
  // This holds the values.
  TiXmlElement xmlValuesElement("settings");
  TiXmlNode* pValues = m_userXmlDoc.InsertEndChild(xmlValuesElement);
  
  TiXmlElement *setting = root->FirstChildElement("Setting");
  while (setting)
  {
    TiXmlElement xmlSetting("setting");
    TiXmlElement xmlValueSetting("setting");
    
    // Walk through the attributes.
    TiXmlAttribute* attrib = setting->FirstAttribute();
    while (attrib)
    {
      string name = attrib->Name();
      string value = attrib->Value();
      
      if (name == "id" || name == "value")
        xmlValueSetting.SetAttribute(name.c_str(), value.c_str());
      
      if (name != "value")
        xmlSetting.SetAttribute(name.c_str(), value.c_str());
      
      attrib = attrib->Next();
    }
    
    pSettings->InsertEndChild(xmlSetting);
    pValues->InsertEndChild(xmlValueSetting);
    
    setting = setting->NextSiblingElement("Setting");
  }
  
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool CPlexPluginSettings::Save(const CStdString& path)
{
  // Build up URL parameters with id and value.
  TiXmlElement* root = m_userXmlDoc.RootElement();
  CURL postURL(path);
  
  for (TiXmlElement* setting = root->FirstChildElement("setting"); setting; setting = setting->NextSiblingElement("setting"))
  {
    const char* id = setting->Attribute("id");
    const char* value = setting->Attribute("value");
    
    if (id && value && strlen(value) > 0)
      postURL.SetOption(id, value);
  }
  
  // Compute the new path.
  PlexUtils::AppendPathToURL(postURL, "set");
  
  // Send the parameters back to the Plex Media Server.
  CFileItemList fileItems;
  CPlexDirectory plexDir;
  
  if (plexDir.GetDirectory(postURL, fileItems))
  {
    // Display a message if there is one.
    if (fileItems.m_displayMessage)
      CGUIDialogOK::ShowAndGetInput(fileItems.m_displayMessageTitle, fileItems.m_displayMessageContents, "", "");
    
    return true;
  }
  
  return false;
}

////////////////////////////////////////////////////////////////////////////////
TiXmlElement* CPlexPluginSettings::GetPluginRoot()
{
  return m_pluginXmlDoc.RootElement();
}

////////////////////////////////////////////////////////////////////////////////
CGUIDialogPlexPluginSettings::CGUIDialogPlexPluginSettings()
  : CGUIDialogBoxBase(WINDOW_PLUGIN_SETTINGS, "DialogPluginSettings.xml")
  , m_okSelected(false)
{}

////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexPluginSettings::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialogBoxBase::OnMessage(message);
      FreeControls();
      CreateControls();
      return true;
    }
      
    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      bool bCloseDialog = false;
      
      if (iControl == ID_BUTTON_OK)
      {
        m_okSelected = true;
        SaveSettings();
      }
      else if (iControl == ID_BUTTON_DEFAULT)
        SetDefaults();
      
      if (iControl == ID_BUTTON_OK || iControl == ID_BUTTON_CANCEL || bCloseDialog)
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

////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexPluginSettings::ShowAndGetInput(const CStdString& path, const CStdString& compositeXml)
{
  // Parse the settings XML.
  TiXmlDocument doc;
  doc.Parse(compositeXml.c_str());
  
  // Get the root element.
  TiXmlElement* root = doc.RootElement();
  if (root == 0)
    return;
  
  // Load the settings.
  CPlexPluginSettings* settings = new CPlexPluginSettings;
  settings->Load(root);

  CGUIDialogPlexPluginSettings* pDialog = NULL;
  pDialog = (CGUIDialogPlexPluginSettings*)g_windowManager.GetWindow(WINDOW_PLUGIN_SETTINGS);
  if (!pDialog)
    return;

  pDialog->m_settings = settings;
  /* FIXME */
  //pDialog->SetHeading(CVariant(root->Attribute("title")));

  pDialog->DoModal();
  
  if (pDialog->m_okSelected)
    settings->Save(path);
}

////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexPluginSettings::ShowVirtualKeyboard(int iControl)
{
  int controlId = CONTROL_START_CONTROL;
  bool bCloseDialog = false;
  
  TiXmlElement *setting = m_settings->GetPluginRoot()->FirstChildElement("setting");
  while (setting)
  {
    if (controlId == iControl)
    {
      const CGUIControl* control = GetControl(controlId);
      if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      {
        const char *type = setting->Attribute("type");
        const char *option = setting->Attribute("option");
        CStdString value = ((CGUIButtonControl*) control)->GetLabel2();
        
        if (strcmp(type, "text") == 0)
        {
          // get any options
          bool bHidden = false;
          if (option)
            bHidden = (strcmp(option, "hidden") == 0);
          
          if (CGUIKeyboardFactory::ShowAndGetInput(value, ((CGUIButtonControl*) control)->GetLabel(), true, bHidden))
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
        else if (strcmpi(type, "action") == 0)
        {
          if (setting->Attribute("default"))
          {
            if (option)
              bCloseDialog = (strcmpi(option, "close") == 0);
            CApplicationMessenger::Get().ExecBuiltIn(setting->Attribute("default"));
          }
        }
        break;
      }
    }
    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
  EnableControls();
  return bCloseDialog;
}

////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexPluginSettings::SaveSettings(void)
{
  // Retrieve all the values from the GUI components and put them in the model
  int controlId = CONTROL_START_CONTROL;
  TiXmlElement *setting = m_settings->GetPluginRoot()->FirstChildElement("setting");
  while (setting)
  {
    CStdString id;
    if (setting->Attribute("id"))
      id = setting->Attribute("id");
    const char *type = setting->Attribute("type");
    
    // skip type "lsep", it is not a required control
    if (strcmpi(type, "lsep") != 0)
    {
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
          if (strcmpi(type, "fileenum") == 0 || strcmpi(type, "labelenum") == 0)
            value = ((CGUISpinControlEx*) control)->GetLabel();
          else
            value.Format("%i", ((CGUISpinControlEx*) control)->GetValue());
          break;
        default:
          break;
      }
      m_settings->Set(id, value);
    }
    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
  return true;
}


////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexPluginSettings::FreeControls()
{
  // clear the category group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(CONTROL_AREA);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
}

////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexPluginSettings::CreateControls()
{
  CGUISpinControlEx *pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  CGUIRadioButtonControl *pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  CGUIButtonControl *pOriginalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  CGUILabelControl *pOriginalLabel = (CGUILabelControl *)GetControl(CONTROL_DEFAULT_LABEL_SEPARATOR);
  
  if (!pOriginalSpin || !pOriginalRadioButton || !pOriginalButton)
    return;
  
  pOriginalSpin->SetVisible(false);
  pOriginalRadioButton->SetVisible(false);
  pOriginalButton->SetVisible(false);
  if (pOriginalLabel)
    pOriginalLabel->SetVisible(false);
  
  // clear the category group
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_AREA);
  if (!group)
    return;
  
  // set our dialog heading
  SET_CONTROL_LABEL(CONTROL_HEADING_LABEL, m_strHeading);
  
  CGUIControl* pControl = NULL;
  int controlId = CONTROL_START_CONTROL;
  TiXmlElement *setting = m_settings->GetPluginRoot()->FirstChildElement("setting");
  while (setting)
  {
    const char *type = setting->Attribute("type");
    const char *option = setting->Attribute("option");
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
    CStdString label;
    if (setting->Attribute("label") && atoi(setting->Attribute("label")) > 0)
      label.Format("$LOCALIZE[%s]", setting->Attribute("label"));
    else
      label = setting->Attribute("label");
    
    if (type)
    {
      if (strcmpi(type, "text") == 0 || strcmpi(type, "integer") == 0)
      {
        pControl = new CGUIEditControl(*pOriginalButton);
        if (!pControl) return;
        ((CGUIEditControl *)pControl)->SetLabel(label);
        if (id)
          ((CGUIEditControl *)pControl)->SetLabel2(m_settings->Get(id));
        
        if (option && strcmpi(option, "hidden") == 0)
          ((CGUIEditControl*)pControl)->SetInputType(CGUIEditControl::INPUT_TYPE_PASSWORD, 0);
      }
      else if (strcmpi(type, "bool") == 0)
      {
        pControl = new CGUIRadioButtonControl(*pOriginalRadioButton);
        if (!pControl) return;
        ((CGUIRadioButtonControl *)pControl)->SetLabel(label);
        ((CGUIRadioButtonControl *)pControl)->SetSelected(m_settings->Get(id) == "true");
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
        else
          CUtil::Tokenize(values, valuesVec, "|");
        if (!entries.IsEmpty())
          CUtil::Tokenize(entries, entryVec, "|");
        for (unsigned int i = 0; i < valuesVec.size(); i++)
        {
          int iAdd = i;
          if (entryVec.size() > i)
            iAdd = atoi(entryVec[i]);
          if (!lvalues.IsEmpty())
          {
            CStdString replace = g_localizeStringsTemp.Get(atoi(valuesVec[i]));
            if (replace.IsEmpty())
              replace = g_localizeStrings.Get(atoi(valuesVec[i]));
            ((CGUISpinControlEx *)pControl)->AddLabel(replace, iAdd);
          }
          else
            ((CGUISpinControlEx *)pControl)->AddLabel(valuesVec[i], iAdd);
        }
        if (strcmpi(type, "labelenum") == 0)
        { // need to run through all our settings and find the one that matches
          ((CGUISpinControlEx*) pControl)->SetValueFromLabel(m_settings->Get(id));
        }
        else
          ((CGUISpinControlEx*) pControl)->SetValue(atoi(m_settings->Get(id)));
        
      }
      else if (strcmpi(type, "lsep") == 0 && pOriginalLabel)
      {
        pControl = new CGUILabelControl(*pOriginalLabel);
        if (pControl)
          ((CGUILabelControl *)pControl)->SetLabel(label);
      }
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

////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexPluginSettings::EnableControls()
{
  int controlId = CONTROL_START_CONTROL;
  TiXmlElement *setting = m_settings->GetPluginRoot()->FirstChildElement("setting");
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

////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexPluginSettings::GetCondition(const CStdString &condition, const int controlId)
{
  if (condition.IsEmpty()) return true;
  
  bool bCondition = true;
  bool bCompare = true;
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
  return bCondition;
}

////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexPluginSettings::TranslateSingleString(const CStdString &strCondition, vector<CStdString> &condVec)
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

////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexPluginSettings::SetDefaults()
{
  int controlId = CONTROL_START_CONTROL;
  TiXmlElement *setting = m_settings->GetPluginRoot()->FirstChildElement("setting");
  while (setting)
  {
    const CGUIControl* control = GetControl(controlId);
    if (control)
    {
      CStdString value;
      switch (control->GetControlType())
      {
        case CGUIControl::GUICONTROL_BUTTON:
          if (setting->Attribute("default") && setting->Attribute("id"))
            ((CGUIButtonControl*) control)->SetLabel2(setting->Attribute("default"));
          else
            ((CGUIButtonControl*) control)->SetLabel2("");
          break;
        case CGUIControl::GUICONTROL_RADIO:
          if (setting->Attribute("default"))
            ((CGUIRadioButtonControl*) control)->SetSelected(strcmpi(setting->Attribute("default"), "true") == 0);
          else
            ((CGUIRadioButtonControl*) control)->SetSelected(false);
          break;
        case CGUIControl::GUICONTROL_SPINEX:
        {
          if (setting->Attribute("default"))
          {
            if (strcmpi(setting->Attribute("type"), "fileenum") == 0 || strcmpi(setting->Attribute("type"), "labelenum") == 0)
            { // need to run through all our settings and find the one that matches
              ((CGUISpinControlEx*) control)->SetValueFromLabel(setting->Attribute("default"));
            }
            else
              ((CGUISpinControlEx*) control)->SetValue(atoi(setting->Attribute("default")));
          }
          else
            ((CGUISpinControlEx*) control)->SetValue(0);
        }
          break;
        default:
          break;
      }
    }
    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
  EnableControls();
}

////////////////////////////////////////////////////////////////////////////////
int CGUIDialogPlexPluginSettings::GetDefaultLabelID(int controlId) const
{
  if (controlId == ID_BUTTON_OK)
    return 12321;
  else if (controlId == ID_BUTTON_CANCEL)
    return 222;
  else if (controlId == ID_BUTTON_DEFAULT)
    return 13278;

  return CGUIDialogBoxBase::GetDefaultLabelID(controlId);
}

