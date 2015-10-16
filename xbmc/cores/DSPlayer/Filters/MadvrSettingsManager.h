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
#include "MadvrCallback.h"
#include "MadvrSettings.h"


class CMadvrSettingsList
{
public:
  CMadvrSettingsList(std::string name, int label, int id = -1):m_name(name), m_label(label), m_id(id) {};
  virtual ~CMadvrSettingsList(){};

  std::string m_name;
  int m_label;
  int m_id;
};

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

class CMadvrSettingsManager :public IMadvrSettingCallback
{
public:

  CMadvrSettingsManager(IUnknown* pUnk);
  virtual ~CMadvrSettingsManager();

  // IMadvrSettingCallback
  virtual void LoadSettings(MADVR_LOAD_TYPE type);
  virtual void RestoreSettings();
  virtual void GetProfileActiveName(std::string *profile);
  virtual void SetStr(std::string path, std::string str);
  virtual void SetBool(std::string path, bool bValue);
  virtual void SetInt(std::string path, int iValue);
  virtual void SetFloat(std::string path, float fValue, int iConv = 100);
  virtual void SetDoubling(std::string path, int iValue);
  virtual void SetDeintActive(std::string path, int iValue);
  virtual void SetBoolValue(std::string path, std::string sValue, int iValue);
  virtual void SetMultiBool(std::string path, std::string sValue, int iValue);
  virtual void SetSmoothmotion(std::string path, int iValue);
  virtual void SetDithering(std::string path, int iValue);
  virtual std::string GetSettingsName(MADVR_SETTINGS_LIST type, int iValue);
  virtual void AddEntry(MADVR_SETTINGS_LIST type, StaticIntegerSettingOptions *entry);
  virtual void UpdateImageDouble();

private:

  BOOL GetSettings(MADVR_SETTINGS_TYPE type, LPCWSTR path, int enumIndex, LPCWSTR sValue, BOOL* bValue, int* iValue, int *bufSize);
  BOOL GetSettings2(MADVR_SETTINGS_TYPE mType, LPCWSTR path, int enumIndex, LPCWSTR id, LPCWSTR type, LPCWSTR name, int *idBufSize, int *nameBufSize, int *typeBufSize);
  BOOL SetSettings(MADVR_SETTINGS_TYPE type, LPCWSTR path, LPCWSTR sValue, BOOL bValue, int iValue);
  void EnumProfilesGroups(MADVR_SETTINGS_TYPE type, std::string path, std::vector<std::string> *sVector);
  void EnumFoldersValues(MADVR_SETTINGS_TYPE type, std::string path, std::vector<std::string> *sVectorId, std::vector<std::string> *sVectorName, std::vector<std::string> *sVectorType);
  
  void EnumGroups(std::string path, std::vector<std::string> *sVector);
  void EnumProfiles(std::string path, std::vector<std::string> *sVector);
  void EnumFolders(std::string path, std::vector<std::string> *sVectorId, std::vector<std::string> *sVectorName, std::vector<std::string> *sVectorType);
  void EnumValues(std::string path, std::vector<std::string> *sVectorId, std::vector<std::string> *sVectorName, std::vector<std::string> *sVectorType);
  void ListSettings(std::string path);

  void GetStr(std::string path, std::string *sValue);
  void GetStr(std::string path, int *iValue, MADVR_SETTINGS_LIST type);
  void GetBool(std::string path, bool *bValue);
  void GetInt(std::string path, int *iValue);
  void GetFloat(std::string path, float* fValue, int iConv = 100);
  void GetDoubling(std::string path, int* iValue);
  void GetDeintActive(std::string path, int* iValue);
  void GetBoolValue(std::string path, std::string sValue, int* iValue);
  void GetMultiBool(std::string path, std::string sValue, int* iValue);
  void GetSmoothmotion(std::string path, int* iValue);
  void GetDithering(std::string path, int* iValue);
  bool IsProfileActive(std::string path, std::string profile);
  void CreateProfile(std::string path, std::string pageList, std::string profileGroup, std::string profile);
  void ActivateProfile(std::string path, std::string profile);

  void AddSettingsListScaler(std::string name, int label, int id, bool chromaUp, bool lumaUp, bool lumaDown);
  void AddSettingsList(MADVR_SETTINGS_LIST type, std::string name, int label, int id);
  std::vector<CMadvrSettingsList*>* GetSettingsVector(MADVR_SETTINGS_LIST type);
  int GetSettingsId(MADVR_SETTINGS_LIST type, std::string sValue);
  void InitSettings();

  bool IsNNEDI3(int iValue) { return iValue < 5; }
  bool IsEnabled(int iValue) { return iValue > -1; }

  std::vector<CMadvrSettingsList* > m_settingsChromaUp;
  std::vector<CMadvrSettingsList* > m_settingsLumaUp;
  std::vector<CMadvrSettingsList* > m_settingsLumaDown;
  std::vector<CMadvrSettingsList* > m_settingsDoubleQuality;
  std::vector<CMadvrSettingsList* > m_settingsDoubleFactor;
  std::vector<CMadvrSettingsList* > m_settingsQuadrupleFactor;
  std::vector<CMadvrSettingsList* > m_settingsDeintForce;
  std::vector<CMadvrSettingsList* > m_settingsDeintActive;
  std::vector<CMadvrSettingsList* > m_settingsNoSmallScaling;
  std::vector<CMadvrSettingsList* > m_settingsMoveSubs;
  std::vector<CMadvrSettingsList* > m_settingsSmoothMotion;
  std::vector<CMadvrSettingsList* > m_settingsDithering;
  std::vector<CMadvrSettingsList* > m_settingsDeband;

  static const std::string DSPROFILE;
  static const std::string DSGROUP;

  IUnknown* m_pDXR;
};