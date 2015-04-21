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

#include "PlayerCoreFactory.h"
#include "threads/SingleLock.h"
#include "cores/dvdplayer/DVDPlayer.h"
#include "cores/paplayer/PAPlayer.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "URL.h"
#include "FileItem.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "utils/AutoPtrHandle.h"
#include "PlayerCoreConfig.h"
#include "PlayerSelectionRule.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"

#define PLAYERCOREFACTORY_XML "playercorefactory.xml"

using namespace AUTOPTR;

CPlayerCoreFactory::CPlayerCoreFactory()
{ }

CPlayerCoreFactory::~CPlayerCoreFactory()
{
  for(std::vector<CPlayerCoreConfig *>::iterator it = m_vecCoreConfigs.begin(); it != m_vecCoreConfigs.end(); ++it)
    delete *it;
  for(std::vector<CPlayerSelectionRule *>::iterator it = m_vecCoreSelectionRules.begin(); it != m_vecCoreSelectionRules.end(); ++it)
    delete *it;
}

CPlayerCoreFactory& CPlayerCoreFactory::Get()
{
  static CPlayerCoreFactory sPlayerCoreFactory;
  return sPlayerCoreFactory;
}

void CPlayerCoreFactory::OnSettingsLoaded()
{
  LoadConfiguration("special://xbmc/system/" PLAYERCOREFACTORY_XML, true);
  LoadConfiguration(CProfilesManager::Get().GetUserDataItem(PLAYERCOREFACTORY_XML), false);
}

/* generic function to make a vector unique, removes later duplicates */
template<typename T> void unique (T &con)
{
  typename T::iterator cur, end;
  cur = con.begin();
  end = con.end();
  while (cur != end)
  {
    typename T::value_type i = *cur;
    end = remove (++cur, end, i);
  }
  con.erase (end, con.end());
}

IPlayer* CPlayerCoreFactory::CreatePlayer(const std::string& strCore, IPlayerCallback& callback) const
{
  return CreatePlayer(GetPlayerCore(strCore), callback );
}

IPlayer* CPlayerCoreFactory::CreatePlayer(const PLAYERCOREID eCore, IPlayerCallback& callback) const
{
  CSingleLock lock(m_section);
  if (m_vecCoreConfigs.empty() || eCore-1 > m_vecCoreConfigs.size()-1)
    return NULL;

  return m_vecCoreConfigs[eCore-1]->CreatePlayer(callback);
}

PLAYERCOREID CPlayerCoreFactory::GetPlayerCore(const std::string& strCoreName) const
{
  CSingleLock lock(m_section);
  if (!strCoreName.empty())
  {
    // Dereference "*default*player" aliases
    std::string strRealCoreName;
    if (StringUtils::EqualsNoCase(strCoreName, "audiodefaultplayer")) strRealCoreName = g_advancedSettings.m_audioDefaultPlayer;
    else if (StringUtils::EqualsNoCase(strCoreName, "videodefaultplayer")) strRealCoreName = g_advancedSettings.m_videoDefaultPlayer;
    else if (StringUtils::EqualsNoCase(strCoreName, "videodefaultdvdplayer")) strRealCoreName = g_advancedSettings.m_videoDefaultDVDPlayer;
    else strRealCoreName = strCoreName;

    for(PLAYERCOREID i = 0; i < m_vecCoreConfigs.size(); i++)
    {
      if (StringUtils::EqualsNoCase(m_vecCoreConfigs[i]->GetName(), strRealCoreName))
        return i+1;
    }
    CLog::Log(LOGWARNING, "CPlayerCoreFactory::GetPlayerCore(%s): no such core: %s", strCoreName.c_str(), strRealCoreName.c_str());
  }
  return EPC_NONE;
}

std::string CPlayerCoreFactory::GetPlayerName(const PLAYERCOREID eCore) const
{
  CSingleLock lock(m_section);
  return m_vecCoreConfigs[eCore-1]->GetName();
}

CPlayerCoreConfig* CPlayerCoreFactory::GetPlayerConfig(const std::string& strCoreName) const
{
  return GetPlayerConfig(GetPlayerCore(strCoreName));
}

CPlayerCoreConfig* CPlayerCoreFactory::GetPlayerConfig(const PLAYERCOREID eCore) const
{
  CSingleLock lock(m_section);
  if (eCore != EPC_NONE)
    return m_vecCoreConfigs[eCore - 1];
  else
    return NULL;
}

void CPlayerCoreFactory::GetPlayers( VECPLAYERCORES &vecCores ) const
{
  CSingleLock lock(m_section);
  for(unsigned int i = 0; i < m_vecCoreConfigs.size(); i++)
  {
    if(m_vecCoreConfigs[i]->m_eCore == EPC_NONE)
      continue;
    if (m_vecCoreConfigs[i]->m_bPlaysAudio || m_vecCoreConfigs[i]->m_bPlaysVideo)
      vecCores.push_back(i+1);
  }
}

void CPlayerCoreFactory::GetPlayers( VECPLAYERCORES &vecCores, const bool audio, const bool video ) const
{
  CSingleLock lock(m_section);
  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: for video=%d, audio=%d", video, audio);

  for(unsigned int i = 0; i < m_vecCoreConfigs.size(); i++)
  {
    if(m_vecCoreConfigs[i]->m_eCore == EPC_NONE)
      continue;
    if (audio == m_vecCoreConfigs[i]->m_bPlaysAudio && video == m_vecCoreConfigs[i]->m_bPlaysVideo)
    {
      CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding player: %s (%d)", m_vecCoreConfigs[i]->m_name.c_str(), i+1);
      vecCores.push_back(i+1);
    }
  }
}

void CPlayerCoreFactory::GetPlayers( const CFileItem& item, VECPLAYERCORES &vecCores) const
{
  CURL url(item.GetPath());

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers(%s)", CURL::GetRedacted(item.GetPath()).c_str());

  // Process rules
  for(unsigned int i = 0; i < m_vecCoreSelectionRules.size(); i++)
    m_vecCoreSelectionRules[i]->GetPlayers(item, vecCores);

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: matched %" PRIuS" rules with players", vecCores.size());

  if( PAPlayer::HandlesType(url.GetFileType()) )
  {
    // We no longer force PAPlayer as our default audio player (used to be true):
    bool bAdd = false;
    if (url.IsProtocol("mms"))
    {
       bAdd = false;
    }
    else if (item.IsType(".wma"))
    {
//      bAdd = true;
//      DVDPlayerCodec codec;
//      if (!codec.Init(item.GetPath(),2048))
//        bAdd = false;
//      codec.DeInit();
    }

    if (bAdd)
    {
      if (url.IsFileType("ac3") || url.IsFileType("dts"))
      {
        CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding DVDPlayer (%d)", EPC_DVDPLAYER);
        vecCores.push_back(EPC_DVDPLAYER);
      }
      else
      {
        CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding PAPlayer (%d)", EPC_PAPLAYER);
        vecCores.push_back(EPC_PAPLAYER);
      }
    }
  }

  // Process defaults

  // Set video default player. Check whether it's video first (overrule audio check)
  // Also push these players in case it is NOT audio either
  if (item.IsVideo() || !item.IsAudio())
  {
    PLAYERCOREID eVideoDefault = GetPlayerCore("videodefaultplayer");
    if (eVideoDefault != EPC_NONE)
    {
      CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding videodefaultplayer (%d)", eVideoDefault);
      vecCores.push_back(eVideoDefault);
    }
    GetPlayers(vecCores, false, true);  // Video-only players
    GetPlayers(vecCores, true, true);   // Audio & video players
  }

  // Set audio default player
  // Pushback all audio players in case we don't know the type
  if (item.IsAudio())
  {
    PLAYERCOREID eAudioDefault = GetPlayerCore("audiodefaultplayer");
    if (eAudioDefault != EPC_NONE)
    {
      CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding audiodefaultplayer (%d)", eAudioDefault);
      vecCores.push_back(eAudioDefault);
    }
    GetPlayers(vecCores, true, false); // Audio-only players
    GetPlayers(vecCores, true, true);  // Audio & video players
  }

  /* make our list unique, preserving first added players */
  unique(vecCores);

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: added %" PRIuS" players", vecCores.size());
}

void CPlayerCoreFactory::GetRemotePlayers( VECPLAYERCORES &vecCores ) const
{
  CSingleLock lock(m_section);
  for(unsigned int i = 0; i < m_vecCoreConfigs.size(); i++)
  {
    if(m_vecCoreConfigs[i]->m_eCore != EPC_UPNPPLAYER)
      continue;
    vecCores.push_back(i+1);
  }
}

PLAYERCOREID CPlayerCoreFactory::GetDefaultPlayer( const CFileItem& item ) const
{
  VECPLAYERCORES vecCores;
  GetPlayers(item, vecCores);

  //If we have any players return the first one
  if( !vecCores.empty() ) return vecCores.at(0);

  return EPC_NONE;
}

PLAYERCOREID CPlayerCoreFactory::SelectPlayerDialog(const VECPLAYERCORES &vecCores, float posX, float posY) const
{
  CContextButtons choices;
  if (vecCores.size())
  {
    //Add default player
    std::string strCaption = CPlayerCoreFactory::GetPlayerName(vecCores[0]);
    strCaption += " (";
    strCaption += g_localizeStrings.Get(13278);
    strCaption += ")";
    choices.Add(0, strCaption);

    //Add all other players
    for( unsigned int i = 1; i < vecCores.size(); i++ )
      choices.Add(i, CPlayerCoreFactory::GetPlayerName(vecCores[i]));

    int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if (choice >= 0)
      return vecCores[choice];
  }
  return EPC_NONE;
}

PLAYERCOREID CPlayerCoreFactory::SelectPlayerDialog(float posX, float posY) const
{
  VECPLAYERCORES vecCores;
  GetPlayers(vecCores);
  return SelectPlayerDialog(vecCores, posX, posY);
}

bool CPlayerCoreFactory::LoadConfiguration(const std::string &file, bool clear)
{
  CSingleLock lock(m_section);

  CLog::Log(LOGNOTICE, "Loading player core factory settings from %s.", file.c_str());
  if (!XFILE::CFile::Exists(file))
  { // tell the user it doesn't exist
    CLog::Log(LOGNOTICE, "%s does not exist. Skipping.", file.c_str());
    return false;
  }

  CXBMCTinyXML playerCoreFactoryXML;
  if (!playerCoreFactoryXML.LoadFile(file))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d (%s)", file.c_str(), playerCoreFactoryXML.ErrorRow(), playerCoreFactoryXML.ErrorDesc());
    return false;
  }

  TiXmlElement *pConfig = playerCoreFactoryXML.RootElement();
  if (pConfig == NULL)
  {
      CLog::Log(LOGERROR, "Error loading %s, Bad structure", file.c_str());
      return false;
  }

  if (clear)
  {
    for(std::vector<CPlayerCoreConfig *>::iterator it = m_vecCoreConfigs.begin(); it != m_vecCoreConfigs.end(); ++it)
      delete *it;
    m_vecCoreConfigs.clear();
    // Builtin players; hard-coded because re-ordering them would break scripts
    CPlayerCoreConfig* dvdplayer = new CPlayerCoreConfig("DVDPlayer", EPC_DVDPLAYER, NULL);
    dvdplayer->m_bPlaysAudio = dvdplayer->m_bPlaysVideo = true;
    m_vecCoreConfigs.push_back(dvdplayer);

     // Don't remove this, its a placeholder for the old MPlayer core, it would break scripts
    CPlayerCoreConfig* mplayer = new CPlayerCoreConfig("oldmplayercore", EPC_DVDPLAYER, NULL);
    m_vecCoreConfigs.push_back(mplayer);

    CPlayerCoreConfig* paplayer = new CPlayerCoreConfig("PAPlayer", EPC_PAPLAYER, NULL);
    paplayer->m_bPlaysAudio = true;
    m_vecCoreConfigs.push_back(paplayer);

    for(std::vector<CPlayerSelectionRule *>::iterator it = m_vecCoreSelectionRules.begin(); it != m_vecCoreSelectionRules.end(); ++it)
      delete *it;
    m_vecCoreSelectionRules.clear();
  }

  if (!pConfig || strcmpi(pConfig->Value(),"playercorefactory") != 0)
  {
    CLog::Log(LOGERROR, "Error loading configuration, no <playercorefactory> node");
    return false;
  }

  TiXmlElement *pPlayers = pConfig->FirstChildElement("players");
  if (pPlayers)
  {
    TiXmlElement* pPlayer = pPlayers->FirstChildElement("player");
    while (pPlayer)
    {
      std::string name = XMLUtils::GetAttribute(pPlayer, "name");
      std::string type = XMLUtils::GetAttribute(pPlayer, "type");
      if (type.empty()) type = name;
      StringUtils::ToLower(type);

      EPLAYERCORES eCore = EPC_NONE;
      if (type == "dvdplayer" || type == "mplayer") eCore = EPC_DVDPLAYER;
      if (type == "paplayer" ) eCore = EPC_PAPLAYER;
      if (type == "externalplayer" ) eCore = EPC_EXTPLAYER;

      if (eCore != EPC_NONE)
      {
        m_vecCoreConfigs.push_back(new CPlayerCoreConfig(name, eCore, pPlayer));
      }

      pPlayer = pPlayer->NextSiblingElement("player");
    }
  }

  TiXmlElement *pRule = pConfig->FirstChildElement("rules");
  while (pRule)
  {
    const char* szAction = pRule->Attribute("action");
    if (szAction)
    {
      if (stricmp(szAction, "append") == 0)
      {
        m_vecCoreSelectionRules.push_back(new CPlayerSelectionRule(pRule));
      }
      else if (stricmp(szAction, "prepend") == 0)
      {
        m_vecCoreSelectionRules.insert(m_vecCoreSelectionRules.begin(), 1, new CPlayerSelectionRule(pRule));
      }
      else
      {
        m_vecCoreSelectionRules.clear();
        m_vecCoreSelectionRules.push_back(new CPlayerSelectionRule(pRule));
      }
    }
    else
    {
      m_vecCoreSelectionRules.push_back(new CPlayerSelectionRule(pRule));
    }

    pRule = pRule->NextSiblingElement("rules");
  }

  // succeeded - tell the user it worked
  CLog::Log(LOGNOTICE, "Loaded playercorefactory configuration");

  return true;
}

void CPlayerCoreFactory::OnPlayerDiscovered(const std::string& id, const std::string& name, EPLAYERCORES core)
{
  CSingleLock lock(m_section);
  std::vector<CPlayerCoreConfig *>::iterator it;
  for(it  = m_vecCoreConfigs.begin();
      it != m_vecCoreConfigs.end();
      ++it)
  {
    if ((*it)->GetId() == id)
    {
      (*it)->m_name  = name;
      (*it)->m_eCore = core;
      return;
    }
  }

  CPlayerCoreConfig* player = new CPlayerCoreConfig(name, core, NULL, id);
  player->m_bPlaysAudio = true;
  player->m_bPlaysVideo = true;
  m_vecCoreConfigs.push_back(player);
}

void CPlayerCoreFactory::OnPlayerRemoved(const std::string& id)
{
  CSingleLock lock(m_section);
  std::vector<CPlayerCoreConfig *>::iterator it;
  for(it  = m_vecCoreConfigs.begin();
      it != m_vecCoreConfigs.end();
      ++it)
  {
    if ((*it)->GetId() == id)
      (*it)->m_eCore = EPC_NONE;
  }
}
