/*
*      Copyright (C) 2005-2014 Team XBMC
*      http://xbmc.org
*
*      Copyright (C) 2014-2015 Aracnoz
*      http://github.com/aracnoz/xbmc
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

#pragma once

#include "AllocatorCommon.h"
#include "DSRendererCallback.h"
#include "MadvrSettings.h"

enum MADVR_SETTINGS_TYPE
{
  MADVR_SETTINGS_PROFILES,
  MADVR_SETTINGS_PROFILEGROUPS,
  MADVR_SETTINGS_FOLDERS,
  MADVR_SETTINGS_VALUES,
  MADVR_SETTINGS_STRING,
  MADVR_SETTINGS_BOOL,
  MADVR_SETTINGS_INT
};

class CMadvrEnum
{
public:
  std::string id;
  std::string name;
  std::string type;
};

class CMadvrSettingsManager :public IMadvrSettingCallback
{
public:

  CMadvrSettingsManager(IUnknown* pUnk);
  virtual ~CMadvrSettingsManager();

  // IMadvrSettingCallback
  virtual void LoadSettings(int iSectionId);
  virtual void RestoreSettings();
  virtual void GetProfileActiveName(const std::string &path, std::string *profile);
  virtual void OnSettingChanged(int iSectionId, CSettingsManager* settingsManager, const CSetting *setting);
  virtual void AddDependencies(const std::string &xml, CSettingsManager *settingsManager, CSetting *setting);
  virtual void ListSettings(const std::string &path);

  void SetBool(const std::string &path, bool bValue, bool bNegate = false, const std::string &type = "");
private:

  BOOL GetSettings(MADVR_SETTINGS_TYPE type, LPCWSTR path, int enumIndex, LPCWSTR sValue, BOOL* bValue, int* iValue, int *bufSize);
  BOOL GetSettings2(MADVR_SETTINGS_TYPE mType, LPCWSTR path, int enumIndex, LPCWSTR id, LPCWSTR type, LPCWSTR name, int *idBufSize, int *nameBufSize, int *typeBufSize);
  BOOL SetSettings(MADVR_SETTINGS_TYPE type, LPCWSTR path, LPCWSTR sValue, BOOL bValue, int iValue);
  void EnumProfilesGroups(MADVR_SETTINGS_TYPE type, const std::string &path, std::vector<std::string> *sVector);
  void EnumFoldersValues(MADVR_SETTINGS_TYPE type, const std::string &path, std::vector<CMadvrEnum> *vector);
  
  void EnumGroups(const std::string &path, std::vector<std::string> *sVector);
  void EnumProfiles(const std::string &path, std::vector<std::string> *sVector);
  void EnumFolders(const std::string &path, std::vector<CMadvrEnum> *vector);
  void EnumValues(const std::string &path, std::vector<CMadvrEnum> *vector);

  std::string GetStr(const std::string &path, const std::string &type = "");
  bool        GetBool(const std::string &path, bool bNegate = false, const std::string &type = "");
  int         GetInt(const std::string &path, const std::string &type = "");
  float       GetFloat(const std::string &path, const std::string &format = "%1.2F", const std::string &type = "");
  int         GetBoolInt(const std::string &path, const std::string &path2, bool bNegate, const std::string &type);
  int         GetBoolBool(const std::string &path, const std::string &path2, bool bNegate, const std::string &type);
  std::string GetBoolStr(const std::string &path, const std::string &path2, bool bNegate, const std::string &type);
  std::string GetCustom(const std::string &path, const std::string &type);

  void        SetStr(const std::string &path, const std::string &str, const std::string &type = "");
  void        SetInt(const std::string &path, int iValue, const std::string &type = "");
  void        SetFloat(const std::string &path, float fValue, const std::string &format = "%1.2F", const std::string &type = "");
  void        SetBoolInt(const std::string &path, const std::string &path2, int iValue, bool bNegate, const std::string &type);
  void        SetBoolBool(const std::string &path, const std::string &path2, int iValue, bool bNegate, const std::string &type);
  void        SetBoolStr(const std::string &path, const std::string &path2, const std::string &sValue, bool bNegate, const std::string &type);
  void        SetCustom(const std::string &path, const std::string &sValue, const std::string &type);

  const std::string GetValueForDebug(const std::string &path, const std::string &type);

  bool IsProfileActive(const std::string &path, const std::string &profile);
  void CreateProfile(const std::string &path, const std::string &pageList, const std::string &profileGroup, const std::string &profile);
  void ActivateProfile(const std::string &path, const std::string &profile);

  void UpdateSettings(const std::string &settingId, CSettingsManager* settingsManager);
  void UpdateImageDouble();

  float IntToFloat(int iValue, const std::string &format);
  int FloatToInt(float fValue, const std::string &format);

  bool IsNNEDI3(const std::string &sValue) { return sValue.find("NNEDI3") != std::string::npos; }
  bool IsEnabled(const std::string &sValue) { return sValue != "-1"; }
  std::string FixedStr(const std::string &str);

  IUnknown* m_pDXR;
  bool m_bAllowChanges;
  bool m_bDebug;
};