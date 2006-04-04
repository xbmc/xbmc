
#include "../stdafx.h"
#include "PlayerCoreFactory.h"
#include "mplayer.h"
#include "modplayer.h"
//#include "SidPlayer.h"
#include "dvdplayer\DVDPlayer.h"
#include "paplayer\paplayer.h"
#include "..\GUIDialogContextMenu.h"
#include "../XBAudioConfig.h"
#include "../FileSystem/FileCurl.h"
#include "../utils/HttpHeader.h"

CPlayerCoreFactory::CPlayerCoreFactory()
{}
CPlayerCoreFactory::~CPlayerCoreFactory()
{}

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

IPlayer* CPlayerCoreFactory::CreatePlayer(const EPLAYERCORES eCore, IPlayerCallback& callback)
{
  switch( eCore )
  {
    case EPC_DVDPLAYER: return new CDVDPlayer(callback);
    case EPC_MPLAYER: return new CMPlayer(callback);
    case EPC_MODPLAYER: return new ModPlayer(callback);
//    case EPC_SIDPLAYER: return new SidPlayer(callback);
    case EPC_PAPLAYER: return new PAPlayer(callback); // added by dataratt
  }
  return NULL;
}

EPLAYERCORES CPlayerCoreFactory::GetPlayerCore(const CStdString& strCore)
{
  CStdString strCoreLower = strCore;
  strCoreLower.ToLower();

  if (strCoreLower == "dvdplayer") return EPC_DVDPLAYER;
  if (strCoreLower == "mplayer") return EPC_MPLAYER;
  if (strCoreLower == "mod") return EPC_MODPLAYER;
//  if (strCoreLower == "sid") return EPC_SIDPLAYER;
  if (strCoreLower == "paplayer" ) return EPC_PAPLAYER;
  return EPC_NONE;
}

CStdString CPlayerCoreFactory::GetPlayerName(const EPLAYERCORES eCore)
{
  switch( eCore )
  {
    case EPC_DVDPLAYER: return "DVDPlayer";
    case EPC_MPLAYER: return "MPlayer";
    case EPC_MODPLAYER: return "MODPlayer";
//    case EPC_SIDPLAYER: return "SIDPlayer";
    case EPC_PAPLAYER: return "PAPlayer";
  }
  return "";
}

void CPlayerCoreFactory::GetPlayers( VECPLAYERCORES &vecCores )
{
  vecCores.push_back(EPC_MPLAYER);
  vecCores.push_back(EPC_DVDPLAYER);
  vecCores.push_back(EPC_MODPLAYER);
//  vecCores.push_back(EPC_SIDPLAYER);
  vecCores.push_back(EPC_PAPLAYER);
}

void CPlayerCoreFactory::GetPlayers( const CFileItem& item, VECPLAYERCORES &vecCores)
{
  CURL url(item.m_strPath);

  CLog::Log(LOGDEBUG,"CPlayerCoreFactor::GetPlayers(%s)",item.m_strPath.c_str());

  if (url.GetProtocol().Equals("lastfm"))
  {
    vecCores.push_back(EPC_PAPLAYER);    
  }

  if (url.GetProtocol().Equals("daap"))		// mplayer is better for daap
  {
    // due to us not using curl for all url handling, extension checking doesn't work
    // when there is an option at the end of the url. should eventually be moved over
    // thou let's hack around it for now
    if ( strstr( g_stSettings.m_szMyVideoExtensions, url.GetFileType().c_str() ) )
    {
      vecCores.push_back(EPC_MPLAYER);
      vecCores.push_back(EPC_DVDPLAYER);
    }

    if ( strstr( g_stSettings.m_szMyMusicExtensions, url.GetFileType().c_str() ) )
    {
      vecCores.push_back(EPC_PAPLAYER);
      vecCores.push_back(EPC_MPLAYER);
      vecCores.push_back(EPC_DVDPLAYER);
    }
  }

  if ( item.IsInternetStream() )
  {
    CURL url = item.GetAsUrl();

    CStdString protocol = url.GetProtocol();
    if( protocol == "http" || protocol == "shout" )
    {
      /* curl is picky about protocol */
      url.SetProtocol("http");

      CHttpHeader headers;
      if( CFileCurl::GetHttpHeader(url, headers) )
      {
        if( headers.GetValue("icy-meta").length() > 0 
         || headers.GetValue("icy-pub").length() > 0
         || headers.GetValue("icy-br").length() > 0 
         || headers.GetValue("icy-url").length() > 0 )
        { // shoutcast stuff
          //vecCores.push_back(EPC_DVDPLAYER);
        }
        CStdString content = headers.GetContentType();

        if( content == "video/x-flv" // mplayer fails on these
         || content == "audio/aacp") // mplayer has no support for AAC+         
          vecCores.push_back(EPC_DVDPLAYER);
      }
    }

    // allways add mplayer as a high prio player for internet streams
    vecCores.push_back(EPC_MPLAYER);
  }

  if (((item.IsDVD()) || item.IsDVDFile() || item.IsDVDImage()))
  {
    vecCores.push_back(EPC_DVDPLAYER);
  }

  if( PAPlayer::HandlesType(url.GetFileType()) )
  {
    if( g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_ANALOG )
    {
      vecCores.push_back(EPC_PAPLAYER);
    }
    else if( ( url.GetFileType().Equals("ac3") && g_audioConfig.GetAC3Enabled() )
         ||  ( url.GetFileType().Equals("dts") && g_audioConfig.GetDTSEnabled() ) ) 
    {
      //NOP
    }
    else
    {
      vecCores.push_back(EPC_PAPLAYER);
    }
  }
  
  if( ModPlayer::IsSupportedFormat(url.GetFileType()) || (url.GetFileType() == "xm") || (url.GetFileType() == "mod") || (url.GetFileType() == "s3m") || (url.GetFileType() == "it") )
  {
    vecCores.push_back(EPC_MODPLAYER);
  }
  
/*  if( url.GetFileType() == "sid" )
  {
    vecCores.push_back(EPC_SIDPLAYER);
  }*/

  //Add all normal players last so you can force them, should you want to
  if( item.IsVideo() || item.IsAudio() )
  {
    vecCores.push_back(EPC_MPLAYER);

    vecCores.push_back(EPC_DVDPLAYER);

    if( item.IsAudio())
      vecCores.push_back(EPC_PAPLAYER);
  }

  /* make our list unique, presevering first added players */
  unique(vecCores);
}

EPLAYERCORES CPlayerCoreFactory::GetDefaultPlayer( const CFileItem& item )
{
  VECPLAYERCORES vecCores;
  GetPlayers(item, vecCores);

  //If we have any players return the first one
  if( vecCores.size() > 0 ) return vecCores.at(0);
  
  return EPC_NONE;
}

EPLAYERCORES CPlayerCoreFactory::SelectPlayerDialog(VECPLAYERCORES &vecCores, int iPosX, int iPosY)
{

  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);

  // Reset menu
  pMenu->Initialize();

  // Add all possible players
  auto_aptr<int> btn_Cores(NULL);
  if( vecCores.size() > 0 )
  {    
    btn_Cores = new int[ vecCores.size() ];
    btn_Cores[0] = 0;

    CStdString strCaption;

    //Add default player
    strCaption = CPlayerCoreFactory::GetPlayerName(vecCores[0]);
    strCaption += " (";
    strCaption += g_localizeStrings.Get(13278);
    strCaption += ")";
    btn_Cores[0] = pMenu->AddButton(strCaption);

    //Add all other players
    for( unsigned int i = 1; i < vecCores.size(); i++ )
    {
      strCaption = CPlayerCoreFactory::GetPlayerName(vecCores[i]);
      btn_Cores[i] = pMenu->AddButton(strCaption);
    }
  }

  // Display menu
  pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
  pMenu->DoModal(m_gWindowManager.GetActiveWindow());

  //Check what player we selected
  int btnid = pMenu->GetButton();
  for( unsigned int i = 0; i < vecCores.size(); i++ )
  {
    if( btnid == btn_Cores[i] )
    {
      return vecCores[i];
    }
  }

  return EPC_NONE;
}

EPLAYERCORES CPlayerCoreFactory::SelectPlayerDialog(int iPosX, int iPosY)
{
  VECPLAYERCORES vecCores;
  GetPlayers(vecCores);
  return SelectPlayerDialog(vecCores, iPosX, iPosY);
}
