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

CMadvrSettingsManager::CMadvrSettingsManager(IUnknown* pUnk)
{
  m_pDXR = pUnk;
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
      pMadvrSettings2->SettingsEnumProfiles(path, enumIndex, sValue, bufSize);
      return TRUE;
    case MADVR_SETTINGS_STRING:
      return pMadvrSettings2->SettingsGetString(path, sValue, bufSize);
    case MADVR_SETTINGS_BOOL:
      return pMadvrSettings2->SettingsGetBoolean(path, bValue);
    case MADVR_SETTINGS_INT:
      return pMadvrSettings2->SettingsGetInteger(path, iValue);
    }
  }
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
}

void CMadvrSettingsManager::EnumProfilesGroups(MADVR_SETTINGS_TYPE type, std::string path, std::vector<std::string> *sVector)
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

void CMadvrSettingsManager::EnumFoldersValues(MADVR_SETTINGS_TYPE type, std::string path, std::vector<std::string> *sVectorId, std::vector<std::string> *sVectorName, std::vector<std::string> *sVectorType)
{
  sVectorId->clear();
  sVectorName->clear();
  sVectorType->clear();
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
    sVectorId->push_back(strId);
    sVectorName->push_back(strName);
    sVectorType->push_back(strType);
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

void CMadvrSettingsManager::EnumGroups(std::string path, std::vector<std::string> *sVector)
{
  EnumProfilesGroups(MADVR_SETTINGS_PROFILEGROUPS, path, sVector);
}

void CMadvrSettingsManager::EnumProfiles(std::string path, std::vector<std::string> *sVector)
{
  EnumProfilesGroups(MADVR_SETTINGS_PROFILES, path, sVector);
}

void CMadvrSettingsManager::EnumFolders(std::string path, std::vector<std::string> *sVectorId, std::vector<std::string> *sVectorName, std::vector<std::string> *sVectorType)
{
  EnumFoldersValues(MADVR_SETTINGS_FOLDERS, path, sVectorId, sVectorName, sVectorType);
}

void CMadvrSettingsManager::EnumValues(std::string path, std::vector<std::string> *sVectorId, std::vector<std::string> *sVectorName, std::vector<std::string> *sVectorType)
{
  EnumFoldersValues(MADVR_SETTINGS_VALUES, path, sVectorId, sVectorName, sVectorType);
}

void CMadvrSettingsManager::GetStr(std::string path, std::string *str)
{
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  wchar_t* buf = NULL;
  int bufSize = 0;
  GetSettings(MADVR_SETTINGS_STRING, pathW.c_str(), 0, NULL, NULL, NULL, &bufSize);
  buf = new wchar_t[bufSize];
  if (GetSettings(MADVR_SETTINGS_STRING, pathW.c_str(), 0, buf, NULL, NULL, &bufSize))
  {
    std::wstring strW(buf);
    g_charsetConverter.wToUTF8(strW, *str);
  }
}

void CMadvrSettingsManager::GetBool(std::string path, BOOL *bValue)
{
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  GetSettings(MADVR_SETTINGS_BOOL, pathW.c_str(), 0, NULL, bValue, NULL, NULL);
}

void CMadvrSettingsManager::GetInt(std::string path, int* iValue)
{
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  GetSettings(MADVR_SETTINGS_INT, pathW.c_str(), 0, NULL, NULL, iValue, NULL);
}

void CMadvrSettingsManager::SetStr(std::string path, std::string str)
{
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  std::wstring strW;
  g_charsetConverter.utf8ToW(str, strW, false);
  SetSettings(MADVR_SETTINGS_STRING, pathW.c_str(), strW.c_str(), NULL, NULL);
}

void CMadvrSettingsManager::SetBool(std::string path, BOOL bValue)
{
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  SetSettings(MADVR_SETTINGS_BOOL, pathW.c_str(), NULL, bValue, NULL);
}

void CMadvrSettingsManager::SetInt(std::string path, int iValue)
{
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  SetSettings(MADVR_SETTINGS_INT, pathW.c_str(), NULL, NULL, iValue);
}

void CMadvrSettingsManager::ListSettings(std::string path)
{
  std::vector<std::string> vecProfileGroups;
  std::vector<std::string> vecProfileName;
  std::vector<std::string> vecFoldersId;
  std::vector<std::string> vecFoldersName;
  std::vector<std::string> vecFoldersType;
  std::vector<std::string> vecValuesId;
  std::vector<std::string> vecValuesName;
  std::vector<std::string> vecValuesType;

  EnumGroups(path, &vecProfileGroups);
  for (int i = 0; i < vecProfileGroups.size(); i++)
  {
    CLog::Log(0, "madVR Profile Groups: %s", vecProfileGroups[i].c_str());

    EnumProfiles(path + "\\" + vecProfileGroups[i], &vecProfileName);
    for (int a = 0; a < vecProfileName.size(); a++)
    {
      CLog::Log(0, "madVR Profiles: %s", vecProfileName[a].c_str());

      EnumFolders(path + "\\" + vecProfileGroups[i] + "\\" + vecProfileName[a], &vecFoldersId, &vecFoldersName, &vecFoldersType);
      for (int b = 0; b < vecFoldersId.size(); b++)
      {
        CLog::Log(0, "madVR Folders %s %s %s", vecFoldersId[b].c_str(), vecFoldersName[b].c_str(), vecFoldersType[b].c_str());

        EnumValues(path + "\\" + vecProfileGroups[i] + "\\" + vecProfileName[a] + "\\" + vecFoldersId[b], &vecValuesId, &vecValuesName, &vecValuesType);
        for (int c = 0; c < vecValuesId.size(); c++)
        {
          CLog::Log(0, "madVR Values %s %s %s", vecValuesId[c].c_str(), vecValuesName[c].c_str(), vecValuesType[c].c_str());
        }
      }
    }
  }
}