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

#include "MadvrSettingsManager.h"
#include "mvrInterfaces.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "Utils/Log.h"
#include "Utils/StringUtils.h"

// DSPLAYER DUMMY
#define DSPROFILE                              "DSPlayer Profile"
#define DSGROUP                                "DSPlayer Profile Group"

// DIALOG ID
#define SET_IMAGE_DOUBLE_LUMA                  "madvr.dl"
#define SET_IMAGE_DOUBLE_CHROMA                "madvr.dc"
#define SET_IMAGE_QUADRUPLE_LUMA               "madvr.ql"
#define SET_IMAGE_QUADRUPLE_CHROMA             "madvr.qc"

#define SET_IMAGE_DOUBLE_LUMA_FACTOR           "madvr.nnedidlscalingfactor"
#define SET_IMAGE_DOUBLE_CHROMA_FACTOR         "madvr.nnedidcscalingfactor"
#define SET_IMAGE_QUADRUPLE_LUMA_FACTOR        "madvr.nnediqlscalingfactor"
#define SET_IMAGE_QUADRUPLE_CHROMA_FACTOR      "madvr.nnediqcscalingfactor"

#define SET_MADVR_DEBAND                       "madvr.debandactive"
#define SET_MADVR_DEBANDLEVEL                  "madvr.debandlevel"
#define SET_MADVR_DEBANDFADELEVEL              "madvr.debandfadelevel"

#define SET_ZOOM_DETECTBARS                    "madvr.detectbars"
#define SET_ZOOM_ARCHANGE                      "madvr.archange"
#define SET_ZOOM_QUICKARCHANGE                 "madvr.quickarchange"

// MADVR DEBUG FIXED STRING GET/SET
#define MADVR_DEBUG_SUCCESS                    "*"
#define MADVR_DEBUG_FAILED                     "x"

CMadvrSettingsManager::CMadvrSettingsManager(IUnknown* pUnk)
{
  m_pDXR = pUnk;
  m_bAllowChanges = true;
  CDSRendererCallback::Get()->Register(this);

  m_bDebug = CMediaSettings::GetInstance().GetCurrentMadvrSettings().m_bDebug;
}

CMadvrSettingsManager::~CMadvrSettingsManager()
{
}

BOOL CMadvrSettingsManager::GetSettings(MADVR_SETTINGS_TYPE type, LPCWSTR path, int enumIndex, LPCWSTR sValue, BOOL* bValue, int* iValue, int *bufSize)
{
  if (Com::SmartQIPtr<IMadVRSettings2> pMadvrSettings2 = m_pDXR)
  {
    switch (type)
    {
    case MADVR_SETTINGS_PROFILEGROUPS:
      return pMadvrSettings2->SettingsEnumProfileGroups(path, enumIndex, sValue, bufSize);
    case MADVR_SETTINGS_PROFILES:
      return pMadvrSettings2->SettingsEnumProfiles(path, enumIndex, sValue, bufSize);
    case MADVR_SETTINGS_STRING:
      return pMadvrSettings2->SettingsGetString(path, sValue, bufSize);
    case MADVR_SETTINGS_BOOL:
      return pMadvrSettings2->SettingsGetBoolean(path, bValue);
    case MADVR_SETTINGS_INT:
      return pMadvrSettings2->SettingsGetInteger(path, iValue);
    }
  }
  return FALSE;
}

BOOL CMadvrSettingsManager::GetSettings2(MADVR_SETTINGS_TYPE mType, LPCWSTR path, int enumIndex, LPCWSTR id, LPCWSTR name, LPCWSTR type, int *idBufSize, int *nameBufSize, int *typeBufSize)
{
  if (Com::SmartQIPtr<IMadVRSettings2> pMadvrSettings2 = m_pDXR)
  {
    switch (mType)
    {
    case MADVR_SETTINGS_FOLDERS:
      return pMadvrSettings2->SettingsEnumFolders(path, enumIndex, id, name, type, idBufSize, nameBufSize, typeBufSize);
    case MADVR_SETTINGS_VALUES:
      return pMadvrSettings2->SettingsEnumValues(path, enumIndex, id, name, type, idBufSize, nameBufSize, typeBufSize);
    }
  }
  return FALSE;
}

BOOL CMadvrSettingsManager::SetSettings(MADVR_SETTINGS_TYPE type, LPCWSTR path, LPCWSTR sValue, BOOL bValue, int iValue)
{
  if (Com::SmartQIPtr<IMadVRSettings2> pMadvrSettings2 = m_pDXR)
  {
    switch (type)
    {
    case MADVR_SETTINGS_STRING:
      return pMadvrSettings2->SettingsSetString(path, sValue);
    case MADVR_SETTINGS_BOOL:
      return pMadvrSettings2->SettingsSetBoolean(path, bValue);
    case MADVR_SETTINGS_INT:
      return pMadvrSettings2->SettingsSetInteger(path, iValue);
    }
  }
  return FALSE;
}

void CMadvrSettingsManager::EnumProfilesGroups(MADVR_SETTINGS_TYPE type, const std::string &path, std::vector<std::string> *sVector)
{
  sVector->clear();
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  std::string str;
  wchar_t* buf = NULL;
  int bufSize = 0;
  int enumIndex = 0;

  GetSettings(type, pathW.c_str(), enumIndex, NULL, NULL, NULL, &bufSize);
  while (bufSize > 0)
  {
    buf = new wchar_t[bufSize];
    GetSettings(type, pathW.c_str(), enumIndex, buf, NULL, NULL, &bufSize);
    std::wstring strW(buf);
    g_charsetConverter.wToUTF8(strW, str);
    sVector->push_back(str);
    buf = NULL;
    bufSize = 0;
    enumIndex++;
    GetSettings(type, pathW.c_str(), enumIndex, NULL, NULL, NULL, &bufSize);
  }
}

void CMadvrSettingsManager::EnumFoldersValues(MADVR_SETTINGS_TYPE type, const std::string &path, std::vector<CMadvrEnum> *vector)
{
  vector->clear();
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  std::string strId;
  std::string strName;
  std::string strType;
  wchar_t* idBuf = NULL;
  wchar_t* nameBuf = NULL;
  wchar_t* typeBuf = NULL;
  int idBufSize = 0;
  int nameBufSize = 0;
  int typeBufSize = 0;
  int enumIndex = 0;

  GetSettings2(type, pathW.c_str(), enumIndex, NULL, NULL, NULL, &idBufSize, &nameBufSize, &typeBufSize);
  while (idBufSize > 0)
  {
    idBuf = new wchar_t[idBufSize];
    nameBuf = new wchar_t[nameBufSize];
    typeBuf = new wchar_t[typeBufSize];
    GetSettings2(type, pathW.c_str(), enumIndex, idBuf, nameBuf, typeBuf, &idBufSize, &nameBufSize, &typeBufSize);

    std::wstring strWid(idBuf);
    std::wstring strWname(nameBuf);
    std::wstring strWtype(typeBuf);
    g_charsetConverter.wToUTF8(strWid, strId);
    g_charsetConverter.wToUTF8(strWname, strName);
    g_charsetConverter.wToUTF8(strWtype, strType);
    CMadvrEnum it;
    it.id = strId;
    it.name = strName;
    it.type = strType;
    vector->push_back(it);
    idBuf = NULL;
    nameBuf = NULL;
    typeBuf = NULL;
    idBufSize = 0;
    nameBufSize = 0;
    typeBufSize = 0;
    enumIndex++;
    GetSettings2(type, pathW.c_str(), enumIndex, NULL, NULL, NULL, &idBufSize, &nameBufSize, &typeBufSize);
  }
}

void CMadvrSettingsManager::EnumGroups(const std::string &path, std::vector<std::string> *sVector)
{
  EnumProfilesGroups(MADVR_SETTINGS_PROFILEGROUPS, path, sVector);
}

void CMadvrSettingsManager::EnumProfiles(const std::string &path, std::vector<std::string> *sVector)
{
  EnumProfilesGroups(MADVR_SETTINGS_PROFILES, path, sVector);
}

void CMadvrSettingsManager::EnumFolders(const std::string &path, std::vector<CMadvrEnum> *vector)
{
  EnumFoldersValues(MADVR_SETTINGS_FOLDERS, path, vector);
}

void CMadvrSettingsManager::EnumValues(const std::string &path, std::vector<CMadvrEnum> *vector)
{
  EnumFoldersValues(MADVR_SETTINGS_VALUES, path, vector);
}

void CMadvrSettingsManager::ListSettings(const std::string &path)
{
  std::vector<std::string> paths = StringUtils::Split(path, "|");
  for (const auto &currentPath : paths)
  {
    std::vector<std::string> groups;
    std::vector<std::string> profiles;
    std::vector<CMadvrEnum> folders;
    std::vector<CMadvrEnum> values;

    CLog::Log(LOGDEBUG, "[madVR debug][Path   ] ################################################################");
    CLog::Log(LOGDEBUG, "[madVR debug][Path   ] %s", currentPath.c_str());
    CLog::Log(LOGDEBUG, "[madVR debug][Path   ] ################################################################");

    EnumFolders(currentPath, &folders);
    for (const auto &folder : folders)
    {
      if (folder.type == "profileRoot")
        continue;

      CLog::Log(LOGDEBUG, "[madVR debug][Folder ] --------------------------------------------------------------");
      CLog::Log(LOGDEBUG, "[madVR debug][Folder ] %s - %s %s", folder.id.c_str(), folder.name.c_str(), folder.type.c_str());
      CLog::Log(LOGDEBUG, "[madVR debug][Folder ] --------------------------------------------------------------");

      EnumValues(currentPath + "\\" + folder.id, &values);
      for (const auto &value : values)
      {
        std::string path = currentPath + "\\" + folder.id + "\\" + value.id;
        CLog::Log(LOGDEBUG, "[madVR debug][Value  ] %s = %s (%s)    %s", value.id.c_str(), GetValueForDebug(path, value.type).c_str(), value.type.c_str(), value.name.c_str());
      }
    }

    EnumGroups(currentPath, &groups);
    for (const auto &group : groups)
    {
      CLog::Log(LOGDEBUG, "[madVR debug][Group  ] ==============================================================");
      CLog::Log(LOGDEBUG, "[madVR debug][Group  ] %s", group.c_str());

      EnumProfiles(currentPath + "\\" + group, &profiles);
      for (const auto &profile : profiles)
      {
        CLog::Log(LOGDEBUG, "[madVR debug][Profile] %s", profile.c_str());
        CLog::Log(LOGDEBUG, "[madVR debug][Profile] ==============================================================");

        EnumFolders(currentPath + "\\" + group + "\\" + profile, &folders);
        for (const auto &folder : folders)
        {
          CLog::Log(LOGDEBUG, "[madVR debug][Folder ] --------------------------------------------------------------");
          CLog::Log(LOGDEBUG, "[madVR debug][Folder ] %s - %s %s", folder.id.c_str(), folder.name.c_str(), folder.type.c_str());
          CLog::Log(LOGDEBUG, "[madVR debug][Folder ] --------------------------------------------------------------");

          EnumValues(currentPath + "\\" + group + "\\" + profile + "\\" + folder.id, &values);
          for (const auto &value : values)
          {
            std::string path = currentPath + "\\" + group + "\\" + profile + "\\" + folder.id + "\\" + value.id;
            CLog::Log(LOGDEBUG, "[madVR debug][Value  ] %s = %s (%s)    %s", value.id.c_str(), GetValueForDebug(path, value.type).c_str(), value.type.c_str(), value.name.c_str());
          }
        }
      }
    }
  }
}

const std::string CMadvrSettingsManager::GetValueForDebug(const std::string &path, const std::string &type)
{
  std::string sValue = "";
  if (type == "boolean")
    sValue = StringUtils::Format("%s", GetBool(path, false) ? "true" : "false");
  else if (type == "integer")
    sValue = StringUtils::Format("%i", GetInt(path));
  else if (type == "string")
    sValue = GetStr(path);

  return sValue;
}

std::string CMadvrSettingsManager::GetStr(const std::string &path, const std::string &type /*=""*/)
{
  std::string sValue;
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  wchar_t* buf = NULL;
  int bufSize = 0;
  GetSettings(MADVR_SETTINGS_STRING, pathW.c_str(), 0, NULL, NULL, NULL, &bufSize);
  buf = new wchar_t[bufSize];
  BOOL bResult = GetSettings(MADVR_SETTINGS_STRING, pathW.c_str(), 0, buf, NULL, NULL, &bufSize);
  if (bResult)
  {
    std::wstring strW(buf);
    g_charsetConverter.wToUTF8(strW, sValue);
  }
  if (m_bDebug && !type.empty())
  {
    CLog::Log(LOGDEBUG, "[madVR debug][%s][Get%s] %s = %s (string)", 
      bResult ? MADVR_DEBUG_SUCCESS : MADVR_DEBUG_FAILED, 
      FixedStr(type).c_str(), path.c_str(), sValue.c_str());
  }
  return sValue;
}

bool CMadvrSettingsManager::GetBool(const std::string &path, bool bNegate, const std::string &type /*=""*/)
{
  bool bValue;
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  BOOL b;
  BOOL bResult = GetSettings(MADVR_SETTINGS_BOOL, pathW.c_str(), 0, NULL, &b, NULL, NULL);
  bNegate ? bValue = (b == 0) : bValue = (b != 0);
  if (m_bDebug && !type.empty())
  {
    bool bValDebug = (b != 0);
    CLog::Log(LOGDEBUG, "[madVR debug][%s][Get%s] %s = %s%s (boolean)",
      bResult ? MADVR_DEBUG_SUCCESS : MADVR_DEBUG_FAILED,
      FixedStr(type).c_str(), path.c_str(), bValDebug ? "true" : "false", bNegate ? " (!negate)" : "");
  }
  return bValue;
}

int CMadvrSettingsManager::GetInt(const std::string &path, const std::string &type /*=""*/)
{
  int iValue;
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  BOOL bResult = GetSettings(MADVR_SETTINGS_INT, pathW.c_str(), 0, NULL, NULL, &iValue, NULL);
  if (m_bDebug && !type.empty())
  {
    CLog::Log(LOGDEBUG, "[madVR debug][%s][Get%s] %s = %i (integer)",
      bResult ? MADVR_DEBUG_SUCCESS : MADVR_DEBUG_FAILED,
      FixedStr(type).c_str(), path.c_str(), iValue);
  }
  return iValue;
}

float CMadvrSettingsManager::GetFloat(const std::string &path, const std::string &format, const std::string &type /*=""*/)
{
  float fValue;
  int iValue;
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  BOOL bResult = GetSettings(MADVR_SETTINGS_INT, pathW.c_str(), 0, NULL, NULL, &iValue, NULL);
  if (iValue > 0)
    fValue = IntToFloat(iValue, format);
  else
    fValue = 0.0f;

  if (m_bDebug && !type.empty())
  {
    int iValue = FloatToInt(fValue, format);
    CLog::Log(LOGDEBUG, "[madVR debug][%s][Get%s] %s = %i (%f) (integer)", 
      bResult ? MADVR_DEBUG_SUCCESS : MADVR_DEBUG_FAILED, 
      FixedStr(type).c_str(), path.c_str(), iValue, fValue);
  }
  return fValue;
};

int CMadvrSettingsManager::GetBoolInt(const std::string &path, const std::string &path2, bool bNegate, const std::string &type)
{
  if (GetBool(path, bNegate, type))
    return GetInt(path2, type);

  return -1;
}

int CMadvrSettingsManager::GetBoolBool(const std::string &path, const std::string &path2, bool bNegate, const std::string &type)
{
  if (GetBool(path, bNegate, type))
    return GetBool(path2, false, type) ? 1 : 0;

  return -1;
}

std::string CMadvrSettingsManager::GetBoolStr(const std::string &path, const std::string &path2, bool bNegate, const std::string &type)
{
  if (GetBool(path, bNegate, type))
      return GetStr(path2, type);

  return "-1";
}

std::string CMadvrSettingsManager::GetCustom(const std::string &path, const std::string &type)
{
  if (type == "list_imagedouble")
  {
    std::string strBool = "nnedi" + path + "Enable";
    std::string strInt = "nnedi" + path + "Quality";
    std::string strAlgo = path + "Algo";    
    
    if (GetBool(strBool, false, type))
    {
      std::string sValue = GetStr(strAlgo, type);
     
      if (sValue == "NNEDI3")
      {
        int iValue = GetInt(strInt, type);
        sValue = "NNEDI316";
        if (iValue == 1)
          sValue = "NNEDI332";
        if (iValue == 2)
          sValue = "NNEDI364";
        if (iValue == 3)
          sValue = "NNEDI3128";
        if (iValue == 4)
          sValue = "NNEDI3256";
      }
      return sValue;
    }
  }
  else if (type == "list_quickar")
  {
    int iValue = -1;
    bool bArChange = GetBool("arChange", false, type);
    bool bQuickArChange = GetBool("quickArChange", false, type);
    bool bQuickZoom = GetBool("quickArChangeZoom", false, type);

    if (bQuickArChange && bQuickZoom && !bArChange)
      iValue = GetInt("quickArChangeDelayValue", type);

    if (bQuickArChange && !bQuickZoom && !bArChange)
      iValue = GetInt("quickArChangeZoomValue", type);

    return StringUtils::Format("%i", iValue);
  }
  else if (type == "list_cleanborders")
  {
    int iValue = -1;
    bool bCleanImageBorders = GetBool("cleanImageBorders", false, type);
    bool bCleanBarsBorders = GetBool("cleanBarsBorders", false, type);

    if (bCleanImageBorders)
      iValue = GetInt("cleanImageBordersValue", type) + 10;

    if (bCleanBarsBorders)
      iValue = GetInt("cleanBarsBordersValue", type);

    return StringUtils::Format("%i", iValue);
  }

  return "-1";
}

void CMadvrSettingsManager::SetStr(const std::string &path, const std::string &str, const std::string &type /*=""*/)
{
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  std::wstring strW;
  g_charsetConverter.utf8ToW(str, strW, false);
  BOOL bResult = SetSettings(MADVR_SETTINGS_STRING, pathW.c_str(), strW.c_str(), NULL, NULL);
  if (m_bDebug && !type.empty())
  {
    CLog::Log(LOGDEBUG, "[madVR debug][%s][Set%s] %s = %s (string)", 
      bResult ? MADVR_DEBUG_SUCCESS : MADVR_DEBUG_FAILED,
      FixedStr(type).c_str(), path.c_str(), str.c_str());
  }
}

void CMadvrSettingsManager::SetBool(const std::string &path, bool bValue, bool bNegate, const std::string &type /*=""*/)
{
  BOOL b;
  std::wstring pathW;
  if (bNegate)
    bValue = !bValue;

  bValue ? b = 1 : b = 0;
  g_charsetConverter.utf8ToW(path, pathW, false);
  BOOL bResult = SetSettings(MADVR_SETTINGS_BOOL, pathW.c_str(), NULL, b, NULL);
  if (m_bDebug && !type.empty())
  {
    CLog::Log(LOGDEBUG, "[madVR debug][%s][Set%s] %s = %s%s (boolean)", 
      bResult ? MADVR_DEBUG_SUCCESS : MADVR_DEBUG_FAILED,
      FixedStr(type).c_str(), path.c_str(), bValue ? "true" : "false", bNegate ? " (!negate)" : "");
  }
}

void CMadvrSettingsManager::SetInt(const std::string &path, int iValue, const std::string &type /*=""*/)
{
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  BOOL bResult = SetSettings(MADVR_SETTINGS_INT, pathW.c_str(), NULL, NULL, iValue);
  if (m_bDebug && !type.empty())
  {
    CLog::Log(LOGDEBUG, "[madVR debug][%s][Set%s] %s = %i (integer)",
      bResult ? MADVR_DEBUG_SUCCESS : MADVR_DEBUG_FAILED,
      FixedStr(type).c_str(), path.c_str(), iValue);
  }
}

void CMadvrSettingsManager::SetFloat(const std::string &path, float fValue, const std::string &format, const std::string &type /*=""*/)
{
  int iValue = FloatToInt(fValue, format);
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  BOOL bResult = SetSettings(MADVR_SETTINGS_INT, pathW.c_str(), NULL, NULL, iValue);
  if (m_bDebug && !type.empty())
  {
    CLog::Log(LOGDEBUG, "[madVR debug][%s][Set%s] %s = %i (%f) (integer)", 
      bResult ? MADVR_DEBUG_SUCCESS : MADVR_DEBUG_FAILED,
      FixedStr(type).c_str(), path.c_str(), iValue, fValue);
  }
};

void CMadvrSettingsManager::SetBoolInt(const std::string &path, const std::string &path2, int iValue, bool bNegate, const std::string &type)
{
  SetBool(path, (iValue > -1), bNegate, type);
  if (iValue > -1)
      SetInt(path2, iValue, type);
}

void CMadvrSettingsManager::SetBoolBool(const std::string &path, const std::string &path2, int iValue, bool bNegate, const std::string &type)
{
  SetBool(path, (iValue > -1), bNegate, type);
  if (iValue > -1)
    SetBool(path2, (iValue == 1), false, type);
}

void CMadvrSettingsManager::SetBoolStr(const std::string &path, const std::string &path2, const std::string &sValue, bool bNegate, const std::string &type)
{
  bool b = sValue != "-1";
  SetBool(path, b, bNegate, type);
  if (b)
    SetStr(path2, sValue, type);
}

void CMadvrSettingsManager::SetCustom(const std::string &path, const std::string &sValue, const std::string &type)
{
  if (type == "list_imagedouble")
  {
    std::string strBool = "nnedi" + path + "Enable";
    std::string strInt = "nnedi" + path + "Quality";
    std::string strAlgo = path + "Algo";
    std::string str = sValue;

    bool b = sValue != "-1";
    SetBool(strBool, (b), false, type);
    if (b)
    {
      if (sValue.find("NNEDI3") != std::string::npos)
      {
        int iValue = 0;
        if (sValue == "NNEDI332")
          iValue = 1;
        if (sValue == "NNEDI364")
          iValue = 2;
        if (sValue == "NNEDI3128")
          iValue = 3;
        if (sValue == "NNEDI3256")
          iValue = 4;

        str = "NNEDI3";

        SetInt(strInt, iValue, type);
      }

      SetStr(strAlgo, str, type);
    }
  }
  else if (type == "list_quickar")
  { 
    int iValue = atoi(sValue.c_str());
    bool bQuickZoom = (iValue == 0 || iValue == 25 || iValue == 50 || iValue == 75 || iValue == 100);
    SetBool("quickArChange", (iValue > -1), false, type);
    if (iValue > -1)
    {
      std::string sValue;
      SetBool("quickArChangeZoom", bQuickZoom, false, type);
      bQuickZoom ? sValue = "quickArChangeZoomValue" : sValue = "quickArChangeDelayValue";
      SetInt(sValue, iValue, type);
    }
  }
  else if (type == "list_cleanborders")
  {
    int iValue = atoi(sValue.c_str());
    bool bCleanImageBorders = (iValue > 10);
    bool bCleanBarsBorders = (iValue > -1) && (iValue < 10);

    SetBool("cleanImageBorders", bCleanImageBorders, false, type);
    SetBool("cleanBarsBorders", bCleanBarsBorders, false, type);

    if (bCleanImageBorders)
      SetInt("cleanImageBordersValue", iValue -10, type);

    if (bCleanBarsBorders)
      SetInt("cleanBarsBordersValue", iValue, type);
  }
}

bool CMadvrSettingsManager::IsProfileActive(const std::string &path, const std::string &profile)
{
  bool result = false;
  BOOL b;
  if (Com::SmartQIPtr<IMadVRSettings2> pMadvrSettings2 = m_pDXR)
  {
    std::wstring pathW;
    std::wstring profileW;
    g_charsetConverter.utf8ToW(path, pathW, false);
    g_charsetConverter.utf8ToW(profile, profileW, false);
    b = pMadvrSettings2->SettingsIsProfileActive(pathW.c_str(), profileW.c_str());
    result = b != 0;
  }
  return result;
}

void CMadvrSettingsManager::GetProfileActiveName(const std::string &path, std::string *profile)
{
  std::vector<std::string> groups;
  std::vector<std::string> profiles;

  EnumGroups(path, &groups);
  for (const auto group : groups)
  {
    EnumProfiles(path + "\\" + group, &profiles);
    for (const auto it : profiles)
    {
      if (IsProfileActive(path + "\\" + group, it))
      {
        *profile = it;
        return;
      }
    }
  }
  *profile = "";
}

void CMadvrSettingsManager::CreateProfile(const std::string &path, const std::string &pageList, const std::string &profileGroup, const std::string &profile)
{
  Com::SmartQIPtr<IMadVRSettings2> pMadvrSettings2 = m_pDXR;
  if (pMadvrSettings2 == NULL)
    return;

  bool existProfile;
  std::vector<std::string> groups;
  std::vector<std::string> profiles;
  std::wstring pathW;
  std::wstring newPathW;
  std::wstring profileW;
  std::wstring profileGroupW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  g_charsetConverter.utf8ToW(profile, profileW, false);

  // Enum madVR Groups
  EnumGroups(path, &groups);  
  for (const auto group : groups)
  {
    existProfile = false;

    // Enum madVR profiles and add a DSPlayer profile if don't exist
    EnumProfiles(path + "\\" + group, &profiles);
    for (const auto it : profiles)
    {
      if (it == profile)
      {
        existProfile = true;
        break;
      }
    }
    if (!existProfile)
    {
      g_charsetConverter.utf8ToW(group, profileGroupW, false);
      newPathW = pathW + L"\\" + profileGroupW;
      pMadvrSettings2->SettingsAddProfile(newPathW.c_str(), profileW.c_str());
    }
  }

  // to be sure that all madVR folders are handled by the DSPlayer profiles add a complete DSPlayer Group
  std::wstring pageListW;
  g_charsetConverter.utf8ToW(pageList, pageListW, false);
  g_charsetConverter.utf8ToW(profileGroup, profileGroupW, false);
  pMadvrSettings2->SettingsCreateProfileGroup(pathW.c_str(), pageListW.c_str(), profileGroupW.c_str(), profileW.c_str());
}

void CMadvrSettingsManager::ActivateProfile(const std::string &path, const std::string &profile)
{
  Com::SmartQIPtr<IMadVRSettings2> pMadvrSettings2 = m_pDXR;
  if (pMadvrSettings2 == NULL)
    return;

  std::vector<std::string> groups;
  std::wstring pathW;
  std::wstring newPathW;
  std::wstring profileW;
  std::wstring profileGroupW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  g_charsetConverter.utf8ToW(profile, profileW, false);

  // Enum madVR groups and activate all DSPlayer profiles
  EnumGroups(path, &groups);
  for (const auto &group : groups)
  {
    g_charsetConverter.utf8ToW(group, profileGroupW, false);
    newPathW = pathW + L"\\" + profileGroupW;
    pMadvrSettings2->SettingsActivateProfile(newPathW.c_str(), profileW.c_str());
  }
}

void CMadvrSettingsManager::RestoreSettings()
{
  if (CSettings::GetInstance().GetInt(CSettings::SETTING_DSPLAYER_MANAGEMADVRWITHKODI) != KODIGUI_LOAD_DSPLAYER)
    return;

  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  for (const auto &it : madvrSettings.m_profiles)
  {
    // Create dummy DSPlayer Profile
    CreateProfile(it.first, it.second, DSGROUP, DSPROFILE);

    // Activate dummy DSPlayer Profile
    ActivateProfile(it.first, DSPROFILE);
  }

  for (const auto &section : madvrSettings.m_gui)
  {
    for (const auto &it : madvrSettings.m_gui[section.first])
    {
      if (it->type.find("button_") != std::string::npos)
        continue;

      if (it->type == "list_string")
        SetStr(it->name, madvrSettings.m_db[it->name].asString(), it->type);
      else if (it->type == "list_int")
        SetInt(it->name, madvrSettings.m_db[it->name].asInteger(), it->type);
      else if (it->type == "list_boolint")
        SetBoolInt(it->name, it->value, madvrSettings.m_db[it->name].asInteger(), it->negate, it->type);
      else if (it->type == "list_boolbool")
        SetBoolBool(it->name, it->value, madvrSettings.m_db[it->name].asInteger(), it->negate, it->type);
      else if (it->type == "list_boolstring")
        SetBoolStr(it->name, it->value, madvrSettings.m_db[it->name].asString(), it->negate, it->type);
      else if (it->type == "bool")
        SetBool(it->name, madvrSettings.m_db[it->name].asBoolean(), it->negate, it->type);
      else if (it->type == "float")
        SetFloat(it->name, madvrSettings.m_db[it->name].asFloat(), it->slider->format, it->type);
      else
        SetCustom(it->name, madvrSettings.m_db[it->name].asString(), it->type);
    }
  }
}

void CMadvrSettingsManager::LoadSettings(int iSectionId)
{
  if (CSettings::GetInstance().GetInt(CSettings::SETTING_DSPLAYER_MANAGEMADVRWITHKODI) != KODIGUI_LOAD_MADVR)
    return;

  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  for (const auto &it : madvrSettings.m_gui[iSectionId])
  {
    if (it->type == "list_string")
      madvrSettings.m_db[it->name] = GetStr(it->name, it->type);
    else if (it->type == "list_int")
      madvrSettings.m_db[it->name] = GetInt(it->name, it->type);
    else if (it->type == "list_boolint")
      madvrSettings.m_db[it->name] = GetBoolInt(it->name, it->value, it->negate, it->type);
    else if (it->type == "list_boolbool")
      madvrSettings.m_db[it->name] = GetBoolBool(it->name, it->value, it->negate, it->type);
    else if (it->type == "list_boolstring")
      madvrSettings.m_db[it->name] = GetBoolStr(it->name, it->value, it->negate, it->type);
    else if (it->type == "bool")
      madvrSettings.m_db[it->name] = GetBool(it->name, it->negate, it->type);
    else if (it->type == "float")
      madvrSettings.m_db[it->name] = GetFloat(it->name, it->slider->format, it->type);
    else
      madvrSettings.m_db[it->name] = GetCustom(it->name, it->type);
  }
}

void CMadvrSettingsManager::OnSettingChanged(int iSectionId, CSettingsManager* settingsManager, const CSetting *setting)
{
  if (!m_bAllowChanges)
    return;

  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  const std::string &settingId = setting->GetId();

  auto it = std::find_if(madvrSettings.m_gui[iSectionId].begin(), madvrSettings.m_gui[iSectionId].end(),
    [settingId](const CMadvrListSettings* setting){
    return setting->dialogId == settingId;
  });

  if (it != madvrSettings.m_gui[iSectionId].end())
  { 
    if ((*it)->type == "list_string")
    {
      madvrSettings.m_db[(*it)->name] = static_cast<const CSettingString*>(setting)->GetValue();
      SetStr((*it)->name, madvrSettings.m_db[(*it)->name].asString(), (*it)->type);
    }
    else if ((*it)->type == "list_int")
    {
      madvrSettings.m_db[(*it)->name] = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
      SetInt((*it)->name, madvrSettings.m_db[(*it)->name].asInteger(), (*it)->type);
    }
    else if ((*it)->type == "list_boolint")
    {
      madvrSettings.m_db[(*it)->name] = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
      SetBoolInt((*it)->name, (*it)->value, madvrSettings.m_db[(*it)->name].asInteger(), (*it)->negate, (*it)->type);
    }
    else if ((*it)->type == "list_boolbool")
    {
      madvrSettings.m_db[(*it)->name] = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
      SetBoolBool((*it)->name, (*it)->value, madvrSettings.m_db[(*it)->name].asInteger(), (*it)->negate, (*it)->type);
    }
    else if ((*it)->type == "list_boolstring")
    {
      madvrSettings.m_db[(*it)->name] = static_cast<const CSettingString*>(setting)->GetValue();
      SetBoolStr((*it)->name, (*it)->value, madvrSettings.m_db[(*it)->name].asString(), (*it)->negate, (*it)->type);
    }
    else if ((*it)->type == "bool")
    {
      madvrSettings.m_db[(*it)->name] = static_cast<const CSettingBool*>(setting)->GetValue();
      SetBool((*it)->name, madvrSettings.m_db[(*it)->name].asBoolean(), (*it)->negate, (*it)->type);
    }
    else if ((*it)->type == "float")
    {
      madvrSettings.m_db[(*it)->name] = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
      SetFloat((*it)->name, madvrSettings.m_db[(*it)->name].asFloat(), (*it)->slider->format, (*it)->type);
    }
    else
    {
      madvrSettings.m_db[(*it)->name] = static_cast<const CSettingString*>(setting)->GetValue();
      SetCustom((*it)->name, madvrSettings.m_db[(*it)->name].asString(), (*it)->type);
    }
    
    UpdateSettings(settingId, settingsManager);
  } 
}

void CMadvrSettingsManager::UpdateImageDouble()
{
  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  std::string sDoubleLuma = madvrSettings.m_db["DL"].asString();
  std::string sDoubleChroma = madvrSettings.m_db["DC"].asString();
  std::string sQuadrupleLuma = madvrSettings.m_db["QL"].asString();
  std::string sQuadrupleChroma = madvrSettings.m_db["QC"].asString();
  std::string sDoubleLumaFactor = madvrSettings.m_db["nnediDLScalingFactor"].asString();
  std::string sDoubleChromaFactor = madvrSettings.m_db["nnediDCScalingFactor"].asString();
  std::string sQuadrupleLumaFactor = madvrSettings.m_db["nnediQLScalingFactor"].asString();
  std::string sQuadrupleChromaFactor = madvrSettings.m_db["nnediQCScalingFactor"].asString();

  // Update double factor
  if (!IsNNEDI3(sDoubleLuma)
    || (IsNNEDI3(sDoubleLuma) && sDoubleChromaFactor > sDoubleLumaFactor))
    sDoubleChromaFactor = sDoubleLumaFactor;

  // Update quadruple factor
  if (!IsNNEDI3(sQuadrupleLuma)
    || (IsNNEDI3(sQuadrupleLuma) && sQuadrupleChromaFactor > sQuadrupleLumaFactor))
    sQuadrupleChromaFactor = sQuadrupleLumaFactor;

  // Update double chroma
  if (!IsNNEDI3(sDoubleLuma)
    || (IsNNEDI3(sDoubleLuma) && sDoubleChroma > sDoubleLuma))
    sDoubleChroma = sDoubleLuma;

  // Update quadruple luma
  if (((!IsNNEDI3(sDoubleLuma) && IsNNEDI3(sQuadrupleLuma))
    || (IsNNEDI3(sDoubleLuma) && IsNNEDI3(sQuadrupleLuma) && sQuadrupleLuma > sDoubleLuma)
    || (!IsEnabled(sDoubleLuma)))
    && IsEnabled(sQuadrupleLuma))
    sQuadrupleLuma = sDoubleLuma;

  // Update quadruple chroma
  if (!IsNNEDI3(sQuadrupleLuma)
    || (IsNNEDI3(sQuadrupleLuma) && sQuadrupleChroma > sQuadrupleLuma))
    sQuadrupleChroma = sQuadrupleLuma;

  if (IsNNEDI3(sQuadrupleLuma) && sQuadrupleChroma > sDoubleChroma)
    sQuadrupleChroma = sDoubleChroma;

  // Set New settings in madVR  
  
  SetCustom("DC", sDoubleChroma, "list_imagedouble");
  SetCustom("QL", sQuadrupleLuma, "list_imagedouble");
  SetCustom("QC", sQuadrupleChroma, "list_imagedouble");    
  SetStr("nnediDCScalingFactor", sDoubleChromaFactor, "list_string");
  SetStr("nnediQCScalingFactor", sQuadrupleChromaFactor, "list_string");

  // Set New settings in db
  madvrSettings.m_db["DC"] = sDoubleChroma;
  madvrSettings.m_db["QL"] = sQuadrupleLuma;
  madvrSettings.m_db["QC"] = sQuadrupleChroma;  
  madvrSettings.m_db["nnediDCScalingFactor"] = sDoubleChromaFactor;
  madvrSettings.m_db["nnediQCScalingFactor"] = sQuadrupleChromaFactor;
  
}

void CMadvrSettingsManager::UpdateSettings(const std::string &settingId, CSettingsManager* settingsManager)
{
  if (!m_bAllowChanges)
    return;

  m_bAllowChanges = false;
  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  // UPDATE IMAGE DOUBLE
  if (settingId == SET_IMAGE_DOUBLE_LUMA
    || settingId == SET_IMAGE_DOUBLE_CHROMA
    || settingId == SET_IMAGE_QUADRUPLE_LUMA
    || settingId == SET_IMAGE_QUADRUPLE_CHROMA
    || settingId == SET_IMAGE_DOUBLE_LUMA_FACTOR
    || settingId == SET_IMAGE_DOUBLE_CHROMA_FACTOR
    || settingId == SET_IMAGE_QUADRUPLE_LUMA_FACTOR
    || settingId == SET_IMAGE_QUADRUPLE_CHROMA_FACTOR
    )
  {
    UpdateImageDouble();
    settingsManager->SetString(SET_IMAGE_DOUBLE_CHROMA, madvrSettings.m_db["DC"].asString());
    settingsManager->SetString(SET_IMAGE_QUADRUPLE_LUMA, madvrSettings.m_db["QL"].asString());
    settingsManager->SetString(SET_IMAGE_QUADRUPLE_CHROMA, madvrSettings.m_db["QC"].asString());
    settingsManager->SetString(SET_IMAGE_DOUBLE_CHROMA_FACTOR, madvrSettings.m_db["nnediDCScalingFactor"].asString());
    settingsManager->SetString(SET_IMAGE_QUADRUPLE_CHROMA_FACTOR, madvrSettings.m_db["nnediQCScalingFactor"].asString());
  }

  // UPDATE ZOOM ARCHANGE
  if (settingId == SET_ZOOM_ARCHANGE
    || settingId == SET_ZOOM_DETECTBARS
    || settingId == SET_ZOOM_QUICKARCHANGE
    )
  {
    int iValue = settingsManager->GetInt(SET_ZOOM_ARCHANGE);
    bool bValue = settingsManager->GetBool(SET_ZOOM_DETECTBARS);
    if (iValue != -1 && bValue)
      if (settingsManager->SetInt(SET_ZOOM_QUICKARCHANGE, -1))
        madvrSettings.m_db["quickArChange"] = -1;
  }

  // UPDATE DEBAND
  if (settingId == SET_MADVR_DEBANDLEVEL
    || settingId == SET_MADVR_DEBANDFADELEVEL
    )
  {
    int iValueA = settingsManager->GetInt(SET_MADVR_DEBANDLEVEL);
    int iValueB = settingsManager->GetInt(SET_MADVR_DEBANDFADELEVEL);

    if (iValueB < iValueA)
    {
      if (settingsManager->SetInt(SET_MADVR_DEBANDFADELEVEL, iValueA))
      {
        madvrSettings.m_db["debandFadeLevel"] = iValueA;
        SetInt("debandFadeLevel", iValueA);
      }
    }
  }
  m_bAllowChanges = true;
}

void CMadvrSettingsManager::AddDependencies(const std::string &xml, CSettingsManager *settingsManager, CSetting *setting)
{
  CXBMCTinyXML doc;
  if (!doc.Parse(xml, TIXML_ENCODING_UTF8))
    return;

  TiXmlNode *dependencies = doc.RootElement();
  if (dependencies != NULL)
  {
    SettingDependencies settingDependencies;
    const TiXmlNode *dependencyNode = dependencies->FirstChild(SETTING_XML_ELM_DEPENDENCY);
    while (dependencyNode != NULL)
    {
      CSettingDependency dependency(settingsManager);
      if (dependency.Deserialize(dependencyNode))
        settingDependencies.push_back(dependency);
      else
        CLog::Log(LOGWARNING, "CSetting: error reading <dependency>");

      dependencyNode = dependencyNode->NextSibling(SETTING_XML_ELM_DEPENDENCY);
    }
    setting->SetDependencies(settingDependencies);
  }
}

float CMadvrSettingsManager::IntToFloat(int iValue, const std::string &format)
{
  if (iValue == 0)
    return 0.0f;

  int i = 100;
  if (format == "%1.2f")
    i = 100;
  else if (format == "%1.1f")
    i = 10;
  else if (format == "%1.0f")
    i = 1;

  return (float)iValue / (float)i;
}

int CMadvrSettingsManager::FloatToInt(float fValue, const std::string &format)
{
  int i = 100;
  if (format == "%1.2f")
    i = 100;
  else if (format == "%1.1f")
    i = 10;
  else if (format == "%1.0f")
    i = 1;

  return (int)round(fValue * i);
}

std::string CMadvrSettingsManager::FixedStr(const std::string &str)
{
  std::string sValue = str;
  StringUtils::Replace(sValue, "list_", "");
  sValue.resize(12);
  StringUtils::ToCapitalize(sValue);
  sValue = StringUtils::Format("%-12s", sValue.c_str());
  return sValue;
}