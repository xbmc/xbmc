#include "application.h"
#include "xbox\iosupport.h"
#include "settings.h"
#include "util.h"
#include "sectionLoader.h"
#include "texturemanager.h"
#include "stdstring.h"
#include "cores/playercorefactory.h"
#include "playlistplayer.h"

using namespace PLAYLIST;

#ifdef _DEBUG
//	#pragma comment (lib,"lib/filezilla/xbfilezillad.lib") // SECTIONNAME=FILEZILL
	#pragma comment (lib,"xbmc/lib/libXBMS/libXBMSd.lib")    // SECTIONNAME=LIBXBMS
	#pragma comment (lib,"xbmc/lib/libsmb/libsmbd.lib")      // SECTIONNAME=LIBSMB
	#pragma comment (lib,"xbmc/lib/cximage/ImageLibd.lib")   // SECTIONNAME=CXIMAGE
	#pragma comment (lib,"xbmc/lib/libID3/i3dlibd.lib")			 // SECTIONNAME=LIBID3
	#pragma comment (lib,"xbmc/lib/libCDRip/cdripd.lib")		 // SECTIONNAME=LIBCDRIP
	#pragma comment (lib,"xbmc/lib/libPython/pythond.lib")	 // SECTIONNAME=PYTHON
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
	#pragma comment (lib,"xbmc/lib/sqlLite/libSQLite.lib")    
	#pragma comment (lib,"guiLib/release/guiLib.lib")
	#pragma comment (lib,"xbmc/cores/dllLoader/release/dllloader.lib")				   // -
	#pragma comment (lib, "xbmc/lib/libcdio/libcdio.lib" )
#endif


CApplication::CApplication(void)
:m_ctrDpad(220,220)
,m_ctrIR(220,220)
{
		m_bOverlayEnabled=true;
		m_pPhytonParser=NULL;
		m_pPlayer=NULL;
		XSetProcessQuantumLength(5);
}	

CApplication::~CApplication(void)
{
}

HRESULT CApplication::Create()
{
	CStdString strPath;
	CUtil::GetHomePath(strPath);
	
  CIoSupport helper;
  helper.Remap("C:,Harddisk0\\Partition2");
  helper.Remap("E:,Harddisk0\\Partition1");
  helper.Remap("F:,Harddisk0\\Partition6");
	
	{
		CHAR szDevicePath[1024];
		helper.Unmount("Q:");
		helper.Mount("Q:","Harddisk0\\Partition2");

		helper.GetPartition(strPath.c_str(),szDevicePath);
		strcat(szDevicePath,&strPath.c_str()[2]);
		helper.Unmount("Q:");
		helper.Mount("Q:",szDevicePath);	
	}

	string strSkinPath=strPath;
	strSkinPath+=CStdString("\\skin");
	
  g_localizeStrings.Load(strSkinPath+string("\\strings.xml") );

	g_settings.Load();
	DWORD dwStandard=XGetVideoStandard();
	DWORD dwFlags=XGetVideoFlags();

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
	string strDir=g_stSettings.m_szAlbumDirectory;
	CreateDirectory((strDir+"\\playlists").c_str(),NULL);
	CreateDirectory((strDir+"\\thumbs").c_str(),NULL); // contains the album thumbs
	CreateDirectory((strDir+"\\cddb").c_str(),NULL);

	//	Start Thread for DVD Mediatype detection
	m_DetectDVDType.Create( true );

  // initialize network
  
  if ( CUtil::InitializeNetwork(g_stSettings.m_strLocalIPAdres,
                            g_stSettings.m_strLocalNetmask,
                            g_stSettings.m_strGateway ) )
  {
			m_sntpClient.Create();                      
    
  }
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

  m_gWindowManager.Add(&m_guiDialogYesNo);							// window id = 100
  m_gWindowManager.Add(&m_guiDialogProgress);						// window id = 101
  m_gWindowManager.Add(&m_keyboard);										// window id = 1000
	m_gWindowManager.Add(&m_guiDialogSelect);							// window id = 2000
	m_gWindowManager.Add(&m_guiMusicInfo);								// window id = 2001
	m_gWindowManager.Add(&m_guiDialogOK);									// window id = 2002
	m_gWindowManager.Add(&m_guiVideoInfo);								// window id = 2003
	
  g_graphicsContext.Set(m_pd3dDevice,m_d3dpp.BackBufferWidth,m_d3dpp.BackBufferHeight, g_stSettings.m_iUIOffsetX, g_stSettings.m_iUIOffsetY, (m_d3dpp.Flags&D3DPRESENTFLAG_WIDESCREEN) !=0 );
  m_keyboard.Initialize();
	m_ctrDpad.SetDelays(g_stSettings.m_iMoveDelayController,g_stSettings.m_iRepeatDelayController);
	m_ctrIR.SetDelays(g_stSettings.m_iMoveDelayIR,g_stSettings.m_iRepeatDelayIR);

	m_gWindowManager.SetCallback(*this);
	m_gWindowManager.ActivateWindow(g_stSettings.m_iStartupWindow);
	m_guiMusicOverlay.AllocResources();
	m_guiWindowVideoOverlay.AllocResources();
	m_guiWindowFullScreen.AllocResources();
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
	m_guiMusicOverlay.FreeResources();
	m_guiWindowFullScreen.FreeResources();
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
	m_guiWindowVideoOverlay.Load( strSkinPath+"\\videoOverlay.xml" );
}


void CApplication::Render()
{
	// process any phyton scripts...
	ProcessScripts();

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
	m_pd3dDevice->SetSoftDisplayFilter(false);
	m_pd3dDevice->SetFlickerFilter(5);

  m_gWindowManager.Render();

	// check if we're playing a file
	if (  m_bOverlayEnabled )
	{
		// yes, then render the music overlay window
		if ( IsPlayingVideo() )
			m_guiWindowVideoOverlay.Render();
		else if ( IsPlayingAudio() ) 
			m_guiMusicOverlay.Render();
	}

  {
	  MEMORYSTATUS stat;
	  GlobalMemoryStatus(&stat);
		CStdStringW wszText;
		wszText.Format(L"FreeMem %i/%iMB",stat.dwAvailPhys  /(1024*1024),
																					  stat.dwTotalPhys  /(1024*1024)  );

    
    CGUIFont* pFont=g_fontManager.GetFont("font13");
    if (pFont)
    {
	    pFont->DrawText( 60, 40, 0xffffffff, wszText);
    }

  }
  // Present the backbuffer contents to the display

	m_pd3dDevice->BlockUntilVerticalBlank();      
	m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	g_graphicsContext.Unlock();
		
}

void CApplication::OnKey(CKey& key)
{
	if (g_application.IsPlayingVideo() )
	{
		if ( key.GetButtonCode() == KEY_BUTTON_X  || key.GetButtonCode() == KEY_REMOTE_MENU)
		{
			// switch between fullscreen & normal video screen
			g_graphicsContext.Lock();
			g_graphicsContext.SetFullScreenVideo( !g_graphicsContext.IsFullScreenVideo() );
			g_graphicsContext.Unlock();
			m_pd3dDevice->BlockUntilVerticalBlank();      
			Sleep(50);
			m_pd3dDevice->BlockUntilVerticalBlank();      

			return;
		}
		if (g_graphicsContext.IsFullScreenVideo())
		{
			m_guiWindowFullScreen.OnKey(key);
			return;
		}
	}
	m_gWindowManager.OnKey(key);   
}

void CApplication::FrameMove()
{
  ReadInput();
	XBIR_REMOTE* pRemote	= &m_DefaultIR_Remote;
  XBGAMEPAD*  pGamepad	= &m_DefaultGamepad;

	WORD wButtons = pGamepad->wButtons;
	WORD wRemotes = pRemote->wPressedButtons;
	WORD wDpad = wButtons&(XINPUT_GAMEPAD_DPAD_UP|XINPUT_GAMEPAD_DPAD_DOWN|XINPUT_GAMEPAD_DPAD_LEFT|XINPUT_GAMEPAD_DPAD_RIGHT);
	
	WORD wDir = m_ctrDpad.DpadInput(wDpad,0!=pGamepad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER],0!=pGamepad->bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER]);
	wDir |= m_ctrIR.IRInput(wRemotes);

	if ( pGamepad->fX1 || pGamepad->fY1 || pGamepad->fX2 || pGamepad->fY2)
	{
    CKey key(false,KEY_INVALID,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
	}
  if ( wDir & DC_LEFTTRIGGER)
  {
    CKey key(true,KEY_BUTTON_LEFT_TRIGGER,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_RIGHTTRIGGER)
  {
    CKey key(true,KEY_BUTTON_RIGHT_TRIGGER,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_LEFT )
  {
    CKey key(true,KEY_BUTTON_DPAD_LEFT,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_RIGHT)
  {
    CKey key(true,KEY_BUTTON_DPAD_RIGHT,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_UP )
  {
    CKey key(true,KEY_BUTTON_DPAD_UP,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_DOWN )
  {
    CKey key(true,KEY_BUTTON_DPAD_DOWN,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }

  
	if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_BACK )
  {
    CKey key(true,KEY_BUTTON_BACK,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
	if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_START)
  {
    CKey key(true,KEY_BUTTON_START,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }


	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_A])
  {
    CKey key(true,KEY_BUTTON_A,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_B])
  {
    CKey key(true,KEY_BUTTON_B,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);
  }
  
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_X])
  {
    CKey key(true,KEY_BUTTON_X,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);
		
  }
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_Y])
  {
    CKey key(true,KEY_BUTTON_Y,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  } 
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK])
  {
    CKey key(true,KEY_BUTTON_BLACK,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  } 
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE])
  {
    CKey key(true,KEY_BUTTON_WHITE,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  } 
	
  switch (pRemote->wPressedButtons)
	{
		case XINPUT_IR_REMOTE_MENU:
    {
		  CKey key(true,KEY_REMOTE_MENU);
      OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_BACK:
    {
		  CKey key(true,KEY_REMOTE_BACK);
      OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_SELECT:
    {
		  CKey key(true,KEY_REMOTE_SELECT);
      OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_DISPLAY:
    {
		  CKey key(true,KEY_REMOTE_DISPLAY);
      OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_TITLE:
    {
		  CKey key(true,KEY_REMOTE_TITLE);
      OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_INFO:
    {
		  CKey key(true,KEY_REMOTE_INFO);
      OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_PLAY:
    {
		  CKey key(true,KEY_REMOTE_PLAY);
      OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_PAUSE:
    {
		  CKey key(true,KEY_REMOTE_PAUSE);
      OnKey(key);   
			if (m_pPlayer)
			{
				if (m_pPlayer->IsPlaying() )
				{
					m_pPlayer->Pause();
				}
			}
		  break;
    }
		case XINPUT_IR_REMOTE_STOP:
    {
		  CKey key(true,KEY_REMOTE_STOP);
      OnKey(key);   
			if (m_pPlayer)
			{
				if (m_pPlayer->IsPlaying() )
				{
					m_pPlayer->closefile();
				}
			}
		  break;
    }
		case XINPUT_IR_REMOTE_SKIP_MINUS:
    {
		  CKey key(true,KEY_REMOTE_SKIPMINUS);
      OnKey(key);   
			if (m_pPlayer && g_playlistPlayer.size() )
			{
				g_playlistPlayer.PlayPrevious();
			}
		  break;
    }
		case XINPUT_IR_REMOTE_SKIP_PLUS:
    {
		  CKey key(true,KEY_REMOTE_SKIPPLUS);
      OnKey(key);   
			if (m_pPlayer && g_playlistPlayer.size() )
			{
				g_playlistPlayer.PlayNext();
			}
		  break;
    }

	case XINPUT_IR_REMOTE_0:
    {
		  CKey key(true,KEY_REMOTE_0);
      OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_1:
    {
		  CKey key(true,KEY_REMOTE_1);
      OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_2:
    {
		  CKey key(true,KEY_REMOTE_2);
      OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_3:
    {
		  CKey key(true,KEY_REMOTE_3);
      OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_4:
    {
		  CKey key(true,KEY_REMOTE_4);
      OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_5:
    {
		  CKey key(true,KEY_REMOTE_5);
      OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_6:
    {
		  CKey key(true,KEY_REMOTE_6);
      OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_7:
    {
		  CKey key(true,KEY_REMOTE_7);
      OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_8:
    {
		  CKey key(true,KEY_REMOTE_8);
      OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_9:
    {
		  CKey key(true,KEY_REMOTE_9);
      OnKey(key);   
		  break;
    }

    case XINPUT_IR_REMOTE_REVERSE:
    {
		  CKey key(true,KEY_REMOTE_REVERSE);
      OnKey(key);   
		  break;
    }

    case XINPUT_IR_REMOTE_FORWARD:
    {
		  CKey key(true,KEY_REMOTE_FORWARD);
      OnKey(key);   
		  break;
    }

	}
	
}
void CApplication::Stop()
{
	if (m_pPhytonParser)
	{
		ivecScriptIds it=m_vecScriptIds.begin();
		while (it != m_vecScriptIds.end())
		{
			int iScriptId = *it;
			if ( !m_pPhytonParser->isDone(iScriptId) )
			{
				m_pPhytonParser->stopScript(iScriptId);
			}
		}
		delete m_pPhytonParser;
		m_pPhytonParser=NULL;
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
	CSectionLoader::UnloadAll();
	Destroy();
}

void CApplication::ExecuteScript(const CStdString& strScript)
{
	if ( !m_pPhytonParser)
	{
		m_pPhytonParser=new Python(NULL);
		int id=m_pPhytonParser->evalFile( (char*)strScript.c_str() );
		m_vecScriptIds.push_back(id);
	}
}

void CApplication::ProcessScripts()
{
	if ( !m_pPhytonParser) return;
	if ( m_vecScriptIds.size()==0) return;

	ivecScriptIds it=m_vecScriptIds.begin();
	while (it != m_vecScriptIds.end())
	{
		int iScriptId = *it;
		if ( m_pPhytonParser->isDone(iScriptId) )
		{
			it=m_vecScriptIds.erase(it);
		}
		else
		{
			++it;
		}
	}
	if ( m_vecScriptIds.size()==0)
	{
		delete m_pPhytonParser;
		m_pPhytonParser=NULL;
	}
}



bool CApplication::PlayFile(const CStdString& strFile)
{
	if (!m_pPlayer)
	{
		CPlayerCoreFactory factory;
		m_pPlayer = factory.CreatePlayer("mplayer",*this);
	}
	bool bResult=m_pPlayer->openfile(strFile);
	if (bResult) 
	{
		m_guiMusicOverlay.SetCurrentFile(strFile);
		m_guiWindowVideoOverlay.SetCurrentFile(strFile);

		if ( CUtil::IsRemote(strFile) || CUtil::IsDVD(strFile) || CUtil::IsCDDA(strFile) || CUtil::IsISO9660(strFile) )
		{
			CIoSupport helper;
			helper.SpindownHarddisk();
		}
	}
	return bResult;
}

void CApplication::OnPlayBackEnded()
{
	if (!m_pPlayer) return;
	g_playlistPlayer.PlayNext();
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