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
using namespace PLAYLIST;

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
#endif


CApplication::CApplication(void)
:m_ctrDpad(220,220)
,m_ctrIR(220,220)
{
		m_bSpinDown=true;
		m_bOverlayEnabled=true;
		m_pPythonParser=NULL;
		m_pWebServer=NULL;
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

	g_settings.Load();
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
	// HACK FOR BUTTON MAPPING
	g_buttonTranslator.Load();
	// END HACK

	DWORD dwStandard=XGetVideoStandard();
	DWORD dwFlags=XGetVideoFlags();
	bool bCanDoWidescreen = (dwFlags & XC_VIDEO_FLAGS_WIDESCREEN)!=0;	// can widescreen be enabled?

	//	0	PAL 720x576 @ 4:3
	//	1	PAL 720x576 @ 16:9
	//	2	NTSC 720x480 @ 4:3
	//	3	NTSC 720x480 @ 16:9
	//	4	HDTV 1920x1080 @ 16:9
	//	5	HDTV 1280x720  @ 16:9
	
	if (g_stSettings.m_iScreenResolution==4) //HDTV 1920x1080 @ 16:9
	{
		if ( dwFlags&XC_VIDEO_FLAGS_HDTV_1080i)
		{
			m_d3dpp.BackBufferWidth        = 1920;
			m_d3dpp.BackBufferHeight       = 1080;
			m_d3dpp.Flags=D3DPRESENTFLAG_WIDESCREEN;
		}
		else g_stSettings.m_iScreenResolution=-1;
	}
	else if (g_stSettings.m_iScreenResolution==5) //HDTV 1280x720  @ 16:9
	{
		if ( dwFlags&XC_VIDEO_FLAGS_HDTV_720p)
		{
			m_d3dpp.BackBufferWidth        = 1280;
			m_d3dpp.BackBufferHeight       = 720;
			m_d3dpp.Flags=D3DPRESENTFLAG_WIDESCREEN|D3DPRESENTFLAG_PROGRESSIVE;
		}
		else g_stSettings.m_iScreenResolution=-1;
	}

	if (g_stSettings.m_iScreenResolution <= 3)
	{
		if (dwStandard==XC_VIDEO_STANDARD_PAL_I)
		{
			if (g_stSettings.m_iScreenResolution<0 || g_stSettings.m_iScreenResolution>1)
				g_stSettings.m_iScreenResolution=0;
			m_d3dpp.BackBufferWidth        = 720;
			m_d3dpp.BackBufferHeight       = 576;
			if (g_stSettings.m_iScreenResolution==1)
			{
				if (bCanDoWidescreen)
					m_d3dpp.Flags=D3DPRESENTFLAG_WIDESCREEN;
			}
		}
		else
		{
			//ntsc
			if (g_stSettings.m_iScreenResolution<2 || g_stSettings.m_iScreenResolution>3)
				g_stSettings.m_iScreenResolution=2;
			m_d3dpp.BackBufferWidth        = 720;
			m_d3dpp.BackBufferHeight       = 480;
			if (g_stSettings.m_iScreenResolution==3)
			{
				if (bCanDoWidescreen)
					m_d3dpp.Flags=D3DPRESENTFLAG_WIDESCREEN;
			}
		}
	}

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
	CreateDirectory(g_stSettings.szThumbnailsDirectory,NULL);
	CreateDirectory(g_stSettings.m_szShortcutDirectory,NULL);
	CreateDirectory(g_stSettings.m_szIMDBDirectory,NULL);
	CreateDirectory(g_stSettings.m_szAlbumDirectory,NULL);
	string strDir=strPath;
	string strAlbumDir=g_stSettings.m_szAlbumDirectory;
	CreateDirectory((strAlbumDir+"\\playlists").c_str(),NULL);
	CreateDirectory((strAlbumDir+"\\cddb").c_str(),NULL);
	CreateDirectory((strAlbumDir+"\\thumbs").c_str(),NULL); // contains the album thumbs
	CreateDirectory((strDir+"\\python").c_str(),NULL);
	CreateDirectory((strDir+"\\scripts").c_str(),NULL);
	CreateDirectory((strDir+"\\language").c_str(),NULL);
	CreateDirectory((strDir+"\\visualisations").c_str(),NULL);


  // initialize network
  
  if ( CUtil::InitializeNetwork(g_stSettings.m_strLocalIPAdres,
                            g_stSettings.m_strLocalNetmask,
                            g_stSettings.m_strGateway ) )
  {
			m_sntpClient.Create(); 
			
			if (false)//g_stSettings.m_bHTTPServerEnabled)
			{
				CSectionLoader::Load("LIBHTTP");
				m_pWebServer = new CWebServer();
				CStdString ipadres;
				CUtil::GetTitleIP(ipadres);
				m_pWebServer->Start(ipadres.c_str(), 80, "Q:\\web");
			}  
  }
	g_graphicsContext.Set(m_pd3dDevice,m_d3dpp.BackBufferWidth,m_d3dpp.BackBufferHeight, g_stSettings.m_iUIOffsetX, g_stSettings.m_iUIOffsetY, (m_d3dpp.Flags&D3DPRESENTFLAG_WIDESCREEN) !=0 );
	LoadSkin(g_stSettings.szDefaultSkin);
  m_gWindowManager.Add(&m_guiHome);											// window id = 0
  m_gWindowManager.Add(&m_guiPrograms);									// window id = 1
	m_gWindowManager.Add(&m_guiPictures);									// window id = 2
  m_gWindowManager.Add(&m_guiMyFiles);									// window id = 3
  m_gWindowManager.Add(&m_guiMyMusic);									// window id = 5
	m_gWindowManager.Add(&m_guiMyVideo);									// window id = 6
	m_gWindowManager.Add(&m_guiSettings);									// window id = 4
	m_gWindowManager.Add(&m_guiSystemInfo);								// window id = 7
	m_gWindowManager.Add(&m_guiSettingsGeneral);					// window id = 8
	m_gWindowManager.Add(&m_guiSettingsScreen);						// window id = 9
	m_gWindowManager.Add(&m_guiSettingsUICalibration);		// window id = 10
	m_gWindowManager.Add(&m_guiSettingsMovieCalibration);	// window id = 11
	m_gWindowManager.Add(&m_guiSettingsSlideShow);				// window id = 12
	m_gWindowManager.Add(&m_guiSettingsFilter);						// window id = 13
	m_gWindowManager.Add(&m_guiSettingsMusic);						// window id = 14
	m_gWindowManager.Add(&m_guiScripts);									// window id = 20

  m_gWindowManager.Add(&m_guiDialogYesNo);							// window id = 100
  m_gWindowManager.Add(&m_guiDialogProgress);						// window id = 101
  m_gWindowManager.Add(&m_keyboard);										// window id = 1000
	m_gWindowManager.Add(&m_guiDialogSelect);							// window id = 2000
	m_gWindowManager.Add(&m_guiMusicInfo);								// window id = 2001
	m_gWindowManager.Add(&m_guiDialogOK);									// window id = 2002
	m_gWindowManager.Add(&m_guiVideoInfo);								// window id = 2003
	m_gWindowManager.Add(&m_guiWindowVisualisation);			// window id = 2006

	/* window id's 3000 - 3100 are reserved for python */
  	
  m_keyboard.Initialize();
	m_ctrDpad.SetDelays(g_stSettings.m_iMoveDelayController,g_stSettings.m_iRepeatDelayController);
	m_ctrIR.SetDelays(g_stSettings.m_iMoveDelayIR,g_stSettings.m_iRepeatDelayIR);

	m_gWindowManager.ActivateWindow(g_stSettings.m_iStartupWindow);
	CUtil::RemoveTempFiles();

	//	Start Thread for DVD Mediatype detection
	m_DetectDVDType.Create( false);

	//this->ExecutePythonScript("q:\\scripts\\medusa\\start_medusa.py");

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
	
	m_guiWindowFullScreen.FreeResources();
	m_guiWindowFullScreen.ClearAll();

	m_gWindowManager.DeInitialize();
	g_TextureManager.Cleanup();

	g_fontManager.Clear();
  g_graphicsContext.SetMediaDir(strSkinPath);
  g_fontManager.LoadFonts(strSkinPath+string("\\font.xml")) ;

  m_guiHome.Load( strSkinPath+"\\home.xml" );  
  m_guiPrograms.Load( strSkinPath+"\\myprograms.xml" );  
	m_guiPictures.Load( strSkinPath+"\\mypics.xml" );  
	m_guiMyFiles.Load( strSkinPath+"\\myfiles.xml" );  
	m_guiMyMusic.Load( strSkinPath+"\\mymusic.xml" );  
	m_guiMyVideo.Load( strSkinPath+"\\myvideo.xml" );  
	m_guiSettings.Load( strSkinPath+"\\settings.xml" );  
	m_guiSystemInfo.Load( strSkinPath+"\\SettingsSystemInfo.xml" );  
	m_guiMusicInfo.Load( strSkinPath+"\\DialogAlbumInfo.xml" );  
	m_guiSettingsGeneral.Load( strSkinPath+"\\SettingsGeneral.xml" );  
	m_guiDialogYesNo.Load( strSkinPath+"\\dialogYesNo.xml" );  
	m_guiDialogProgress.Load( strSkinPath+"\\dialogProgress.xml" );  
	m_guiDialogSelect.Load( strSkinPath+"\\dialogSelect.xml" );  
	m_guiDialogOK.Load( strSkinPath+"\\dialogOK.xml" );  
	m_guiVideoInfo.Load( strSkinPath+"\\DialogVideoInfo.xml" );  
	m_guiMusicOverlay.Load( strSkinPath+"\\musicOverlay.xml" );  
	m_guiSettingsScreen.Load( strSkinPath+"\\settingsScreen.xml" );
  m_guiSettingsUICalibration.Load( strSkinPath+"\\settingsUICalibration.xml" );
	m_guiSettingsMovieCalibration.Load( strSkinPath+"\\settingsMovieCalibration.xml" );
	m_guiSettingsSlideShow.Load( strSkinPath+"\\SettingsSlideShow.xml" );
	m_guiSettingsFilter.Load( strSkinPath+"\\SettingsFilter.xml" );
	m_guiWindowVideoOverlay.Load( strSkinPath+"\\videoOverlay.xml" );
	m_guiWindowFullScreen.Load( strSkinPath+"\\videoFullScreen.xml" );
	m_guiScripts.Load( strSkinPath+"\\myscripts.xml");
	m_guiWindowVisualisation.Load( strSkinPath+"\\musicVisualisation.xml");
	m_guiSettingsMusic.Load( strSkinPath+"\\SettingsMusic.xml");

	m_guiMusicOverlay.AllocResources();
	m_guiWindowVideoOverlay.AllocResources();
	m_guiWindowFullScreen.AllocResources();
	m_gWindowManager.AddMsgTarget(this);
	m_gWindowManager.SetCallback(*this);

}


void CApplication::Render()
{
	SpinHD();
	// process messages, even if a movie is playing
	g_applicationMessenger.ProcessMessages();
	
	// dont show GUI when playing full screen video
	if ( IsPlayingVideo() )
	{
		if ( g_graphicsContext.IsFullScreenVideo() ) 
		{
			// set soften on/off
			m_pd3dDevice->SetSoftDisplayFilter(g_stSettings.m_bSoften);
			m_pd3dDevice->SetFlickerFilter(g_stSettings.m_bSoften ? 5 : 0);
			Sleep(50);
			return;
		}
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
		if ( IsPlayingVideo() )
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

	// process messages which have to be send to the gui
	// (this can only be done after m_gWindowManager.Render())
	g_applicationMessenger.ProcessMessages(MSG_TYPE_WINDOW);
	// process any Python scripts
	ProcessPythonScripts();
}

void CApplication::OnKey(CKey& key)
{
	// get the current window to send to
	int iWin = m_gWindowManager.GetActiveWindow();
	// now translate our key into an action id (Transfer into button translator!!)
	CAction action;
	g_buttonTranslator.GetAction(iWin, key, action);

	if ( IsPlayingVideo() )
	{
		if (action.wID == ACTION_SHOW_GUI)
		{
			// switch between fullscreen & normal video screen
			g_TextureManager.Flush();
			g_graphicsContext.Lock();
			g_graphicsContext.SetFullScreenVideo( !g_graphicsContext.IsFullScreenVideo() );
			g_graphicsContext.Unlock();
			m_pd3dDevice->BlockUntilVerticalBlank();      
			Sleep(50);
			m_pd3dDevice->BlockUntilVerticalBlank();      
			Sleep(200);
			m_pPlayer->Update();
			return;
		}
		if (g_graphicsContext.IsFullScreenVideo())
		{
			// get the new action for the FullScreenVideo window
			g_buttonTranslator.ReGetAction(WINDOW_FULLSCREEN_VIDEO, action);
			m_guiWindowFullScreen.OnAction(action);
			return;
		}
	}
	else 
	{
		if (IsPlayingAudio() )
		{
			if (action.wID == ACTION_SHOW_GUI)
			{
				if (m_gWindowManager.GetActiveWindow()==WINDOW_VISUALISATION)
				{
					m_gWindowManager.ActivateWindow(WINDOW_HOME);//home.
				}	
				else
				{
					m_gWindowManager.ActivateWindow(WINDOW_VISUALISATION);//visz.
				}
				return;
			}
		}
	}
	m_gWindowManager.OnAction(action);

	// handle extra global presses (FIXME)
	if (action.wID == ACTION_PAUSE)
	{
		m_pPlayer->Pause();
	}
	if (action.wID == ACTION_STOP)
	{
		m_pPlayer->closefile();
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

void CApplication::FrameMove()
{
	ReadInput();
	XBIR_REMOTE* pRemote	= &m_DefaultIR_Remote;
	XBGAMEPAD*  pGamepad	= &m_DefaultGamepad;

	WORD wButtons = pGamepad->wButtons;
	WORD wRemotes = pRemote->wPressedButtons;
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
	if (m_pPythonParser)
	{
		//first load the python sections if not loaded.
		if(!g_sectionLoader.IsLoaded("PYTHON")) g_sectionLoader.Load("PYTHON");
		if(!g_sectionLoader.IsLoaded("PY_RW")) g_sectionLoader.Load("PY_RW");

		ivecScriptIds it=m_vecScriptIds.begin();
		while (it != m_vecScriptIds.end())
		{
			int iScriptId = *it;
			if ( !m_pPythonParser->isDone(iScriptId) )
			{
				m_pPythonParser->stopScript(iScriptId);
			}
			++it;
		}
		delete m_pPythonParser;
		m_pPythonParser=NULL;
	}

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

	m_DetectDVDType.StopThread();
	m_sntpClient.StopThread();
	m_guiMusicOverlay.FreeResources();
	m_guiWindowVideoOverlay.FreeResources();
	g_fontManager.Clear();
	m_gWindowManager.DeInitialize();
	g_TextureManager.Cleanup();
	g_applicationMessenger.Cleanup();
	CSectionLoader::UnloadAll();
	Destroy();
}

void CApplication::ExecutePythonScript(const CStdString& strScript)
{
	 /* PY_RW stay's loaded as longs as m_pPythonParser != NULL.
	  * When someone runs a script for the first time both sections PYTHON and PY_RW
	  * are loaded. After that script has finished only section PYTHON is unloaded
	  * and m_pPythonParser is 'not' deleted.
		* Only delete m_pPythonParser and unload PY_RW if you don't want to use Python
		* anymore
		*/
	if(!g_sectionLoader.IsLoaded("PYTHON")) g_sectionLoader.Load("PYTHON");
	if(!g_sectionLoader.IsLoaded("PY_RW")) g_sectionLoader.Load("PY_RW");

	if (!m_pPythonParser) m_pPythonParser = new XBPython();

	//run script..
	int id=m_pPythonParser->evalFile(strScript.c_str());
	m_vecScriptIds.push_back(id);
}

void CApplication::ProcessPythonScripts()
{
	if ( !m_pPythonParser || m_vecScriptIds.size() == 0) return;

	ivecScriptIds it=m_vecScriptIds.begin();
	while (it != m_vecScriptIds.end())
	{
		int iScriptId = *it;
		if (m_pPythonParser->isDone(iScriptId))
			it=m_vecScriptIds.erase(it);
		else ++it;
	}
	if ( m_vecScriptIds.size()==0)
		// no scripts are running, it's safe to unload section PYTHON now
		g_sectionLoader.Unload("PYTHON");
}

int CApplication::ScriptsSize()
{
	return m_vecScriptIds.size();
}

int CApplication::GetPythonScriptId(int scriptPosition)
{
	return (int)m_vecScriptIds[scriptPosition];
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
		if (m_strCurrentPlayer != strNewPlayer || !CUtil::IsAudio(strFile) )
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


void CApplication::GetD3DParameters(D3DPRESENT_PARAMETERS& params)
{
	memcpy(&params, &m_d3dpp, sizeof(params));
}
void CApplication::RenderFullScreen()
{
	m_guiWindowFullScreen.Render();
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
			if (m_gWindowManager.GetActiveWindow()==2006)
			{
				if (g_playlistPlayer.size() ==0 || bTimeOut)
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
	m_guiWindowFullScreen.ResetAllControls();
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

		case GUI_MSG_PLAYBACK_ENDED:
		{
			m_dwSpinDownTime=timeGetTime();
			m_dwIdleTime=timeGetTime();
			if (m_pPlayer) 
			{
				g_playlistPlayer.PlayNext();
			}
		}
		break;
	}

	return true;
}
