#include "application.h"
#include "xbox\iosupport.h"
#include "settings.h"
#include "util.h"
#include "sectionLoader.h"
#include "texturemanager.h"
#include "stdstring.h"
#include "cores/playercorefactory.h"
#include "playlistplayer.h"
#include "musicdatabase.h"
#include "url.h"
#include "autorun.h"
#include "ActionManager.h"
#ifdef _DEBUG
//	#pragma comment (lib,"lib/filezilla/xbfilezillad.lib") // SECTIONNAME=FILEZILL
	#pragma comment (lib,"xbmc/lib/libXBMS/libXBMSd.lib")    // SECTIONNAME=LIBXBMS
	#pragma comment (lib,"xbmc/lib/libsmb/libsmbd.lib")      // SECTIONNAME=LIBSMB
	#pragma comment (lib,"xbmc/lib/cximage/ImageLibd.lib")   // SECTIONNAME=CXIMAGE
	#pragma comment (lib,"xbmc/lib/libID3/i3dlibd.lib")			 // SECTIONNAME=LIBID3
	#pragma comment (lib,"xbmc/lib/libCDRip/cdripd.lib")		 // SECTIONNAME=LIBCDRIP
	#pragma comment (lib,"xbmc/lib/libPython/pythond.lib")	 // SECTIONNAME=PYTHON/PY_RW
	#pragma comment (lib,"xbmc/lib/libGoAhead/goaheadd.lib") // SECTIONNAME=LIBHTTP
	#pragma comment (lib,"xbmc/lib/sqlLite/libSQLited.lib")    
	#pragma comment (lib,"guilib/debug/guiLib.lib")				   // -
	#pragma comment (lib,"xbmc/cores/dllLoader/debug/dllloader.lib")				   // -
	#pragma comment (lib, "xbmc/lib/libcdio/libcdiod.lib" )
	#pragma comment (lib, "xbmc/lib/libshout/libshoutd.lib" )
#else
//  #pragma comment (lib,"lib/filezilla/xbfilezilla.lib")
	#pragma comment (lib,"xbmc/lib/libXBMS/libXBMS.lib")          
  #pragma comment (lib,"xbmc/lib/libsmb/libsmb.lib")           
	#pragma comment (lib,"xbmc/lib/cximage/ImageLib.lib")
	#pragma comment (lib,"xbmc/lib/libID3/i3dlib.lib")					
	#pragma comment (lib,"xbmc/lib/libCDRip/cdrip.lib")						
	#pragma comment (lib,"xbmc/lib/libPython/python.lib")		 
	#pragma comment (lib,"xbmc/lib/libGoAhead/goahead.lib")
	#pragma comment (lib,"xbmc/lib/sqlLite/libSQLite.lib")    
	#pragma comment (lib,"guiLib/release/guiLib.lib")
	#pragma comment (lib,"xbmc/cores/dllLoader/release/dllloader.lib")				   // -
	#pragma comment (lib, "xbmc/lib/libcdio/libcdio.lib" )
	#pragma comment (lib, "xbmc/lib/libshout/libshout.lib" )
#endif


CApplication::CApplication(void)
:m_ctrDpad(220,220)
,m_ctrIR(220,220)
{
		m_bSpinDown=true;
		m_bOverlayEnabled=true;
		m_pWebServer=NULL;
		m_pFileZilla=NULL;
		m_pPlayer=NULL;
		XSetProcessQuantumLength(5); //default=20msec
		XSetFileCacheSize (256*1024);//default=64kb
		m_dwSpinDownTime=timeGetTime();		
}	

CApplication::~CApplication(void)
{
	
}

HRESULT CApplication::Create()
{
	CStdString strPath;
	CUtil::GetHomePath(strPath);
	
  CIoSupport helper;
	
	{
		CHAR szDevicePath[1024];
		helper.Mount("Q:","Harddisk0\\Partition2");

		helper.GetPartition(strPath.c_str(),szDevicePath);
		strcat(szDevicePath,&strPath.c_str()[2]);
		helper.Unmount("Q:");
		helper.Mount("Q:",szDevicePath);	
	}


	string strSkinPath=strPath;
	strSkinPath+=CStdString("\\skin");

	// Transfer the resolution information to our graphics context
	g_graphicsContext.SetD3DParameters(&m_d3dpp, g_settings.m_ResInfo);
	m_bSettingsLoaded=g_settings.Load();
	helper.Remap("C:,Harddisk0\\Partition2");
	helper.Remap("E:,Harddisk0\\Partition1");
	helper.Remount("D:","Cdrom0");

	if ( g_stSettings.m_bUseFDrive )
	{
		helper.Remap("F:,Harddisk0\\Partition6");
	}
	if ( g_stSettings.m_bUseGDrive )
	{
		helper.Remap("G:,Harddisk0\\Partition7"); // used for the LBA-48 hack allowing >120 gig
	}

	CStdString strLanguagePath;
	strLanguagePath.Format("%s\\language\\%s\\strings.xml", strPath.c_str(),g_stSettings.szDefaultLanguage);
	g_localizeStrings.Load(strLanguagePath );
	g_buttonTranslator.Load();

	g_graphicsContext.SetGUIResolution(g_stSettings.m_ScreenResolution);
	g_graphicsContext.SetOffset(g_stSettings.m_iUIOffsetX, g_stSettings.m_iUIOffsetY);
	m_gWindowManager.Initialize();
	g_actionManager.SetScriptActionCallback(&m_pythonParser);
	return CXBApplicationEx::Create();
}

HRESULT CApplication::Initialize()
{
	CStdString strPath;
	CUtil::GetHomePath(strPath);
	string strSkinPath=strPath;
	strSkinPath+=CStdString("\\skin");
  if (g_stSettings.szThumbnailsDirectory[0]==0)
  {
    strcpy(g_stSettings.szThumbnailsDirectory,strPath.c_str());
    strcat(g_stSettings.szThumbnailsDirectory,"\\thumbs");
  }

  if (g_stSettings.m_szShortcutDirectory[0]==0)
  {
    strcpy(g_stSettings.m_szShortcutDirectory,strPath.c_str());
    strcat(g_stSettings.m_szShortcutDirectory,"\\shortcuts");		
  }
  if (g_stSettings.m_szIMDBDirectory[0]==0)
  {
    strcpy(g_stSettings.m_szIMDBDirectory,strPath.c_str());
    strcat(g_stSettings.m_szIMDBDirectory,"\\imdb");		
  }
  if (g_stSettings.m_szAlbumDirectory[0]==0)
  {
    strcpy(g_stSettings.m_szAlbumDirectory,strPath.c_str());
    strcat(g_stSettings.m_szAlbumDirectory,"\\albums");		
  }
	if (g_stSettings.m_szMusicRecordingDirectory[0]==0)
  {
    strcpy(g_stSettings.m_szMusicRecordingDirectory,strPath.c_str());
    strcat(g_stSettings.m_szMusicRecordingDirectory,"\\recordings");		
  }
	CreateDirectory(g_stSettings.szThumbnailsDirectory,NULL);
	CreateDirectory(g_stSettings.m_szShortcutDirectory,NULL);
	CreateDirectory(g_stSettings.m_szIMDBDirectory,NULL);
	CreateDirectory(g_stSettings.m_szAlbumDirectory,NULL);
	CreateDirectory(g_stSettings.m_szMusicRecordingDirectory,NULL);
	string strDir=strPath;
	string strAlbumDir=g_stSettings.m_szAlbumDirectory;
	CreateDirectory((strAlbumDir+"\\playlists").c_str(),NULL);
	CreateDirectory((strAlbumDir+"\\cddb").c_str(),NULL);
	CreateDirectory((strAlbumDir+"\\thumbs").c_str(),NULL); // contains the album thumbs
	CreateDirectory((strDir+"\\python").c_str(),NULL);
	CreateDirectory((strDir+"\\scripts").c_str(),NULL);
	CreateDirectory((strDir+"\\language").c_str(),NULL);
	CreateDirectory((strDir+"\\visualisations").c_str(),NULL);

  if ( strlen(g_stSettings.m_szAlternateSubtitleDirectory) )
  {
    CreateDirectory(g_stSettings.m_szAlternateSubtitleDirectory,NULL);
  }

  // initialize network
  
  if ( CUtil::InitializeNetwork(g_stSettings.m_strLocalIPAdres,
                            g_stSettings.m_strLocalNetmask,
                            g_stSettings.m_strGateway ) )
  {
			m_sntpClient.Create(); 
	
			if (g_stSettings.m_bHTTPServerEnabled)
			{
				CSectionLoader::Load("LIBHTTP");
				m_pWebServer = new CWebServer();
				CStdString ipadres;
				CUtil::GetTitleIP(ipadres);
				m_pWebServer->Start(ipadres.c_str(), 80, "Q:\\web");
			}  
			
      if (g_stSettings.m_bFTPServerEnabled)
			{
				m_pFileZilla = new CXBFileZilla("Q:\\");
				m_pFileZilla->Start();
			}

  }
	g_graphicsContext.SetD3DDevice(m_pd3dDevice);
	LoadSkin(g_stSettings.szDefaultSkin);
  m_gWindowManager.Add(&m_guiHome);											// window id = 0
  m_gWindowManager.Add(&m_guiPrograms);									// window id = 1
	m_gWindowManager.Add(&m_guiPictures);									// window id = 2
  m_gWindowManager.Add(&m_guiMyFiles);									// window id = 3
	m_gWindowManager.Add(&m_guiMyVideo);									// window id = 6
	m_gWindowManager.Add(&m_guiSettings);									// window id = 4
	m_gWindowManager.Add(&m_guiSystemInfo);								// window id = 7
	m_gWindowManager.Add(&m_guiSettingsGeneral);					// window id = 8
	m_gWindowManager.Add(&m_guiSettingsScreen);						// window id = 9
	m_gWindowManager.Add(&m_guiSettingsUICalibration);		// window id = 10
	m_gWindowManager.Add(&m_guiSettingsScreenCalibration);	// window id = 11
	m_gWindowManager.Add(&m_guiSettingsSlideShow);				// window id = 12 slideshow:window id 2007
	m_gWindowManager.Add(&m_guiSettingsFilter);						// window id = 13
	m_gWindowManager.Add(&m_guiSettingsMusic);						// window id = 14
  m_gWindowManager.Add(&m_guiSettingsSubtitles);				// window id = 15
	m_gWindowManager.Add(&m_guiScripts);									// window id = 20

  m_gWindowManager.Add(&m_guiDialogYesNo);							// window id = 100
  m_gWindowManager.Add(&m_guiDialogProgress);						// window id = 101
  m_gWindowManager.Add(&m_guiMyMusicPlayList);					// window id = 500
	m_gWindowManager.Add(&m_guiMyMusicSongs);							// window id = 501
  m_gWindowManager.Add(&m_guiMyMusicAlbum);							// window id = 502
	m_gWindowManager.Add(&m_guiMyMusicArtists);						// window id = 503
	m_gWindowManager.Add(&m_guiMyMusicGenres);						// window id = 504
	m_gWindowManager.Add(&m_guiMyMusicTop100);						// window id = 505
  m_gWindowManager.Add(&m_keyboard);										// window id = 1000
	m_gWindowManager.Add(&m_guiDialogSelect);							// window id = 2000
	m_gWindowManager.Add(&m_guiMusicInfo);								// window id = 2001
	m_gWindowManager.Add(&m_guiDialogOK);									// window id = 2002
	m_gWindowManager.Add(&m_guiVideoInfo);								// window id = 2003
	m_gWindowManager.Add(&m_guiScriptsInfo);							// window id = 2004
	m_gWindowManager.Add(&m_guiWindowFullScreen);					// window id = 2005
	m_gWindowManager.Add(&m_guiWindowVisualisation);			// window id = 2006
	m_gWindowManager.Add(&m_guiWindowSlideshow);					// window id = 2007

	/* window id's 3000 - 3100 are reserved for python */
  	
  m_keyboard.Initialize();
	m_ctrDpad.SetDelays(g_stSettings.m_iMoveDelayController,g_stSettings.m_iRepeatDelayController);
	m_ctrIR.SetDelays(g_stSettings.m_iMoveDelayIR,g_stSettings.m_iRepeatDelayIR);

	m_gWindowManager.ActivateWindow(g_stSettings.m_iStartupWindow);
	CUtil::RemoveTempFiles();

	//	Start Thread for DVD Mediatype detection
	m_DetectDVDType.Create( false);


  if (!m_bSettingsLoaded)
  {
    m_guiDialogOK.SetHeading(279);
    m_guiDialogOK.SetLine(0,280);
    m_guiDialogOK.SetLine(1,281);
    m_guiDialogOK.SetLine(2,L"");
    m_guiDialogOK.DoModal(g_stSettings.m_iStartupWindow);
  }

	return S_OK;
}

void CApplication::LoadSkin(const CStdString& strSkin)
{
	CStdString strHomePath;
	string strSkinPath;
	CUtil::GetHomePath(strHomePath);
	strSkinPath=strHomePath;
	strSkinPath+="\\skin\\";
	strSkinPath+=strSkin;

	m_guiWindowVideoOverlay.FreeResources();
	m_guiWindowVideoOverlay.ClearAll();
	
	m_guiMusicOverlay.FreeResources();
	m_guiMusicOverlay.ClearAll();
	
	m_gWindowManager.DeInitialize();
	g_TextureManager.Cleanup();

	g_fontManager.Clear();
  g_graphicsContext.SetMediaDir(strSkinPath);
  g_fontManager.LoadFonts(strSkinPath+string("\\font.xml")) ;

  m_guiHome.Load( strSkinPath+"\\home.xml" );  
  m_guiPrograms.Load( strSkinPath+"\\myprograms.xml" );  
	m_guiPictures.Load( strSkinPath+"\\mypics.xml" );  
	m_guiMyFiles.Load( strSkinPath+"\\myfiles.xml" );  
	m_guiMyVideo.Load( strSkinPath+"\\myvideo.xml" );  
	m_guiSettings.Load( strSkinPath+"\\settings.xml" );  
	m_guiSystemInfo.Load( strSkinPath+"\\SettingsSystemInfo.xml" );  
	m_guiMusicInfo.Load( strSkinPath+"\\DialogAlbumInfo.xml" );  
	m_guiScriptsInfo.Load( strSkinPath+"\\DialogScriptInfo.xml" ); 
	m_guiSettingsGeneral.Load( strSkinPath+"\\SettingsGeneral.xml" );  
	m_guiDialogYesNo.Load( strSkinPath+"\\dialogYesNo.xml" );  
	m_guiDialogProgress.Load( strSkinPath+"\\dialogProgress.xml" );  
  m_guiMyMusicPlayList.Load( strSkinPath+"\\mymusicplaylist.xml" );
	m_guiMyMusicSongs.Load( strSkinPath+"\\mymusicsongs.xml" );
  m_guiMyMusicAlbum.Load( strSkinPath+"\\mymusicalbum.xml" );
	m_guiMyMusicArtists.Load( strSkinPath+"\\mymusicartists.xml" );
	m_guiMyMusicGenres.Load( strSkinPath+"\\mymusicgenres.xml" );
	m_guiMyMusicTop100.Load( strSkinPath+"\\mymusictop100.xml" );
	m_guiDialogSelect.Load( strSkinPath+"\\dialogSelect.xml" );  
	m_guiDialogOK.Load( strSkinPath+"\\dialogOK.xml" );  
	m_guiVideoInfo.Load( strSkinPath+"\\DialogVideoInfo.xml" );  
	m_guiMusicOverlay.Load( strSkinPath+"\\musicOverlay.xml" );  
	m_guiSettingsScreen.Load( strSkinPath+"\\settingsScreen.xml" );
	m_guiSettingsUICalibration.Load( strSkinPath+"\\settingsUICalibration.xml" );
	m_guiSettingsScreenCalibration.Load( strSkinPath+"\\settingsScreenCalibration.xml" );
	m_guiSettingsSlideShow.Load( strSkinPath+"\\SettingsSlideShow.xml" );
	m_guiSettingsFilter.Load( strSkinPath+"\\SettingsFilter.xml" );
	m_guiWindowVideoOverlay.Load( strSkinPath+"\\videoOverlay.xml" );
	m_guiWindowFullScreen.Load( strSkinPath+"\\videoFullScreen.xml" );
	m_guiScripts.Load( strSkinPath+"\\myscripts.xml");
	m_guiWindowVisualisation.Load( strSkinPath+"\\musicVisualisation.xml");
	m_guiSettingsMusic.Load( strSkinPath+"\\SettingsMusic.xml");
	m_guiWindowSlideshow.Load( strSkinPath+"\\slideshow.xml");
	m_guiSettingsSubtitles.Load( strSkinPath+"\\SettingsScreenSubtitles.xml");
	m_guiMusicOverlay.AllocResources();
	m_guiWindowVideoOverlay.AllocResources();
	m_gWindowManager.AddMsgTarget(this);
	m_gWindowManager.AddMsgTarget(&g_playlistPlayer);
	m_gWindowManager.SetCallback(*this);

}


void CApplication::Render()
{
  static iBlinkRecord=0;
	// process messages which have to be send to the gui
	// (this can only be done after m_gWindowManager.Render())
	g_applicationMessenger.ProcessWindowMessages();
	// process any Python scripts
	m_pythonParser.Process();

	SpinHD();
	// process messages, even if a movie is playing
	g_applicationMessenger.ProcessMessages();
	
	// dont show GUI when playing full screen video
	if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
	{
		if ( g_graphicsContext.IsFullScreenVideo() ) 
		{
			// set soften on/off
			m_pd3dDevice->SetSoftDisplayFilter(g_stSettings.m_bSoften);
			// set the flicker filter - Changed by JM to improve subtitle flicker (using 1)
			// HDTV modes may not need this, though.
			m_pd3dDevice->SetFlickerFilter(g_stSettings.m_bSoften ? 5 : 1);
			
      if (m_pPlayer)
      {
        if (m_pPlayer->IsPaused()) 
        {
	        g_graphicsContext.Lock();
          m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );
          RenderFullScreen();
          m_gWindowManager.Render();
	        m_pd3dDevice->BlockUntilVerticalBlank();      
	        m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	        g_graphicsContext.Unlock();
          return;
        }
      }
      Sleep(50);
      return;
		}
	}

  if (IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
	{
		g_graphicsContext.EnablePreviewWindow(true);
	}
	else
	{
		g_graphicsContext.EnablePreviewWindow(false);
	}

	g_graphicsContext.Lock();
	// draw GUI (always enable soften filter when displaying the UI)
  m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );
	m_pd3dDevice->SetSoftDisplayFilter(true);
	m_pd3dDevice->SetFlickerFilter(5);

  m_gWindowManager.Render();

	// check if we're playing a file
	if (  m_bOverlayEnabled )
	{
		// yes, then render the music overlay window
		if ( IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
		{
			m_guiWindowVideoOverlay.Render();
		}
		else if ( IsPlayingAudio() ) 
		{
			m_guiMusicOverlay.Render();
		}
	}

  

	{
	  MEMORYSTATUS stat;
	  GlobalMemoryStatus(&stat);
		DWORD dwMegFree=stat.dwAvailPhys / (1024*1024);
		if (dwMegFree <= 10)
		{
			g_TextureManager.Flush();
		}
		if (IsPlayingAudio())
		{
			if (m_pPlayer->IsRecording() )
			{
				CGUIFont* pFont=g_fontManager.GetFont("font13");
				if (pFont)
				{
					iBlinkRecord++;
					if (iBlinkRecord>25)
						pFont->DrawText(60, 50, 0xffff0000, L"REC"); //Draw REC in RED
					if (iBlinkRecord> 50)
						iBlinkRecord=0;
				}
			}
		}
#ifdef _DEBUG		
		CStdStringW wszText;
		wszText.Format(L"FreeMem %i/%iMB",stat.dwAvailPhys  /(1024*1024),
																					  stat.dwTotalPhys  /(1024*1024)  );
    
    CGUIFont* pFont=g_fontManager.GetFont("font13");
    if (pFont)
    {
	    pFont->DrawText( 60, 40, 0xffffffff, wszText);
    }
#endif

  }
  // Present the backbuffer contents to the display

	m_pd3dDevice->BlockUntilVerticalBlank();      
	m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	g_graphicsContext.Unlock();

}

void CApplication::OnKey(CKey& key)
{
	CAction action;
	// get the current window to send to
	int iWin = m_gWindowManager.GetActiveWindow();
  g_buttonTranslator.GetAction(iWin, key, action);
	// now translate our key into an action id (Transfer into button translator!!)

	if ( IsPlayingVideo() )
	{
		if (action.wID == ACTION_SHOW_GUI && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
		{
			// switch to fullscreen mode
			OutputDebugString("Flushing Texture Manager\n");
			g_TextureManager.Flush();
			OutputDebugString("Switching to FullScreen\n");
			m_gWindowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
			OutputDebugString("Now in Fullscreen mode\n");
			return;
		}
	}
	else 
	{
		if (IsPlayingAudio() )
		{
			if (action.wID == ACTION_SHOW_GUI && m_gWindowManager.GetActiveWindow() != WINDOW_VISUALISATION)
			{
				m_gWindowManager.ActivateWindow(WINDOW_VISUALISATION);
				return;
			}
		}
	}
	m_gWindowManager.OnAction(action);

	/* handle extra global presses */
  if (iWin != WINDOW_FULLSCREEN_VIDEO)
  {
    if (action.wID == ACTION_STOP)
	  {
      if ( IsPlayingAudio() && !IsPlayingVideo())
      {
        m_pPlayer->closefile();
      }
		  CGUIMessage msg( GUI_MSG_PLAYBACK_STOPPED, 0, 0, 0, 0, NULL );
		  m_gWindowManager.SendThreadMessage( msg );
	  }  
	  if (action.wID == ACTION_PAUSE)
	  {
		  if (m_pPlayer) m_pPlayer->Pause();
	  }
	  if (action.wID == ACTION_PREV_ITEM)
	  {
		  g_playlistPlayer.PlayPrevious();
	  }
	  if (action.wID == ACTION_NEXT_ITEM)
	  {
		  g_playlistPlayer.PlayNext();
	  }
  }
}

void CApplication::FrameMove()
{
	ReadInput();
	XBIR_REMOTE* pRemote	= &m_DefaultIR_Remote;
	XBGAMEPAD*  pGamepad	= &m_DefaultGamepad;

	WORD wButtons = pGamepad->wButtons;
	WORD wRemotes = pRemote->wButtons;
	WORD wDpad = wButtons&(XINPUT_GAMEPAD_DPAD_UP|XINPUT_GAMEPAD_DPAD_DOWN|XINPUT_GAMEPAD_DPAD_LEFT|XINPUT_GAMEPAD_DPAD_RIGHT);

	BYTE bLeftTrigger = pGamepad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER];
	BYTE bRightTrigger = pGamepad->bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER];
	
	WORD wDir = m_ctrDpad.DpadInput(wDpad,0!=bLeftTrigger,0!=bRightTrigger);
	wRemotes=m_ctrIR.IRInput(wRemotes);

	bool bGotKey=false;

	if (pGamepad->fX1 || pGamepad->fY1)
	{
		bGotKey=true;
		CKey key(KEY_BUTTON_LEFT_THUMB_STICK,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
		OnKey(key);   
	}

	if (pGamepad->fX2 || pGamepad->fY2)
	{
		bGotKey=true;
		CKey key(KEY_BUTTON_RIGHT_THUMB_STICK,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
		OnKey(key);   
	}

	if ( wDir & DC_LEFTTRIGGER)
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_LEFT_TRIGGER,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_RIGHTTRIGGER)
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_RIGHT_TRIGGER,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_LEFT )
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_DPAD_LEFT,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_RIGHT)
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_DPAD_RIGHT,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_UP )
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_DPAD_UP,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_DOWN )
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_DPAD_DOWN,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }

  
	if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_BACK )
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_BACK,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
	if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_START)
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_START,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }


	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_A])
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_A,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_B])
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_B,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);
  }
  
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_X])
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_X,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);
		
  }
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_Y])
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_Y,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  } 
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK])
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_BLACK,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  } 
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE])
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_WHITE,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  } 
	
  switch (wRemotes)
	{
		// 0 is invalid
		case 0:
			break;
		// Map all other keys unchanged
		default:
		{
			bGotKey=true;
			CKey key(wRemotes);
			OnKey(key);   
			break;
		}
	}

	if (bGotKey) 
	{
		m_dwSpinDownTime=timeGetTime();
	}

	m_Autorun.HandleAutorun();

	m_gWindowManager.DispatchThreadMessages();

}

void CApplication::Stop()
{
	if (m_pWebServer)
	{
		m_pWebServer->Stop();
		delete m_pWebServer;
		m_pWebServer = NULL;
	}

	if (m_pPlayer)
	{
		delete m_pPlayer;
		m_pPlayer=NULL;
	}

	g_applicationMessenger.Cleanup();
	m_pythonParser.FreeResources();
	m_DetectDVDType.StopThread();
	m_sntpClient.StopThread();
	m_guiMusicOverlay.FreeResources();
	m_guiWindowVideoOverlay.FreeResources();
	g_fontManager.Clear();
	m_gWindowManager.DeInitialize();
	g_TextureManager.Cleanup();
	CSectionLoader::UnloadAll();
	Destroy();
}

bool CApplication::PlayFile(const CStdString& strFile)
{
	CURL url(strFile);
	CStdString strNewPlayer = "mplayer";
	if ( url.GetProtocol() == "cdda")
	{
		strNewPlayer = "cdda";
	}
	if (m_pPlayer)
	{
		if (1||m_strCurrentPlayer != strNewPlayer || !CUtil::IsAudio(strFile) )
		{
			delete m_pPlayer;
			m_pPlayer=NULL;
		}
	}

	m_strCurrentPlayer=strNewPlayer;
	m_bSpinDown=true;
	if (!m_pPlayer)
	{
		CPlayerCoreFactory factory;
		m_pPlayer = factory.CreatePlayer(strNewPlayer,*this);
	}
	bool bResult=m_pPlayer->openfile(strFile);
	if (bResult) 
	{
		m_guiMusicOverlay.SetCurrentFile(strFile);
		m_guiWindowVideoOverlay.SetCurrentFile(strFile);

		if ( CUtil::IsHD(strFile) )
		{
			m_bSpinDown=false;
		}
	}
	m_dwIdleTime=timeGetTime();
	return bResult;
}

void CApplication::OnPlayBackEnded()
{
	OutputDebugString("Playback has finished\n");
	CGUIMessage msg( GUI_MSG_PLAYBACK_ENDED, 0, 0, 0, 0, NULL );
	m_gWindowManager.SendThreadMessage( msg );
}

void CApplication::OnPlayBackStarted()
{
}

void CApplication::EnableOverlay()
{
	m_bOverlayEnabled=true;
}
void CApplication::DisableOverlay()
{
	m_bOverlayEnabled=false;
}


bool CApplication::IsPlayingAudio() const 
{
	if (!m_pPlayer) return false;
	if (!m_pPlayer->IsPlaying()) return false;
	if (m_pPlayer->HasVideo()) return false;
	if (m_pPlayer->HasAudio()) return true;
	return false;
}

bool CApplication::IsPlayingVideo() const 
{
	if (!m_pPlayer) return false;
	if (!m_pPlayer->IsPlaying()) return false;
	if (m_pPlayer->HasVideo()) return true;
	return false;
}

void CApplication::RenderFullScreen()
{
//	OutputDebugString("RenderFullScreen ...\n");
	if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
	{
//		OutputDebugString("Actually doing RenderFullScreen\n");
		CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)m_gWindowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
		pFSWin->RenderFullScreen();
	}
//	OutputDebugString("RenderFullScreen Done\n");
}

void CApplication::SpinHD()
{
	if (!g_stSettings.m_iHDSpinDownTime) return;// dont do HD spindown
	if (!m_bSpinDown)
	{
		if (!m_pPlayer)  
		{
			m_bSpinDown=true;
		}
		else if (!m_pPlayer->IsPlaying()) 
		{
			m_bSpinDown=true;
		}

		if (m_bSpinDown) 
		{
			m_dwSpinDownTime=timeGetTime();
		}
	}

	// spin down HD after 3 mins of inactivity
	if (m_bSpinDown)
	{
		if ( (long)(timeGetTime() - m_dwSpinDownTime) >= ((long)g_stSettings.m_iHDSpinDownTime*60L*1000L) )
		{
			m_dwSpinDownTime=timeGetTime();
			CIoSupport helper;
			helper.SpindownHarddisk();
		}
	}

	// clean up player core if its inactive >= 10 sec.
	if (m_pPlayer)
	{
		if (!m_pPlayer->IsPlaying() )
		{
			bool bTimeOut=(long)(timeGetTime() - m_dwIdleTime) >= 10L*1000L ;
			// music stopped.
			if (m_gWindowManager.GetActiveWindow()==WINDOW_VISUALISATION)
			{
				if (g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size() ==0 || bTimeOut)
				{
					m_gWindowManager.ActivateWindow(WINDOW_HOME);//home.
				}
			}

			if ( bTimeOut )
			{
				delete m_pPlayer;
				m_pPlayer=NULL;
			}
		}
		else
		{
			m_dwIdleTime=timeGetTime();
		}
	}
}

void CApplication::ResetAllControls()
{
	m_guiMusicOverlay.ResetAllControls();
	m_guiWindowVideoOverlay.ResetAllControls();
}
bool CApplication::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		{
				//	Update general playlist: Remove DVD playlist items
				int nRemoved = g_playlistPlayer.RemoveDVDItems();
				if ( nRemoved > 0 ) {
					CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
					m_gWindowManager.SendMessage( msg );
				}
		}
		break;

		case GUI_MSG_PLAYBACK_ENDED:
		{
			m_dwSpinDownTime=timeGetTime();
			m_dwIdleTime=timeGetTime();
			if (m_pPlayer) 
			{
				g_playlistPlayer.PlayNext(true);
			}
			if (!IsPlayingVideo() && m_gWindowManager.GetActiveWindow()==WINDOW_FULLSCREEN_VIDEO)
				m_gWindowManager.PreviousWindow();
		}
		break;

		case GUI_MSG_PLAYLIST_PLAY_NEXT_PREV:
		{
			if (message.GetParam1()==PLAYLIST_MUSIC || message.GetParam1()==PLAYLIST_MUSIC_TEMP)
			{
				CPlayList::CPlayListItem* pItem=(CPlayList::CPlayListItem*)message.GetLPVOID();
				if (pItem)
				{
					//	only Increment Top 100 Counter, if we have not clicked from inside the Top 100 view
					if (g_stSettings.m_iMyMusicStartWindow==WINDOW_MUSIC_TOP100)
					{
						break;
					}
					else
					{
						CMusicDatabase db;
						if (db.Open())
						{
							db.IncrTop100CounterByFileName(pItem->GetFileName());
							db.Close();
						}
					}
				}
			}
		}
		break;
	}

	return true;
}
