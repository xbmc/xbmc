#include "application.h"
#include "xbox\iosupport.h"
#include "settings.h"
#include "util.h"
#include "sectionLoader.h"
#include "texturemanager.h"
#include "stdstring.h"



#ifdef _DEBUG
//	#pragma comment (lib,"lib/filezilla/xbfilezillad.lib") // SECTIONNAME=FILEZILL
	#pragma comment (lib,"xbmc/lib/libXBMS/libXBMSd.lib")         // SECTIONNAME=LIBXBMS
	#pragma comment (lib,"xbmc/lib/libsmb/libsmbd.lib")           // SECTIONNAME=LIBSMB
	#pragma comment (lib,"xbmc/lib/cximage/ImageLibd.lib")        // SECTIONNAME=CXIMAGE
	#pragma comment (lib,"guilib\\debug\\guiLib.lib")
#else
//  #pragma comment (lib,"lib/filezilla/xbfilezilla.lib")
	#pragma comment (lib,"xbmc/lib/libXBMS/libXBMS.lib")          
  #pragma comment (lib,"xbmc/lib/libsmb/libsmb.lib")           
	#pragma comment (lib,"xbmc/lib/cximage/ImageLib.lib")
	#pragma comment (lib,"guiLib\\release\\guiLib.lib")
#endif


CApplication::CApplication(void)
:m_ctrDpad(220,220)
,m_ctrIR(220,220)
{
  
}

CApplication::~CApplication(void)
{
}

HRESULT CApplication::Initialize()
{

  CIoSupport helper;
  helper.Remap("C:,Harddisk0\\Partition2");
  helper.Remap("E:,Harddisk0\\Partition1");
  helper.Remap("F:,Harddisk0\\Partition6");


	CStdString strPath;
	CUtil::GetHomePath(strPath);
	string strSkinPath=strPath;
	strSkinPath+=CStdString("\\skin");
  g_localizeStrings.Load(strSkinPath+string("\\strings.xml") );

	g_settings.Load();
	strSkinPath+=CStdString("\\");
  strSkinPath+=g_stSettings.szDefaultSkin;

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


  // initialize network
  if (g_stSettings.m_strLocalIPAdres[0]!=0 &&
      g_stSettings.m_strLocalNetmask[0]!=0 &&
      g_stSettings.m_strGateway [0]!=0)
  {
    if ( CUtil::InitializeNetwork(g_stSettings.m_strLocalIPAdres,
                             g_stSettings.m_strLocalNetmask,
                             g_stSettings.m_strGateway ) )
    {
				m_sntpClient.Create();                      
      
    }
  }
  g_graphicsContext.Set(m_pd3dDevice,m_d3dpp.BackBufferWidth,m_d3dpp.BackBufferHeight);
  g_graphicsContext.SetMediaDir(strSkinPath);
  g_fontManager.LoadFonts(strSkinPath+string("\\font.xml")) ;

  m_guiHome.Load( strSkinPath+string("\\home.xml" ));  
  m_guiPrograms.Load( strSkinPath+string("\\myprograms.xml" ));  
	m_guiPictures.Load( strSkinPath+string("\\mypics.xml" ));  
	m_guiMyFiles.Load( strSkinPath+string("\\myfiles.xml" ));  
	m_guiMyMusic.Load( strSkinPath+string("\\mymusic.xml" ));  
	m_guiMyVideo.Load( strSkinPath+string("\\myvideo.xml" ));  
	m_guiSettings.Load( strSkinPath+string("\\settings.xml" ));  

	m_guiDialogYesNo.Load( strSkinPath+string("\\dialogYesNo.xml" ));  
	m_guiDialogProgress.Load( strSkinPath+string("\\dialogProgress.xml" ));  

	m_gWindowManager.SetCallback(*this);
  m_gWindowManager.Add(&m_guiHome);
  m_gWindowManager.Add(&m_guiPrograms);
	m_gWindowManager.Add(&m_guiPictures);
  m_gWindowManager.Add(&m_guiMyFiles);
  m_gWindowManager.Add(&m_guiMyMusic);
	m_gWindowManager.Add(&m_guiMyVideo);
	m_gWindowManager.Add(&m_guiSettings);
  m_gWindowManager.Add(&m_guiDialogYesNo);
  m_gWindowManager.Add(&m_guiDialogProgress);

  m_keyboard.Initialize();
  m_gWindowManager.Add(&m_keyboard);

  m_gWindowManager.ActivateWindow(g_stSettings.m_iStartupWindow);


  return S_OK;
}


void CApplication::Render()
{
      // Clear the backbuffer to a blue color
      m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0xff202020, 1.0f, 0L );
      m_gWindowManager.Render();
      
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

  if ( wDir & DC_LEFTTRIGGER)
  {
    CKey key(true,KEY_BUTTON_LEFT_TRIGGER);
    m_gWindowManager.OnKey(key);   
  }
  if ( wDir & DC_RIGHTTRIGGER)
  {
    CKey key(true,KEY_BUTTON_RIGHT_TRIGGER);
    m_gWindowManager.OnKey(key);   
  }
  if ( wDir & DC_LEFT )
  {
    CKey key(true,KEY_BUTTON_DPAD_LEFT);
    m_gWindowManager.OnKey(key);   
  }
  if ( wDir & DC_RIGHT)
  {
    CKey key(true,KEY_BUTTON_DPAD_RIGHT);
    m_gWindowManager.OnKey(key);   
  }
  if ( wDir & DC_UP )
  {
    CKey key(true,KEY_BUTTON_DPAD_UP);
    m_gWindowManager.OnKey(key);   
  }
  if ( wDir & DC_DOWN )
  {
    CKey key(true,KEY_BUTTON_DPAD_DOWN);
    m_gWindowManager.OnKey(key);   
  }

  
	if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_BACK )
  {
    CKey key(true,KEY_BUTTON_BACK);
    m_gWindowManager.OnKey(key);   
  }
	if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_START)
  {
    CKey key(true,KEY_BUTTON_START);
    m_gWindowManager.OnKey(key);   
  }


	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_A])
  {
    CKey key(true,KEY_BUTTON_A);
    m_gWindowManager.OnKey(key);   
  }
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_B])
  {
    CKey key(true,KEY_BUTTON_B);
    m_gWindowManager.OnKey(key);
  }
  
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_X])
  {
    CKey key(true,KEY_BUTTON_X);
    m_gWindowManager.OnKey(key);
  }
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_Y])
  {
    CKey key(true,KEY_BUTTON_Y);
    m_gWindowManager.OnKey(key);   
  } 
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK])
  {
    CKey key(true,KEY_BUTTON_BLACK);
    m_gWindowManager.OnKey(key);   
  } 
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE])
  {
    CKey key(true,KEY_BUTTON_WHITE);
    m_gWindowManager.OnKey(key);   
  } 
	
  switch (pRemote->wPressedButtons)
	{
		case XINPUT_IR_REMOTE_MENU:
    {
		  CKey key(true,KEY_REMOTE_MENU);
      m_gWindowManager.OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_BACK:
    {
		  CKey key(true,KEY_REMOTE_BACK);
      m_gWindowManager.OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_SELECT:
    {
		  CKey key(true,KEY_REMOTE_SELECT);
      m_gWindowManager.OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_DISPLAY:
    {
		  CKey key(true,KEY_REMOTE_DISPLAY);
      m_gWindowManager.OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_TITLE:
    {
		  CKey key(true,KEY_REMOTE_TITLE);
      m_gWindowManager.OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_INFO:
    {
		  CKey key(true,KEY_REMOTE_INFO);
      m_gWindowManager.OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_PLAY:
    {
		  CKey key(true,KEY_REMOTE_PLAY);
      m_gWindowManager.OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_PAUSE:
    {
		  CKey key(true,KEY_REMOTE_PAUSE);
      m_gWindowManager.OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_STOP:
    {
		  CKey key(true,KEY_REMOTE_STOP);
      m_gWindowManager.OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_SKIP_MINUS:
    {
		  CKey key(true,KEY_REMOTE_SKIPMINUS);
      m_gWindowManager.OnKey(key);   
		  break;
    }
		case XINPUT_IR_REMOTE_SKIP_PLUS:
    {
		  CKey key(true,KEY_REMOTE_SKIPPLUS);
      m_gWindowManager.OnKey(key);   
		  break;
    }

case XINPUT_IR_REMOTE_0:
    {
		  CKey key(true,KEY_REMOTE_0);
      m_gWindowManager.OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_1:
    {
		  CKey key(true,KEY_REMOTE_1);
      m_gWindowManager.OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_2:
    {
		  CKey key(true,KEY_REMOTE_2);
      m_gWindowManager.OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_3:
    {
		  CKey key(true,KEY_REMOTE_3);
      m_gWindowManager.OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_4:
    {
		  CKey key(true,KEY_REMOTE_4);
      m_gWindowManager.OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_5:
    {
		  CKey key(true,KEY_REMOTE_5);
      m_gWindowManager.OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_6:
    {
		  CKey key(true,KEY_REMOTE_6);
      m_gWindowManager.OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_7:
    {
		  CKey key(true,KEY_REMOTE_7);
      m_gWindowManager.OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_8:
    {
		  CKey key(true,KEY_REMOTE_8);
      m_gWindowManager.OnKey(key);   
		  break;
    }
    case XINPUT_IR_REMOTE_9:
    {
		  CKey key(true,KEY_REMOTE_9);
      m_gWindowManager.OnKey(key);   
		  break;
    }

    case XINPUT_IR_REMOTE_REVERSE:
    {
		  CKey key(true,KEY_REMOTE_REVERSE);
      m_gWindowManager.OnKey(key);   
		  break;
    }

    case XINPUT_IR_REMOTE_FORWARD:
    {
		  CKey key(true,KEY_REMOTE_FORWARD);
      m_gWindowManager.OnKey(key);   
		  break;
    }


	}

	
}
void CApplication::Stop()
{
	m_sntpClient.StopThread();
	m_gWindowManager.DeInitialize();
	g_TextureManager.Cleanup();
	CSectionLoader::UnloadAll();
	Destroy();
}
