/*
 *      Copyright (C) 2005-2013 Team XBMC
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
// MadvrSettings.h: interface for the CMadvrSettings class.
//
//////////////////////////////////////////////////////////////////////

//#if !defined(AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
//#define AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#pragma once

#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/StdString.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"

enum MADVR_DEFAULT_SECTION
{
  MADVR_VIDEO_ROOT = 0,
  MADVR_SECTION_ROOT = 1000,
  MADVR_SECTION_SUB = 10
};

class CMadvrSection
{
public:
  int label;
  int parentId;
};

class CMadvrSlider
{
 public:
  std::string format;
  int parentLabel;
  float min,max,step;
};

class CMadvrListSettings
{
public:
  int group = -1;
  std::string name = "";
  std::string type = "";
  std::string dialogId = "";
  std::string value = "";
  std::string dependencies = "";
  std::string parent = "";
  bool negate = false;
  int label = 0;
  CMadvrSlider *slider;
  int sectionId = 0;
  std::vector< std::pair<int, int> > optionsInt;
  std::vector< std::pair<int, std::string> > optionsString;
};

class CMadvrSettings
{
public:
  CMadvrSettings();
  ~CMadvrSettings() {};

  void StoreSettingsAtStart();
  void RestoreDefaultSettings();
  bool SettingsChanged();

  CVariant m_db;
  
  std::map<int, CMadvrSection> m_sections;
  std::map<int, std::vector<CMadvrListSettings*> > m_gui;
  std::map<std::string, std::string> m_profiles;

  int m_Resolution;
  std::string m_TvShowName;
  bool m_bDebug;

private:
  void InitSettings();

  void AddSection(TiXmlNode *pNode, int iSectionId);
  void AddButton(TiXmlNode *pNode, int iSectionId, int iGroupId, int iSubSectionId, const std::string &type, const std::string &name);
  void AddSetting(TiXmlNode *pNode, int iSectionId, int iGroupId);
  void AddProfiles(TiXmlNode *pNode);
  std::string DependenciesNameToId(const std::string &dependencies);
  std::string NameToId(const std::string &str);

  bool GetVariant(TiXmlElement *pElement, const std::string &attr, const std::string &type, CVariant *variant);
  bool GetInt(TiXmlElement *pElement, const std::string &attr, int *iValue);
  bool GetFloat(TiXmlElement *pElement, const std::string &attr, float *fValue);
  bool GetString(TiXmlElement *pElement, const std::string &attr, std::string *sValue);
  
  int m_iSubSectionId;
  std::string m_madvrJsonAtStart;
  CVariant m_dbDefault;
};

//#endif // !defined(AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
