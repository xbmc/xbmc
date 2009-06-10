#include "stdafx.h"
#include "URL.h"
#include "PlayerSelectionRule.h"

CPlayerSelectionRule::CPlayerSelectionRule(TiXmlElement* pRule)
{
  Initialize(pRule);
}

CPlayerSelectionRule::~CPlayerSelectionRule()
{}

void CPlayerSelectionRule::Initialize(TiXmlElement* pRule)
{
  m_tInternetStream = GetTristate(pRule->Attribute("internetstream"));
  m_tAudio = GetTristate(pRule->Attribute("audio"));
  m_tVideo = GetTristate(pRule->Attribute("video"));

  m_tDVD = GetTristate(pRule->Attribute("dvd"));
  m_tDVDFile = GetTristate(pRule->Attribute("dvdfile"));
  m_tDVDImage = GetTristate(pRule->Attribute("dvdimage"));

  m_protocols = pRule->Attribute("protocols");
  m_fileTypes = pRule->Attribute("filetypes");
  m_mimeTypes = pRule->Attribute("mimetypes");
  m_fileName = pRule->Attribute("filename");

  m_playerName = pRule->Attribute("player");
  m_playerCoreId = 0;

  TiXmlElement* pSubRule = pRule->FirstChildElement("rule");
  while (pSubRule) 
  {
    vecSubRules.push_back(new CPlayerSelectionRule(pSubRule));
    pSubRule = pSubRule->NextSiblingElement("rule");
  }
}

int CPlayerSelectionRule::GetTristate(const char* szValue) const
{
  if (szValue)
  {
    if (stricmp(szValue, "true") == 0) return 1;
    if (stricmp(szValue, "false") == 0) return 0;
  }
  return -1;
}

void CPlayerSelectionRule::GetPlayers(const CFileItem& item, VECPLAYERCORES &vecCores)
{
  if (m_tAudio >= 0 && (m_tAudio > 0) != item.IsAudio()) return;
  if (m_tVideo >= 0 && (m_tVideo > 0) != item.IsVideo()) return;
  if (m_tInternetStream >= 0 && (m_tInternetStream > 0) != item.IsInternetStream()) return;

  if (m_tDVD >= 0 && (m_tDVD > 0) != item.IsDVD()) return;
  if (m_tDVDFile >= 0 && (m_tDVDFile > 0) != item.IsDVDFile()) return;
  if (m_tDVDImage >= 0 && (m_tDVDImage > 0) != item.IsDVDImage()) return;

  CURL url(item.m_strPath);

  CRegExp regExp;
  if (m_fileTypes && m_fileTypes.length() > 0 && regExp.RegComp(m_fileTypes.c_str()))
    if (regExp.RegFind(url.GetFileType(), 0) != 0) return;
  
  if (m_protocols && m_protocols.length() > 0 && regExp.RegComp(m_protocols.c_str()) &&
      regExp.RegFind(url.GetProtocol(), 0) != 0) return;
  
  if (m_mimeTypes && m_mimeTypes.length() > 0 && regExp.RegComp(m_mimeTypes.c_str()) &&
      regExp.RegFind(item.GetContentType(), 0) != 0) return;

  if (m_fileName && m_fileName.length() > 0 && regExp.RegComp(m_fileName.c_str()) &&
      regExp.RegFind(item.m_strPath, 0) != 0) return;

  for (unsigned int i = 0; i < vecSubRules.size(); i++)
    vecSubRules[i]->GetPlayers(item, vecCores);
  
  if (GetPlayerCore() != EPC_NONE)
    vecCores.push_back(GetPlayerCore());
}

PLAYERCOREID CPlayerSelectionRule::GetPlayerCore()
{
  if (!m_playerCoreId)
  {
    m_playerCoreId = CPlayerCoreFactory::GetPlayerCore(m_playerName);
  }
  return m_playerCoreId;
}

