
#include "stdafx.h"
#include "PlayerCoreFactory.h"
#include "dvdplayer\DVDPlayer.h"
#ifdef HAS_XBOX_HARDWARE
#include "mplayer\mplayer.h"
#else
#include "DummyVideoPlayer.h"
#endif
#ifdef HAS_MODPLAYER
#include "modplayer.h"
#endif
#include "paplayer\paplayer.h"
#include "paplayer/wmacodec.h"
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
#ifdef HAS_XBOX_HARDWARE
    case EPC_MPLAYER: return new CMPlayer(callback);
#else
    case EPC_MPLAYER: return new CDVDPlayer(callback);
#endif
#ifdef HAS_MODPLAYER
    case EPC_MODPLAYER: return new ModPlayer(callback);
#endif
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

  // uggly hack for ReplayTV. our filesystem is broken against real ReplayTV's (not the psuevdo DVArchive)
  // it breaks down for small requests. As we can't allow truncated reads for all emulated dll file functions
  // we are often forced to do small reads to fill up the full buffer size wich seems gives garbage back
  if (url.GetProtocol().Equals("rtv"))
    vecCores.push_back(EPC_DVDPLAYER);    

  if (url.GetProtocol().Equals("hdhomerun"))
    vecCores.push_back(EPC_DVDPLAYER);

  if (url.GetProtocol().Equals("lastfm"))
  {
    vecCores.push_back(EPC_PAPLAYER);    
  }

  // force flv files to default to mplayer due to weak http streaming in dvdplayer
  if (url.GetFileType() == "flv" )
  {
    vecCores.push_back(EPC_MPLAYER);
  }
  
  // dvdplayer can play standard rtsp streams, mplayer can't
  if (url.GetProtocol().Equals("rtsp") 
  && !url.GetFileType().Equals("rm") 
  && !url.GetFileType().Equals("ra"))
    vecCores.push_back(EPC_DVDPLAYER);

  if ( item.IsInternetStream() )
  {
    CStdString content = item.GetContentType();

    if (content == "video/x-flv" // mplayer fails on these
     || content == "video/flv")
      vecCores.push_back(EPC_DVDPLAYER);
    else if (content == "audio/aacp") // mplayer has no support for AAC+         
      vecCores.push_back(EPC_DVDPLAYER);
    else if (content == "application/octet-stream")
    {
      //unknown contenttype, send mp2 to pap, mplayer fails
      if( url.GetFileType() == "mp2")
        vecCores.push_back(EPC_PAPLAYER);
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
    bool bAdd = true;
    if (item.IsType(".wma"))
    {
#ifdef HAS_WMA_CODEC
      WMACodec codec;
      if (!codec.Init(item.m_strPath,2048))
        bAdd = false;
      codec.DeInit();        
#endif
    }

    if (bAdd)
    {
      if( g_guiSettings.GetInt("audiooutput.mode") == AUDIO_ANALOG )
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
  }
#ifdef HAS_MODPLAYER
  if( ModPlayer::IsSupportedFormat(url.GetFileType()) || (url.GetFileType() == "xm") || (url.GetFileType() == "mod") || (url.GetFileType() == "s3m") || (url.GetFileType() == "it") )
  {
    vecCores.push_back(EPC_MODPLAYER);
  }
#endif
  
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

EPLAYERCORES CPlayerCoreFactory::SelectPlayerDialog(VECPLAYERCORES &vecCores, float posX, float posY)
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
  if (posX && posY)
    pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
  else
    pMenu->CenterWindow();
  pMenu->DoModal();

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

EPLAYERCORES CPlayerCoreFactory::SelectPlayerDialog(float posX, float posY)
{
  VECPLAYERCORES vecCores;
  GetPlayers(vecCores);
  return SelectPlayerDialog(vecCores, posX, posY);
}
