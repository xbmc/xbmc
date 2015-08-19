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

void CMadvrSettingsManager::GetFloat(std::string path, float* fValue, int iConv)
{
  int iValue;
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  GetSettings(MADVR_SETTINGS_INT, pathW.c_str(), 0, NULL, NULL, &iValue, NULL);
  if (iValue > 0)
    *fValue = (float)iValue / (float)iConv;
  else
    *fValue = 0.0f;
};

void CMadvrSettingsManager::GetDoubling(CStdString path, int* iValue)
{
  CStdString strBool = "nnedi" + path + "Enable";
  CStdString strInt = "nnedi" + path + "Quality";
  CStdString strAlgo = path + "Algo";
  BOOL bValue;
  int aValue;
  int result = -1;
  std::string sValue;

  GetBool(strBool, &bValue);
  if (bValue)
  {
    GetStr(strAlgo, &sValue);
    if (sValue != "NNEDI3")
    {
      result = CMadvrSettings::GeDoubleAlgo(sValue);
    }
    else
    {
      GetInt(strInt, &aValue);
      result = CMadvrSettings::GetDoubleId(aValue);
    }
  };

  *iValue = result;
}

void CMadvrSettingsManager::GetDeintActive(CStdString path, int* iValue)
{
  CStdString strAuto = "autoActivateDeinterlacing";
  CStdString strIfDoubt = "ifInDoubtDeinterlace";

  BOOL bValue1;
  BOOL bValue2;
  int result = -1;

  GetBool(strAuto, &bValue1);
  if (bValue1)
  {
    GetBool(strIfDoubt, &bValue2);
    result = !bValue2;
  };

  *iValue = result;
}

void CMadvrSettingsManager::GetSmoothmotion(CStdString path, int* iValue)
{
  CStdString stEnabled = "smoothMotionEnabled";
  CStdString strMode = "smoothMotionMode";

  BOOL bValue;
  std::string sValue;
  int result = -1;

  GetBool(stEnabled, &bValue);
  if (bValue)
  {
    GetStr(strMode, &sValue);
    result = CMadvrSettings::GetSmoothMotionId(sValue);
  };

  *iValue = result;
}

void CMadvrSettingsManager::GetDithering(CStdString path, int* iValue)
{
  CStdString stDisable = "dontDither";
  CStdString strMode = "ditheringAlgo";

  BOOL bValue;
  std::string sValue;
  int result = -1;

  GetBool(stDisable, &bValue);
  if (!bValue)
  {
    GetStr(strMode, &sValue);
    result = CMadvrSettings::GetDitheringId(sValue);
  };

  *iValue = result;
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

void CMadvrSettingsManager::SetFloat(std::string path, float fValue, int iConv)
{
  int iValue = (int)round(fValue * iConv);
  std::wstring pathW;
  g_charsetConverter.utf8ToW(path, pathW, false);
  SetSettings(MADVR_SETTINGS_INT, pathW.c_str(), NULL, NULL, iValue);
};

void CMadvrSettingsManager::SetDoubling(CStdString path, int iValue)
{
  CStdString strBool = "nnedi" + path + "Enable";
  CStdString strInt = "nnedi" + path + "Quality";
  CStdString strAlgo = path + "Algo";

  SetBool(strBool, (iValue>-1));
  if (iValue > -1)
  {
    SetInt(strInt, MadvrDoubleQuality[iValue].id);
    SetStr(strAlgo, MadvrDoubleQuality[iValue].algo);
  }
}

void CMadvrSettingsManager::SetDeintActive(CStdString path, int iValue)
{
  CStdString strAuto = "autoActivateDeinterlacing";
  CStdString strIfDoubt = "ifInDoubtDeinterlace";

  SetBool(strAuto, (iValue > -1));
  SetBool(strIfDoubt, (iValue != MadvrDeintActiveDef));
}

void CMadvrSettingsManager::SetSmoothmotion(CStdString path, int iValue)
{
  CStdString stEnabled = "smoothMotionEnabled";
  CStdString strMode = "smoothMotionMode";

  SetBool(stEnabled, (iValue > -1));
  if (iValue > -1)
    SetStr(strMode, MadvrSmoothMotion[iValue].name);
}

void CMadvrSettingsManager::SetDithering(CStdString path, int iValue)
{
  CStdString stDisable = "dontDither";
  CStdString strMode = "ditheringAlgo";

  SetBool(stDisable, (iValue == -1));
  if (iValue > -1)
    SetStr(strMode, MadvrDithering[iValue].name);
}

bool CMadvrSettingsManager::IsProfileActive(std::string path, std::string profile)
{
  bool result = false;
  if (Com::SmartQIPtr<IMadVRSettings2> pMadvrSettings2 = m_pDXR)
  {
    std::wstring pathW;
    std::wstring profileW;
    g_charsetConverter.utf8ToW(path, pathW, false);
    g_charsetConverter.utf8ToW(profile, profileW, false);
    result = (bool)pMadvrSettings2->SettingsIsProfileActive(pathW.c_str(), profileW.c_str());
  }
  return result;
}


void CMadvrSettingsManager::GetProfileActiveName(std::string *profile)
{
  std::vector<std::string> vecProfileGroups;
  std::vector<std::string> vecProfileName;
  std::string path = "scalingParent";
  std::string result = "";

  EnumGroups(path, &vecProfileGroups);
  for (unsigned int i = 0; i < vecProfileGroups.size(); i++)
  {
    CLog::Log(0, "madVR Profile Groups: %s", vecProfileGroups[i].c_str());

    EnumProfiles(path + "\\" + vecProfileGroups[i], &vecProfileName);
    for (unsigned int a = 0; a < vecProfileName.size(); a++)
    {
      if (IsProfileActive(path + "\\" + vecProfileGroups[i], vecProfileName[a]))
      {
        result = vecProfileName[a];
        break;
      }
    }
  }
  *profile = result;
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
  for (unsigned int i = 0; i < vecProfileGroups.size(); i++)
  {
    CLog::Log(0, "madVR Profile Groups: %s", vecProfileGroups[i].c_str());

    EnumProfiles(path + "\\" + vecProfileGroups[i], &vecProfileName);
    for (unsigned int a = 0; a < vecProfileName.size(); a++)
    {
      CLog::Log(0, "madVR Profiles: %s", vecProfileName[a].c_str());

      EnumFolders(path + "\\" + vecProfileGroups[i] + "\\" + vecProfileName[a], &vecFoldersId, &vecFoldersName, &vecFoldersType);
      for (unsigned int b = 0; b < vecFoldersId.size(); b++)
      {
        CLog::Log(0, "madVR Folders %s %s %s", vecFoldersId[b].c_str(), vecFoldersName[b].c_str(), vecFoldersType[b].c_str());

        EnumValues(path + "\\" + vecProfileGroups[i] + "\\" + vecProfileName[a] + "\\" + vecFoldersId[b], &vecValuesId, &vecValuesName, &vecValuesType);
        for (unsigned int c = 0; c < vecValuesId.size(); c++)
        {
          CLog::Log(0, "madVR Values %s %s %s", vecValuesId[c].c_str(), vecValuesName[c].c_str(), vecValuesType[c].c_str());
        }
      }
    }
  }
}

void CMadvrSettingsManager::RestoreMadvrSettings()
{
  if (CSettings::Get().GetInt("dsplayer.madvrsettingswithkodi") != KODIGUI_LOAD_DSPLAYER)
    return;

  CMadvrSettings &madvrSettings = CMediaSettings::Get().GetCurrentMadvrSettings();

  SetStr("chromaUp", MadvrScaling[madvrSettings.m_ChromaUpscaling].name);
  SetBool("chromaAntiRinging", madvrSettings.m_ChromaAntiRing);
  SetBool("superChromaRes", madvrSettings.m_ChromaSuperRes);
  SetStr("LumaUp", MadvrScaling[madvrSettings.m_ImageUpscaling].name);
  SetBool("lumaUpAntiRinging", madvrSettings.m_ImageUpAntiRing);
  SetBool("lumaUpLinear", madvrSettings.m_ImageUpLinear);
  SetStr("LumaDown", MadvrScaling[madvrSettings.m_ImageDownscaling].name);
  SetBool("lumaDownAntiRinging", madvrSettings.m_ImageDownAntiRing);
  SetBool("lumaDownLinear", madvrSettings.m_ImageDownLinear);
  SetDoubling("DL", madvrSettings.m_ImageDoubleLuma);
  SetStr("nnediDLScalingFactor", MadvrDoubleFactor[madvrSettings.m_ImageDoubleLumaFactor].name);
  SetDoubling("DC", madvrSettings.m_ImageDoubleChroma);
  SetStr("nnediDCScalingFactor", MadvrDoubleFactor[madvrSettings.m_ImageDoubleChromaFactor].name);
  SetDoubling("QL", madvrSettings.m_ImageQuadrupleLuma);
  SetStr("nnediQLScalingFactor", MadvrQuadrupleFactor[madvrSettings.m_ImageQuadrupleLumaFactor].name);
  SetDoubling("QC", madvrSettings.m_ImageQuadrupleChroma);
  SetStr("nnediQCScalingFactor", MadvrQuadrupleFactor[madvrSettings.m_ImageQuadrupleChromaFactor].name);
  SetDeintActive("", madvrSettings.m_deintactive);
  SetStr("contentType", MadvrDeintForce[madvrSettings.m_deintforce].name);
  SetBool("scanPartialFrame", madvrSettings.m_deintlookpixels);
  SetBool("debandActive", madvrSettings.m_deband);
  SetInt("debandLevel", madvrSettings.m_debandLevel);
  SetInt("debandFadeLevel", madvrSettings.m_debandFadeLevel);
  SetDithering("", madvrSettings.m_dithering);
  SetBool("coloredDither", madvrSettings.m_ditheringColoredNoise);
  SetBool("dynamicDither", madvrSettings.m_ditheringEveryFrame);
  SetSmoothmotion("", madvrSettings.m_smoothMotion);

  SetBool("fineSharp", madvrSettings.m_fineSharp);
  SetFloat("fineSharpStrength", madvrSettings.m_fineSharpStrength, 10);
  SetBool("lumaSharpen", madvrSettings.m_lumaSharpen);
  SetFloat("lumaSharpenStrength", madvrSettings.m_lumaSharpenStrength);
  SetBool("adaptiveSharpen", madvrSettings.m_adaptiveSharpen);
  SetFloat("adaptiveSharpenStrength", madvrSettings.m_adaptiveSharpenStrength, 10);

  SetBool("upRefFineSharp", madvrSettings.m_UpRefFineSharp);
  SetFloat("upRefFineSharpStrength", madvrSettings.m_UpRefFineSharpStrength, 10);
  SetBool("upRefLumaSharpen", madvrSettings.m_UpRefLumaSharpen);
  SetFloat("upRefLumaSharpenStrength", madvrSettings.m_UpRefLumaSharpenStrength);
  SetBool("upRefAdaptiveSharpen", madvrSettings.m_UpRefAdaptiveSharpen);
  SetFloat("upRefAdaptiveSharpenStrength", madvrSettings.m_UpRefAdaptiveSharpenStrength, 10);

  SetBool("superRes", madvrSettings.m_superRes);
  SetFloat("superResStrength", madvrSettings.m_superResStrength, 1);

  SetBool("refineOnce", !madvrSettings.m_refineOnce);
  SetBool("superResFirst", madvrSettings.m_superResFirst);
}

void CMadvrSettingsManager::LoadMadvrSettings(MADVR_LOAD_TYPE type)
{
  CMadvrSettings &madvrSettings = CMediaSettings::Get().GetCurrentMadvrSettings();

  std::string sValue;
  BOOL bValue;

  if (type == MADVR_LOAD_GENERAL)
  {
    GetDeintActive("", &madvrSettings.m_deintactive);
    GetStr("contentType", &sValue);
    madvrSettings.m_deintforce = CMadvrSettings::GetDeintForceId(sValue);
    GetBool("scanPartialFrame", &bValue);
    madvrSettings.m_deintlookpixels = (bool)bValue;
    GetBool("debandActive", &bValue);
    madvrSettings.m_deband = (bool)bValue;
    GetInt("debandLevel", &madvrSettings.m_debandLevel);
    GetInt("debandFadeLevel", &madvrSettings.m_debandFadeLevel);
    GetDithering("", &madvrSettings.m_dithering);
    GetBool("coloredDither", &bValue);
    madvrSettings.m_ditheringColoredNoise = (bool)bValue;
    GetBool("dynamicDither", &bValue);
    madvrSettings.m_ditheringEveryFrame = (bool)bValue;
    GetSmoothmotion("", &madvrSettings.m_smoothMotion);
  }

  if (type == MADVR_LOAD_SCALING)
  {
    GetStr("chromaUp", &sValue);
    madvrSettings.m_ChromaUpscaling = CMadvrSettings::GetScalingId(sValue);
    GetBool("chromaAntiRinging", &bValue);
    madvrSettings.m_ChromaAntiRing = (bool)bValue;
    GetBool("superChromaRes", &bValue);
    madvrSettings.m_ChromaSuperRes = (bool)bValue;
    GetStr("LumaUp", &sValue);
    madvrSettings.m_ImageUpscaling = CMadvrSettings::GetScalingId(sValue);

    GetBool("lumaUpAntiRinging", &bValue);
    madvrSettings.m_ImageUpAntiRing = (bool)bValue;
    GetBool("lumaUpLinear", &bValue);
    madvrSettings.m_ImageUpLinear = (bool)bValue;
    GetStr("LumaDown", &sValue);
    madvrSettings.m_ImageDownscaling = CMadvrSettings::GetScalingId(sValue);
    GetBool("lumaDownAntiRinging", &bValue);
    madvrSettings.m_ImageDownAntiRing = (bool)bValue;
    GetBool("lumaDownLinear", &bValue);
    madvrSettings.m_ImageDownLinear = (bool)bValue;
    GetDoubling("DL", &madvrSettings.m_ImageDoubleLuma);
    GetStr("nnediDLScalingFactor", &sValue);
    madvrSettings.m_ImageDoubleLumaFactor = CMadvrSettings::GetDoubleFactorId(sValue);
    GetDoubling("DC", &madvrSettings.m_ImageDoubleChroma);
    GetStr("nnediDCScalingFactor", &sValue);
    madvrSettings.m_ImageDoubleChromaFactor = CMadvrSettings::GetDoubleFactorId(sValue);
    GetDoubling("QL", &madvrSettings.m_ImageQuadrupleLuma);
    GetStr("nnediQLScalingFactor", &sValue);
    madvrSettings.m_ImageQuadrupleLumaFactor = CMadvrSettings::GetQuadrupleFactorId(sValue);
    GetDoubling("QC", &madvrSettings.m_ImageQuadrupleChroma);
    GetStr("nnediQCScalingFactor", &sValue);
    madvrSettings.m_ImageQuadrupleChromaFactor = CMadvrSettings::GetQuadrupleFactorId(sValue);

    GetBool("fineSharp", &bValue);
    madvrSettings.m_fineSharp = (bool)bValue;
    GetFloat("fineSharpStrength", &madvrSettings.m_fineSharpStrength, 10);
    GetBool("lumaSharpen", &bValue);
    madvrSettings.m_lumaSharpen = (bool)bValue;
    GetFloat("lumaSharpenStrength", &madvrSettings.m_lumaSharpenStrength);
    GetBool("adaptiveSharpen", &bValue);
    madvrSettings.m_adaptiveSharpen = (bool)bValue;
    GetFloat("adaptiveSharpenStrength", &madvrSettings.m_adaptiveSharpenStrength, 10);

    GetBool("upRefFineSharp", &bValue);
    madvrSettings.m_UpRefFineSharp = (bool)bValue;
    GetFloat("upRefFineSharpStrength", &madvrSettings.m_UpRefFineSharpStrength, 10);
    GetBool("upRefLumaSharpen", &bValue);
    madvrSettings.m_UpRefLumaSharpen = (bool)bValue;
    GetFloat("upRefLumaSharpenStrength", &madvrSettings.m_UpRefLumaSharpenStrength);
    GetBool("upRefAdaptiveSharpen", &bValue);
    madvrSettings.m_UpRefAdaptiveSharpen = (bool)bValue;
    GetFloat("upRefAdaptiveSharpenStrength", &madvrSettings.m_UpRefAdaptiveSharpenStrength, 10);

    GetBool("superRes", &bValue);
    madvrSettings.m_superRes = (bool)bValue;
    GetFloat("superResStrength", &madvrSettings.m_superResStrength, 1);

    GetBool("refineOnce", &bValue);
    madvrSettings.m_refineOnce = !bValue;
    GetBool("superResFirst", &bValue);
    madvrSettings.m_superResFirst = (bool)bValue;
  }
}