/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "CdgParser.h"
#include "Application.h"
#include "Util.h"
#include "AudioContext.h"
#include "utils/GUIInfoManager.h"

using namespace MUSIC_INFO;
using namespace XFILE;

//CdgLoader
CCdgLoader::CCdgLoader()
{
  m_strFileName.Empty();
  m_CdgFileState = FILE_NOT_LOADED;
  m_pBuffer = NULL;
  m_uiFileLength = 0;
  m_uiLoadedBytes = 0;
  m_uiStreamChunk = STREAM_CHUNK;
}
CCdgLoader::~CCdgLoader()
{
  StopStream();
}

void CCdgLoader::StreamFile(CStdString strfilename)
{
  CSingleLock lock (m_CritSection);
  m_strFileName = strfilename;
  CUtil::RemoveExtension(m_strFileName);
  m_strFileName += ".cdg";
  CThread::Create(false);
}

void CCdgLoader::StopStream()
{
  CSingleLock lock (m_CritSection);
  CThread::StopThread();
  m_uiLoadedBytes = 0;
  m_CdgFileState = FILE_NOT_LOADED;
  if (m_pBuffer)
    SAFE_DELETE_ARRAY(m_pBuffer);
}

SubCode* CCdgLoader::GetCurSubCode()
{
  CSingleLock lock (m_CritSection);
  SubCode* pFirst = GetFirstLoaded();
  SubCode* pLast = GetLastLoaded();
  if (!pFirst || !pLast || !m_pSubCode) return NULL;
  if (m_pSubCode < pFirst || m_pSubCode > pLast) return NULL;
  return m_pSubCode;
}
bool CCdgLoader::SetNextSubCode()
{
  CSingleLock lock (m_CritSection);
  SubCode* pFirst = GetFirstLoaded();
  SubCode* pLast = GetLastLoaded();
  if (!pFirst || !pLast || !m_pSubCode) return false;
  if (m_pSubCode < pFirst || m_pSubCode >= pLast) return false;
  m_pSubCode++;
  return true;
}
errCode CCdgLoader::GetFileState()
{
  CSingleLock lock (m_CritSection);
  return m_CdgFileState;
}
CStdString CCdgLoader::GetFileName()
{
  CSingleLock lock (m_CritSection);
  return m_strFileName;
}
SubCode* CCdgLoader::GetFirstLoaded()
{
  if (!m_uiLoadedBytes)
    return NULL;
  return (SubCode*) m_pBuffer;
}
SubCode* CCdgLoader::GetLastLoaded()
{
  if (m_uiLoadedBytes < sizeof(SubCode) || m_uiLoadedBytes > m_uiFileLength )
    return NULL;
  return ((SubCode*) (m_pBuffer)) + m_uiLoadedBytes / sizeof(SubCode) - 1;
}
void CCdgLoader::OnStartup()
{
  CSingleLock lock (m_CritSection);
  if (!CFile::Exists(m_strFileName))
  {
    m_CdgFileState = FILE_ERR_NOT_FOUND;
    return ;
  }
  if (!m_File.Open(m_strFileName, TRUE))
  {
    m_CdgFileState = FILE_ERR_OPENING;
    return ;
  }
  m_uiFileLength = (int)m_File.GetLength(); // ASSUMES FILELENGTH IS LESS THAN 2^32 bytes!!!
  if (!m_uiFileLength) return ;
  m_File.Seek(0, SEEK_SET);
  if (m_pBuffer)
    SAFE_DELETE_ARRAY(m_pBuffer);
  m_pBuffer = new BYTE[m_uiFileLength];
  if (!m_pBuffer)
  {
    m_CdgFileState = FILE_ERR_NO_MEM;
    return ;
  }
  m_uiLoadedBytes = 0;
  m_pSubCode = (SubCode*) m_pBuffer;
  m_CdgFileState = FILE_LOADING;
}

void CCdgLoader::Process()
{
  if (m_CdgFileState != FILE_LOADING && m_CdgFileState != FILE_SKIP) return ;

  if (m_uiFileLength < m_uiStreamChunk)
    m_uiLoadedBytes = m_File.Read(m_pBuffer, m_uiFileLength);
  else
  {
    UINT uiNumReadings = m_uiFileLength / m_uiStreamChunk;
    UINT uiRemainder = m_uiFileLength % m_uiStreamChunk;
    UINT uiCurReading = 0;
    while (!CThread::m_bStop && uiCurReading < uiNumReadings)
    {
      m_uiLoadedBytes += m_File.Read(m_pBuffer + m_uiLoadedBytes, m_uiStreamChunk);
      uiCurReading++;
    }
    if (uiRemainder && m_uiLoadedBytes + uiRemainder == m_uiFileLength)
      m_uiLoadedBytes += m_File.Read(m_pBuffer + m_uiLoadedBytes, uiRemainder);
  }
}

void CCdgLoader::OnExit()
{
  m_File.Close();
  if (m_uiFileLength && m_uiLoadedBytes == m_uiFileLength)
    m_CdgFileState = FILE_LOADED;
}
//CdgReader
CCdgReader::CCdgReader()
{
  m_pLoader = NULL;
  m_fStartingTime = 0.0f;
  m_fAVDelay = 0.0f;
  m_uiNumReadSubCodes = 0;
  m_Cdg.ClearDisplay();
}
CCdgReader::~CCdgReader()
{
  StopThread();
}


bool CCdgReader::Attach(CCdgLoader* pLoader)
{
  CSingleLock lock (m_CritSection);
  if (!m_pLoader)
    m_pLoader = pLoader;
  if (!m_pLoader) return false;
  return true;
}
void CCdgReader::DetachLoader()
{
  StopThread();
  CSingleLock lock (m_CritSection);
  m_pLoader = NULL;
}
bool CCdgReader::Start(float fStartTime)
{
  CSingleLock lock (m_CritSection);
  if (!m_pLoader) return false;
  m_fStartingTime = fStartTime;
  SetAVDelay(g_advancedSettings.m_karaokeSyncDelay);
  m_uiNumReadSubCodes = 0;
  m_Cdg.ClearDisplay();
  m_FileState = FILE_LOADED;
  CThread::Create(false);
  return true;
}

void CCdgReader::SetAVDelay(float fDelay)
{
  m_fAVDelay = fDelay;
#ifdef _DEBUG
  m_fAVDelay -= DEBUG_AVDELAY_MOD;
#endif
}
float CCdgReader::GetAVDelay()
{
  return m_fAVDelay;
}
errCode CCdgReader::GetFileState()
{
  CSingleLock lock (m_CritSection);

  if (m_FileState == FILE_SKIP)
    return m_FileState;

  if (!m_pLoader) return FILE_NOT_LOADED;
  return m_pLoader->GetFileState();
}
CCdg* CCdgReader::GetCdg()
{
  return (CCdg*) &m_Cdg;
}

CStdString CCdgReader::GetFileName()
{
  CSingleLock lock (m_CritSection);
  if (m_pLoader)
    return m_pLoader->GetFileName();
  return "";
}
void CCdgReader::ReadUpToTime(float secs)
{
  if (secs < 0) return ;
  if (!(m_pLoader->GetCurSubCode())) return ;

  UINT uiFinalOffset = (UINT) (secs * PARSING_FREQ);
  if ( m_uiNumReadSubCodes >= uiFinalOffset) return ;
  UINT i;
  for (i = m_uiNumReadSubCodes; i <= uiFinalOffset; i++)
  {
    m_Cdg.ReadSubCode(m_pLoader->GetCurSubCode());
    if (m_pLoader->SetNextSubCode())
      m_uiNumReadSubCodes++;
  }
}

void CCdgReader::SkipUpToTime(float secs)
{
  if (secs < 0) return ;
  m_FileState= FILE_SKIP;

  UINT uiFinalOffset = (UINT) (secs * PARSING_FREQ);
  // is this needed?
  if ( m_uiNumReadSubCodes > uiFinalOffset) return ;
  for (UINT i = m_uiNumReadSubCodes; i <= uiFinalOffset; i++)
  {
    //m_Cdg.ReadSubCode(m_pLoader->GetCurSubCode());
    if (m_pLoader->SetNextSubCode())
      m_uiNumReadSubCodes++;
  }
  m_FileState = FILE_LOADING;
}

void CCdgReader::OnStartup()
{}

void CCdgReader::Process()
{
  double fCurTime = 0.0f;
  bool bIsFirstPass = true;
  double fNewTime=0.f;
  CStdString strExt;
  CUtil::GetExtension(m_pLoader->GetFileName(),strExt);
  strExt = m_pLoader->GetFileName().substr(0,m_pLoader->GetFileName().size()-strExt.size());

  while (!CThread::m_bStop)
  {
    CSingleLock lock (m_CritSection);
    double fDiff;
    const CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();
    if (!tag || tag->GetURL().substr(0,strExt.size()) != strExt)
    {
      Sleep(15);
      if (CThread::m_bStop)
        return;

      CUtil::GetExtension(m_pLoader->GetFileName(),strExt);
      strExt = m_pLoader->GetFileName().substr(0,m_pLoader->GetFileName().size()-strExt.size());

      fDiff = 0.f;
    }
    else
    {
      fNewTime=g_application.GetTime();
      fDiff = fNewTime-fCurTime-m_fAVDelay;
    }
    if (fDiff < -0.3f)
    {
      CStdString strFile = m_pLoader->GetFileName();
      m_pLoader->StopStream();
      while (m_pLoader->GetCurSubCode());
      m_pLoader->StreamFile(strFile);
      m_uiNumReadSubCodes = 0;
      m_Cdg.ClearDisplay();
      fNewTime = g_application.GetTime();
      SkipUpToTime((float)fNewTime-m_fAVDelay);
    }
    else
      ReadUpToTime((float)fNewTime-m_fAVDelay);

    fCurTime = fNewTime;
    lock.Leave();
    Sleep(15);
  }
}

void CCdgReader::OnExit()
{}


//CdgRenderer
CCdgRenderer::CCdgRenderer()
{
  m_pd3dDevice = NULL;
  m_pCdgTexture = NULL;
  m_pReader = NULL;
  m_pCdg = NULL;
  m_bRender = false;
  m_bgAlpha = 0x00000000;
  m_fgAlpha = 0xFF000000;
}


CCdgRenderer::~CCdgRenderer()
{
  ReleaseGraphics();
}

bool CCdgRenderer::Attach(CCdgReader* pReader)
{
  CSingleLock lock (m_CritSection);
  if (!m_pReader)
    m_pReader = pReader;
  if (!m_pReader) return false;
  if (!m_pCdg)
    m_pCdg = m_pReader->GetCdg();
  if (!m_pCdg) return false;
  return true;
}
void CCdgRenderer::DetachReader()
{
  CSingleLock lock (m_CritSection);
  m_pReader = NULL;
  m_pCdg = NULL;
}

void CCdgRenderer::Render()
{
  CSingleLock lock (m_CritSection);
  if (!m_pReader) return ;
  m_FileState = m_pReader->GetFileState();
  if (m_FileState == FILE_NOT_LOADED || m_FileState == FILE_SKIP) return ;
  if (m_FileState == FILE_LOADED || m_FileState == FILE_LOADING )
  {
    if (!m_bRender) return ;
    UpdateTexture();
    DrawTexture();
    return ;
  }
  else
  {
    CStdString strMessage, strFileName;
    strFileName = CUtil::GetFileName(m_pReader->GetFileName());
    switch (m_FileState)
    {
    case FILE_ERR_NOT_FOUND:
      strMessage.Format("%s not found", strFileName.c_str());
      break;
    case FILE_ERR_OPENING:
      strMessage.Format("Error opening %s", strFileName.c_str());
      break;
    case FILE_ERR_LOADING:
      strMessage.Format("Error loading %s", strFileName.c_str());
      break;
    case FILE_ERR_NO_MEM:
      strMessage = "Out of memory";
      break;
    }
    // don't render the message to the screen, just log it
    // Hmmm.  Can't seem to be able to log
    //  CLog::Log(LOGWARNING, "Karaoke CDG Renderer: %s", strMessage.c_str());
    /*  CGUIFont* pFont = g_fontManager.GetFont("font14");
      if(pFont)
       pFont->DrawText(60,60, 0xffffffff, strMessage);*/
  }
}

bool CCdgRenderer::InitGraphics()
{
  CSingleLock lock (m_CritSection);
  if (!m_pd3dDevice)
    m_pd3dDevice = g_graphicsContext.Get3DDevice();
  if (!m_pd3dDevice) return false;

  // set the colours
  m_bgAlpha = 0;
  if (g_guiSettings.GetString("mymusic.visualisation").Equals("None"))
    m_bgAlpha = 0xff000000;
  m_fgAlpha = 0xff000000;

  if (!m_pCdgTexture)
    m_pd3dDevice->CreateTexture(WIDTH, HEIGHT, 0, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &m_pCdgTexture);
  if (!m_pCdgTexture) return false;
  m_bRender = true;
  return true;
}

void CCdgRenderer::ReleaseGraphics()
{
  CSingleLock lock (m_CritSection);
  SAFE_RELEASE(m_pCdgTexture);
  m_bRender = false;
}

void CCdgRenderer::DrawTexture()
{
  m_pd3dDevice->SetVertexShader( D3DFVF_CUSTOMVERTEX );
  m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
  m_pd3dDevice->SetTexture(0, m_pCdgTexture);

  m_pd3dDevice->Begin(D3DPT_QUADLIST);

  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  m_pd3dDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)BORDERWIDTH, (float) BORDERHEIGHT);
  m_pd3dDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)g_settings.m_ResInfo[res].Overscan.left, (float) g_settings.m_ResInfo[res].Overscan.top, 0, 0 );

  m_pd3dDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)(WIDTH - BORDERWIDTH), (float) BORDERHEIGHT);
  m_pd3dDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)g_settings.m_ResInfo[res].Overscan.right, (float) g_settings.m_ResInfo[res].Overscan.top, 0, 0 );

  m_pd3dDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)(WIDTH - BORDERWIDTH), (float)(HEIGHT - BORDERHEIGHT));
  m_pd3dDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)g_settings.m_ResInfo[res].Overscan.right, (float) g_settings.m_ResInfo[res].Overscan.bottom, 0, 0);

  m_pd3dDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)BORDERWIDTH, (float)(HEIGHT - BORDERHEIGHT));
  m_pd3dDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)g_settings.m_ResInfo[res].Overscan.left, (float) g_settings.m_ResInfo[res].Overscan.bottom, 0, 0 );

  m_pd3dDevice->End();
}

void CCdgRenderer::UpdateTexture()
{
  D3DLOCKED_RECT LockedRect;
  m_pCdgTexture->LockRect(0, &LockedRect, NULL, 0L);
  for (UINT j = 0; j < HEIGHT; j++ )
  {
    DWORD *texel = (DWORD *)((BYTE *)LockedRect.pBits + j * LockedRect.Pitch);
    for (UINT i = 0; i < WIDTH; i++ )
    {
      BYTE ClutOffset = m_pCdg->GetClutOffset(j + m_pCdg->GetVOffset() , i + m_pCdg->GetHOffset());
      TEX_COLOR TexColor = ConvertColor(m_pCdg->GetColor(ClutOffset));
      if (TexColor >> 24) //Only override transp. for opaque alpha
      {
        TexColor &= 0x00FFFFFF;
        if (ClutOffset == m_pCdg->GetBackgroundColor())
          TexColor |= m_bgAlpha;
        else
          TexColor |= m_fgAlpha;
      }
      *texel++ = TexColor;
    }
  }
  m_pCdgTexture->UnlockRect(0);
}

TEX_COLOR CCdgRenderer::ConvertColor(CDG_COLOR CdgColor)
{
  TEX_COLOR red, green, blue, alpha;
  blue = (TEX_COLOR)((CdgColor & 0x000F) * 17);
  green = ((TEX_COLOR)(((CdgColor & 0x00F0) >> 4) * 17)) << 8;
  red = ((TEX_COLOR)(((CdgColor & 0x0F00) >> 8) * 17)) << 16;
  alpha = ((TEX_COLOR)(((CdgColor & 0xF000) >> 12) * 17)) << 24;
  return alpha | red | green | blue;
}

void CCdgRenderer::SetBGalpha(TEX_COLOR alpha)
{
  m_bgAlpha = alpha;
}

//CdgParser
CCdgParser::CCdgParser()
{
  m_pReader = NULL;
  m_pLoader = NULL;
  m_pRenderer = NULL;
  m_pVoiceManager = NULL;
  m_bIsRunning = false;
}
CCdgParser::~CCdgParser()
{
  FreeGraphics();
  Free();
}

bool CCdgParser::AllocGraphics()
{
  CSingleLock lock (m_CritSection);
  if (!AllocRenderer()) return false;
  m_pRenderer->InitGraphics();
  if (m_pReader)
    m_pRenderer->Attach(m_pReader);
  return true;
}

void CCdgParser::FreeGraphics()
{
  CSingleLock lock (m_CritSection);
  if (m_pRenderer)
    SAFE_DELETE(m_pRenderer);
}

bool CCdgParser::Start(CStdString strSongPath)
{
  if (!StartLoader(strSongPath)) return false;
  if (!StartReader()) return false;

  /* make sure we have fullscreen viz */
  /* hmm won't this switch to fullscreen after each track?? */
  if (m_gWindowManager.GetActiveWindow() != WINDOW_VISUALISATION)
    m_gWindowManager.ActivateWindow(WINDOW_VISUALISATION);

  // Karaoke patch (114097) ...
  if ( g_guiSettings.GetBool("karaoke.voiceenabled") )
  {
    CDG_VOICE_MANAGER_CONFIG VoiceConfig;
    VoiceConfig.dwVoicePacketTime = 20;       // 20ms (can't be lower than this)
    VoiceConfig.dwMaxStoredPackets = 2;
    VoiceConfig.pDSound = g_audioContext.GetDirectSoundDevice();
    VoiceConfig.pCallbackContext = this;
    VoiceConfig.pfnVoiceDeviceCallback = NULL;
    VoiceConfig.pfnVoiceDataCallback = NULL;
    StartVoice(&VoiceConfig);
  }
  // ... Karaoke patch (114097)
  m_bIsRunning = true;
  return true;
}

void CCdgParser::Stop()
{
  StopReader();
  StopLoader();
  StopVoice();
  m_bIsRunning = false;
}

void CCdgParser::Free()
{
  FreeReader();
  FreeLoader();
}

void CCdgParser::SetAVDelay(float fDelay)
{
  CSingleLock lock (m_CritSection);
  if (m_pReader)
    m_pReader->SetAVDelay(fDelay);
}

void CCdgParser::SetBGTransparent(bool bTransparent /* = true */)
{
  if (m_pRenderer)
  {
    TEX_COLOR alpha = 0;
    if (!bTransparent)
      alpha = 0xff000000;
    m_pRenderer->SetBGalpha(alpha);
  }
}

float CCdgParser::GetAVDelay()
{
  CSingleLock lock (m_CritSection);
  if (m_pReader)
    return m_pReader->GetAVDelay();
  return g_advancedSettings.m_karaokeSyncDelay;
}

void CCdgParser::Render()
{
  CSingleLock lock (m_CritSection);
  if (m_pRenderer)
    m_pRenderer->Render();
}

bool CCdgParser::AllocLoader()
{
  if (!m_pLoader)
    m_pLoader = new CCdgLoader;
  if (!m_pLoader) return false;
  return true;
}
bool CCdgParser::AllocReader()
{
  if (!m_pReader)
    m_pReader = new CCdgReader;
  if (!m_pReader) return false;
  return true;
}
bool CCdgParser::AllocRenderer()
{
  if (!m_pRenderer)
    m_pRenderer = new CCdgRenderer;
  if (!m_pRenderer) return false;
  return true;
}
bool CCdgParser::StartLoader(CStdString strSongPath)
{
  CSingleLock lock (m_CritSection);
  if (!AllocLoader()) return false;

  CUtil::RemoveExtension(strSongPath);
  strSongPath += ".cdg";
  if (CFile::Exists(strSongPath))
  {
    m_pLoader->StreamFile(strSongPath);
    return true;
  }

  return false;
}
void CCdgParser::StopLoader()
{
  CSingleLock lock (m_CritSection);
  if (m_pLoader)
    m_pLoader->StopStream();
}
void CCdgParser::FreeLoader()
{
  CSingleLock lock (m_CritSection);
  if (m_pReader)
    m_pReader->DetachLoader();
  if (m_pLoader)
    SAFE_DELETE(m_pLoader);
}
bool CCdgParser::StartReader()
{
  CSingleLock lock (m_CritSection);
  if (!AllocReader()) return false;
  if (m_pLoader)
    m_pReader->Attach(m_pLoader);
  m_pReader->Start((float)g_application.GetTime());
  return true;
}
void CCdgParser::StopReader()
{
  CSingleLock lock (m_CritSection);
  if (m_pReader)
    m_pReader->StopThread();
}
void CCdgParser::FreeReader()
{
  CSingleLock lock (m_CritSection);
  if (m_pRenderer)
    m_pRenderer->DetachReader();
  if (m_pReader)
    SAFE_DELETE(m_pReader);
}
// Karaoke patch (114097) ...
bool CCdgParser::AllocVoice()
{
  if (!m_pVoiceManager)
    m_pVoiceManager = new CCdgVoiceManager;
  if (!m_pVoiceManager) return false;
  return true;
}
bool CCdgParser::StartVoice(CDG_VOICE_MANAGER_CONFIG* pConfig)
{
  CSingleLock lock (m_CritSection);
  if (!AllocVoice()) return false;
  if (m_pVoiceManager)
    m_pVoiceManager->Initialize(pConfig);
  return true;
}
void CCdgParser::StopVoice()
{
  CSingleLock lock (m_CritSection);
  if (m_pVoiceManager)
    m_pVoiceManager->Shutdown();
}
void CCdgParser:: FreeVoice()
{
  CSingleLock lock (m_CritSection);
  if (m_pVoiceManager)
    SAFE_DELETE(m_pVoiceManager);
}
void CCdgParser:: ProcessVoice()
{
  CSingleLock lock (m_CritSection);
  if (m_pVoiceManager)
    m_pVoiceManager->ProcessVoice();
}
// ... Karaoke patch (114097)