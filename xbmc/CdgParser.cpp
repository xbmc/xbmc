#include "stdafx.h"
#include "CdgParser.h"
#include "application.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
  m_pSubCode = NULL;
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
  if (m_CdgFileState != FILE_LOADING) return ;

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
  m_Timer.StartZero();
  SetAVDelay(g_guiSettings.GetFloat("Karaoke.SyncDelay"));
  m_uiNumReadSubCodes = 0;
  m_Cdg.ClearDisplay();
  CThread::Create(false);
  return true;
}


void CCdgReader::Pause()
{
  CSingleLock lock (m_CritSection);
  if (m_Timer.IsRunning())
    m_Timer.Stop();
  else
    m_Timer.Start();
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
void CCdgReader::OnStartup()
{}

void CCdgReader::Process()
{
  float fCurTime = 0.0f;
  bool bIsFirstPass = true;

  while (!CThread::m_bStop)
  {
    CSingleLock lock (m_CritSection);
    fCurTime = m_Timer.GetElapsedSeconds();
    if (fCurTime > 2.0f && bIsFirstPass)
    {
      //Sync the playback with mplayer after 2 secs.
      m_fStartingTime = g_application.m_pPlayer->GetTime() * 0.001f;
      m_Timer.StartZero();
      fCurTime = 0.0f;
      bIsFirstPass = false;
    }
    ReadUpToTime(fCurTime + m_fStartingTime - m_fAVDelay);
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
  m_pVertexBuffer = NULL;
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
  if (m_FileState == FILE_NOT_LOADED) return ;
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

  if (!m_pVertexBuffer)
  {
    m_pd3dDevice->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX), D3DUSAGE_WRITEONLY,
                                     D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &m_pVertexBuffer);
    if (!m_pVertexBuffer) return false;
    RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
    CUSTOMVERTEX* pVertices = NULL;
    m_pVertexBuffer->Lock(0, 4 * sizeof(CUSTOMVERTEX), (BYTE**)&pVertices, 0L);
    pVertices[0].x = pVertices[3].x = (float) g_settings.m_ResInfo[iRes].Overscan.left;
    pVertices[1].x = pVertices[2].x = (float) g_settings.m_ResInfo[iRes].Overscan.right;
    pVertices[0].y = pVertices[1].y = (float) g_settings.m_ResInfo[iRes].Overscan.top;
    pVertices[2].y = pVertices[3].y = (float) g_settings.m_ResInfo[iRes].Overscan.bottom;
    pVertices[0].z = pVertices[1].z = (float) pVertices[2].z = pVertices[3].z = 1.0f;
    pVertices[0].rhw = pVertices[1].rhw = pVertices[2].rhw = pVertices[3].rhw = 1.0f;
#ifdef _DEBUG   //In debug mode, show the border area
    pVertices[1].u = pVertices[2].u = (float) WIDTH / (float)TEXWIDTH;
    pVertices[0].u = pVertices[3].u = 0;
    pVertices[0].v = pVertices[1].v = 0;
    pVertices[2].v = pVertices[3].v = (float) HEIGHT / (float) TEXHEIGHT;
#else // Don't show the border in release mode
    pVertices[1].u = pVertices[2].u = ((float) WIDTH - (float)BORDERWIDTH) / (float)TEXWIDTH;
    pVertices[0].u = pVertices[3].u = (float)BORDERWIDTH / (float) TEXWIDTH;
    pVertices[0].v = pVertices[1].v = (float) BORDERHEIGHT / (float)TEXHEIGHT;
    pVertices[2].v = pVertices[3].v = ((float) HEIGHT - (float)BORDERHEIGHT) / (float) TEXHEIGHT;
#endif
    m_pVertexBuffer->Unlock();
  }

  // set the colours
  m_bgAlpha = ((TEX_COLOR) (g_guiSettings.GetInt("Karaoke.BackgroundAlpha") & 0x000000FF)) << 24;
  m_fgAlpha = ((TEX_COLOR) (g_guiSettings.GetInt("Karaoke.ForegroundAlpha") & 0x000000FF)) << 24;

  if (!m_pCdgTexture)
    m_pd3dDevice->CreateTexture(TEXWIDTH, TEXHEIGHT, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_pCdgTexture);
  if (!m_pCdgTexture) return false;
  m_bRender = true;
  return true;
}

void CCdgRenderer::ReleaseGraphics()
{
  CSingleLock lock (m_CritSection);
  SAFE_RELEASE(m_pCdgTexture);
  SAFE_RELEASE(m_pVertexBuffer);
  m_bRender = false;
}

void CCdgRenderer::DrawTexture()
{
  m_pd3dDevice->SetVertexShader( D3DFVF_CUSTOMVERTEX );
  m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  m_pd3dDevice->SetStreamSource( 0, m_pVertexBuffer, sizeof(CUSTOMVERTEX) );
  m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  m_pd3dDevice->SetTexture(0, m_pCdgTexture);
  m_pd3dDevice->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );
}

void CCdgRenderer::UpdateTexture()
{
  D3DLOCKED_RECT LockedRect;
  m_pCdgTexture->LockRect(0, &LockedRect, NULL, 0L);
  void *pTexel = (void*) LockedRect.pBits;

  TEX_COLOR TexColor;
  BYTE ClutOffset;
  UINT i, j;
  Swizzler s( TEXWIDTH, TEXHEIGHT, 0);
  s.SetU(0);
  for ( i = 0; i < WIDTH; i++ )
  {
    s.SetV(0);
    for ( j = 0; j < HEIGHT; j++ )
    {
      ClutOffset = m_pCdg->GetClutOffset(j + m_pCdg->GetVOffset() , i + m_pCdg->GetHOffset());
      TexColor = ConvertColor(m_pCdg->GetColor(ClutOffset));
      if (TexColor >> 24) //Only override transp. for opaque alpha
      {
        TexColor &= 0x00FFFFFF;
        if (ClutOffset == m_pCdg->GetBackgroundColor())
          TexColor |= m_bgAlpha;
        else
          TexColor |= m_fgAlpha;
      }
      ((TEX_COLOR*)pTexel)[s.Get2D()] = TexColor;
      s.IncV();
    }
    s.IncU();
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
//CdgParser
CCdgParser::CCdgParser()
{
  m_pReader = NULL;
  m_pLoader = NULL;
  m_pRenderer = NULL;
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
  return true;
}

void CCdgParser::Pause()
{
  CSingleLock lock (m_CritSection);
  if (m_pReader)
    m_pReader->Pause();
}

void CCdgParser::Stop()
{
  StopReader();
  StopLoader();
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

float CCdgParser::GetAVDelay()
{
  CSingleLock lock (m_CritSection);
  if (m_pReader)
    return m_pReader->GetAVDelay();
  return g_guiSettings.GetFloat("Karaoke.SyncDelay");
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
  m_pLoader->StreamFile(strSongPath);
  return true;
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
  m_pReader->Start(0.001f*g_application.m_pPlayer->GetTime());
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
