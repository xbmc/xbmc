/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "utils/BitstreamStats.h"
#include "PlayerCoreFactory.h"
#include "threads/SingleLock.h"
#include "cores/dvdplayer/DVDPlayer.h"
#include "cores/paplayer/PAPlayer.h"
#include "cores/paplayer/DVDPlayerCodec.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "utils/HttpHeader.h"
#include "settings/GUISettings.h"
#include "URL.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "utils/AutoPtrHandle.h"
#include "cores/ExternalPlayer/ExternalPlayer.h"
#include "PlayerCoreConfig.h"
#include "PlayerSelectionRule.h"
#include "guilib/LocalizeStrings.h"

using namespace AUTOPTR;

std::vector<CPlayerCoreConfig *> CPlayerCoreFactory::s_vecCoreConfigs;
std::vector<CPlayerSelectionRule *> CPlayerCoreFactory::s_vecCoreSelectionRules;

CPlayerCoreFactory::CPlayerCoreFactory()
{}
CPlayerCoreFactory::~CPlayerCoreFactory()
{
  for(std::vector<CPlayerCoreConfig *>::iterator it = s_vecCoreConfigs.begin(); it != s_vecCoreConfigs.end(); it++)
    delete *it;
  for(std::vector<CPlayerSelectionRule *>::iterator it = s_vecCoreSelectionRules.begin(); it != s_vecCoreSelectionRules.end(); it++)
    delete *it;
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

IPlayer* CPlayerCoreFactory::CreatePlayer(const CStdString& strCore, IPlayerCallback& callback) const
{
  return CreatePlayer( GetPlayerCore(strCore), callback );
}

IPlayer* CPlayerCoreFactory::CreatePlayer(const PLAYERCOREID eCore, IPlayerCallback& callback)
{
  if (!s_vecCoreConfigs.size() || eCore-1 > s_vecCoreConfigs.size()-1)
    return NULL;

  return s_vecCoreConfigs[eCore-1]->CreatePlayer(callback);
}

PLAYERCOREID CPlayerCoreFactory::GetPlayerCore(const CStdString& strCoreName)
{
  if (!strCoreName.empty())
  {
    // Dereference "*default*player" aliases
    CStdString strRealCoreName;
    if (strCoreName.Equals("audiodefaultplayer", false)) strRealCoreName = g_advancedSettings.m_audioDefaultPlayer;
    else if (strCoreName.Equals("videodefaultplayer", false)) strRealCoreName = g_advancedSettings.m_videoDefaultPlayer;
    else if (strCoreName.Equals("videodefaultdvdplayer", false)) strRealCoreName = g_advancedSettings.m_videoDefaultDVDPlayer;
    else strRealCoreName = strCoreName;

    for(PLAYERCOREID i = 0; i < s_vecCoreConfigs.size(); i++)
    {
      if (s_vecCoreConfigs[i]->GetName().Equals(strRealCoreName, false))
        return i+1;
    }
    CLog::Log(LOGWARNING, "CPlayerCoreFactory::GetPlayerCore(%s): no such core: %s", strCoreName.c_str(), strRealCoreName.c_str());
  }
  return EPC_NONE;
}

CStdString CPlayerCoreFactory::GetPlayerName(const PLAYERCOREID eCore)
{
  return s_vecCoreConfigs[eCore-1]->GetName();
}

CPlayerCoreConfig* CPlayerCoreFactory::GetPlayerConfig(const CStdString& strCoreName)
{
  PLAYERCOREID id = GetPlayerCore(strCoreName);
  if (id != EPC_NONE) return s_vecCoreConfigs[id-1];
  else return NULL;
}

void CPlayerCoreFactory::GetPlayers( VECPLAYERCORES &vecCores )
{
  for(unsigned int i = 0; i < s_vecCoreConfigs.size(); i++)
    if (s_vecCoreConfigs[i]->m_bPlaysAudio || s_vecCoreConfigs[i]->m_bPlaysVideo)
      vecCores.push_back(i+1);
}

void CPlayerCoreFactory::GetPlayers( VECPLAYERCORES &vecCores, const bool audio, const bool video )
{
  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: for video=%d, audio=%d", video, audio);

  for(unsigned int i = 0; i < s_vecCoreConfigs.size(); i++)
  {
    if (audio == s_vecCoreConfigs[i]->m_bPlaysAudio && video == s_vecCoreConfigs[i]->m_bPlaysVideo)
    {
      CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding player: %s (%d)", s_vecCoreConfigs[i]->m_name.c_str(), i+1);
      vecCores.push_back(i+1);
    }
  }
}

void CPlayerCoreFactory::GetPlayers( const CFileItem& item, VECPLAYERCORES &vecCores)
{
  CURL url(item.GetPath());

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers(%s)", item.GetPath().c_str());

  // Process rules
  for(unsigned int i = 0; i < s_vecCoreSelectionRules.size(); i++)
    s_vecCoreSelectionRules[i]->GetPlayers(item, vecCores);

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: matched %"PRIuS" rules with players", vecCores.size());

  if( PAPlayer::HandlesType(url.GetFileType()) )
  {
    // We no longer force PAPlayer as our default audio player (used to be true):
    bool bAdd = false;
    if (url.GetProtocol().Equals("mms"))
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
      if( g_guiSettings.GetInt("audiooutput.mode") == AUDIO_ANALOG )
      {
        CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding PAPlayer (%d)", EPC_PAPLAYER);
        vecCores.push_back(EPC_PAPLAYER);
      }
      else if (url.GetFileType().Equals("ac3") 
            || url.GetFileType().Equals("dts"))
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

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: added %"PRIuS" players", vecCores.size());
}

PLAYERCOREID CPlayerCoreFactory::GetDefaultPlayer( const CFileItem& item )
{
  VECPLAYERCORES vecCores;
  GetPlayers(item, vecCores);

  //If we have any players return the first one
  if( vecCores.size() > 0 ) return vecCores.at(0);

  return EPC_NONE;
}

PLAYERCOREID CPlayerCoreFactory::SelectPlayerDialog(VECPLAYERCORES &vecCores, float posX, float posY)
{
  CContextButtons choices;
  if (vecCores.size())
  {
    //Add default player
    CStdString strCaption = CPlayerCoreFactory::GetPlayerName(vecCores[0]);
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

PLAYERCOREID CPlayerCoreFactory::SelectPlayerDialog(float posX, float posY)
{
  VECPLAYERCORES vecCores;
  GetPlayers(vecCores);
  return SelectPlayerDialog(vecCores, posX, posY);
}

bool CPlayerCoreFactory::LoadConfiguration(TiXmlElement* pConfig, bool clear)
{
  if (clear)
  {
    for(std::vector<CPlayerCoreConfig *>::iterator it = s_vecCoreConfigs.begin(); it != s_vecCoreConfigs.end(); it++)
      delete *it;
    s_vecCoreConfigs.clear();
    // Builtin players; hard-coded because re-ordering them would break scripts
    CPlayerCoreConfig* dvdplayer = new CPlayerCoreConfig("DVDPlayer", EPC_DVDPLAYER, NULL);
    dvdplayer->m_bPlaysAudio = dvdplayer->m_bPlaysVideo = true;
    s_vecCoreConfigs.push_back(dvdplayer);

     // Don't remove this, its a placeholder for the old MPlayer core, it would break scripts
    CPlayerCoreConfig* mplayer = new CPlayerCoreConfig("oldmplayercore", EPC_DVDPLAYER, NULL);
    s_vecCoreConfigs.push_back(mplayer);

    CPlayerCoreConfig* paplayer = new CPlayerCoreConfig("PAPlayer", EPC_PAPLAYER, NULL);
    paplayer->m_bPlaysAudio = true;
    s_vecCoreConfigs.push_back(paplayer);

#if defined(HAS_AMLPLAYER)
    CPlayerCoreConfig* amlplayer = new CPlayerCoreConfig("AMLPlayer", EPC_AMLPLAYER, NULL);
    amlplayer->m_bPlaysAudio = true;
    amlplayer->m_bPlaysVideo = true;
    s_vecCoreConfigs.push_back(amlplayer);
#endif

#if defined(HAS_OMXPLAYER)
    CPlayerCoreConfig* omxplayer = new CPlayerCoreConfig("OMXPlayer", EPC_OMXPLAYER, NULL);
    omxplayer->m_bPlaysAudio = true;
    omxplayer->m_bPlaysVideo = true;
    s_vecCoreConfigs.push_back(omxplayer);
#endif

    for(std::vector<CPlayerSelectionRule *>::iterator it = s_vecCoreSelectionRules.begin(); it != s_vecCoreSelectionRules.end(); it++)
      delete *it;
    s_vecCoreSelectionRules.clear();
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
      CStdString name = pPlayer->Attribute("name");
      CStdString type = pPlayer->Attribute("type");
      if (type.length() == 0) type = name;
      type.ToLower();

      EPLAYERCORES eCore = EPC_NONE;
      if (type == "dvdplayer" || type == "mplayer") eCore = EPC_DVDPLAYER;
      if (type == "paplayer" ) eCore = EPC_PAPLAYER;
      if (type == "externalplayer" ) eCore = EPC_EXTPLAYER;

      if (eCore != EPC_NONE)
      {
        s_vecCoreConfigs.push_back(new CPlayerCoreConfig(name, eCore, pPlayer));
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
        s_vecCoreSelectionRules.push_back(new CPlayerSelectionRule(pRule));
      }
      else if (stricmp(szAction, "prepend") == 0)
      {
        s_vecCoreSelectionRules.insert(s_vecCoreSelectionRules.begin(), 1, new CPlayerSelectionRule(pRule));
      }
      else
      {
        s_vecCoreSelectionRules.clear();
        s_vecCoreSelectionRules.push_back(new CPlayerSelectionRule(pRule));
      }
    }
    else
    {
      s_vecCoreSelectionRules.push_back(new CPlayerSelectionRule(pRule));
    }

    pRule = pRule->NextSiblingElement("rules");
  }

  // succeeded - tell the user it worked
  CLog::Log(LOGNOTICE, "Loaded playercorefactory configuration");

  return true;
}
