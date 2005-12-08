#include "include.h"
#include "GraphicContext.h"
#include "../xbmc/Settings.h"
#include "../xbmc/XBVideoConfig.h"
#include "xgraphics.h"

#define WIDE_SCREEN_COMPENSATIONY (FLOAT)1.2
#define WIDE_SCREEN_COMPENSATIONX (FLOAT)0.85

CGraphicContext g_graphicsContext;

CGraphicContext::CGraphicContext(void)
{
  m_iScreenWidth = 720;
  m_iScreenHeight = 576;
  m_pd3dDevice = NULL;
  m_pd3dParams = NULL;
  m_dwID = 0;
  m_strMediaDir = "D:\\media";
  m_bShowPreviewWindow = false;
  m_bCalibrating = false;
  m_Resolution = INVALID;
  m_pCallback = NULL;
  m_stateBlock = 0xffffffff;
  m_windowScaleX = m_windowScaleY = 1.0f;
  m_windowPosX = m_windowPosY = 0.0f;
  m_controlAlpha = m_windowAlpha = 255;
  m_controlOffsetX = m_controlOffsetY = m_windowOffsetX = m_windowOffsetY = 0;
  MergeAlpha(10); // this just here so the inline function is included (why doesn't it include it normally??)
}

CGraphicContext::~CGraphicContext(void)
{
  if (m_stateBlock != 0xffffffff)
  {
    Get3DDevice()->DeleteStateBlock(m_stateBlock);
  }
  while (m_viewStack.size())
  {
    D3DVIEWPORT8 *viewport = m_viewStack.top();
    m_viewStack.pop();
    if (viewport) delete viewport;
  }
}


void CGraphicContext::SetD3DDevice(LPDIRECT3DDEVICE8 p3dDevice)
{
  m_pd3dDevice = p3dDevice;
}

void CGraphicContext::SetD3DParameters(D3DPRESENT_PARAMETERS *p3dParams)
{
  m_pd3dParams = p3dParams;
}

bool CGraphicContext::SendMessage(CGUIMessage& message)
{
  if (!m_pCallback) return false;
  return m_pCallback->SendMessage(message);
}

void CGraphicContext::setMessageSender(IMsgSenderCallback* pCallback)
{
  m_pCallback = pCallback;
}

DWORD CGraphicContext::GetNewID()
{
  m_dwID++;
  return m_dwID;
}

bool CGraphicContext::SetViewPort(float fx, float fy , float fwidth, float fheight, bool intersectPrevious /* = false */)
{
  D3DVIEWPORT8 newviewport;
  D3DVIEWPORT8 *oldviewport = new D3DVIEWPORT8;
  Get3DDevice()->GetViewport(oldviewport);
  // transform coordinates
  fx = ScaleFinalXCoord(fx);
  fy = ScaleFinalYCoord(fy);
  fwidth *= m_windowScaleX;
  fheight *= m_windowScaleY;
  if (fx < 0) fx = 0;
  if (fy < 0) fy = 0;

  newviewport.X = (DWORD)fx;
  newviewport.Y = (DWORD)fy;
  newviewport.Width = (DWORD)(fwidth);
  newviewport.Height = (DWORD)(fheight);
  if (intersectPrevious)
  {
    if (newviewport.X < oldviewport->X) newviewport.X = oldviewport->X;
    if (newviewport.Y < oldviewport->Y) newviewport.Y = oldviewport->Y;
    if (newviewport.X > oldviewport->X + oldviewport->Width || newviewport.Y > oldviewport->Y + oldviewport->Height)
    {
      delete oldviewport;
      return false;
    }
    if (newviewport.X + newviewport.Width > oldviewport->X + oldviewport->Width)
      newviewport.Width = oldviewport->Y + oldviewport->Width - newviewport.X;
    if (newviewport.Y + newviewport.Height > oldviewport->Y + oldviewport->Height)
      newviewport.Height = oldviewport->Y + oldviewport->Height - newviewport.Y;
  }
  // check range
  if (newviewport.X + newviewport.Width > (DWORD)m_iScreenWidth)
  {
    if (newviewport.X >= (DWORD)m_iScreenWidth)
    {
      newviewport.X = m_iScreenWidth - 1;
      newviewport.Width = 1;
    }
    else
    {
      newviewport.Width = m_iScreenWidth - newviewport.X;
    }
  }
  if (newviewport.Y + newviewport.Height > (DWORD)m_iScreenHeight)
  {
    if (newviewport.Y >= (DWORD)m_iScreenHeight)
    {
      newviewport.Y = m_iScreenHeight - 1;
      newviewport.Height = 1;
    }
    else
    {
      newviewport.Height = m_iScreenHeight-newviewport.Y;
    }
  }
  newviewport.MinZ = 0.0f;
  newviewport.MaxZ = 1.0f;
  Get3DDevice()->SetViewport(&newviewport);
  m_viewStack.push(oldviewport);
  return true;
}

void CGraphicContext::RestoreViewPort()
{
  if (!m_viewStack.size()) return;
  D3DVIEWPORT8 *oldviewport = (D3DVIEWPORT8*)m_viewStack.top();
  m_viewStack.pop();
  Get3DDevice()->SetViewport(oldviewport);
  if (oldviewport) delete oldviewport;
}

const RECT& CGraphicContext::GetViewWindow() const
{
  return m_videoRect;
}
void CGraphicContext::SetViewWindow(const RECT& rc)
{
  if (m_bCalibrating)
  {
    SetFullScreenViewWindow(m_Resolution);
  }
  else
  {
    m_videoRect.left = (long)(ScaleFinalXCoord((float)rc.left) + 0.5f);
    m_videoRect.top = (long)(ScaleFinalYCoord((float)rc.top) + 0.5f);
    m_videoRect.right = (long)(ScaleFinalXCoord((float)rc.right) + 0.5f);
    m_videoRect.bottom = (long)(ScaleFinalYCoord((float)rc.bottom) + 0.5f);
    if (m_bShowPreviewWindow && !m_bFullScreenVideo)
    {
      D3DRECT d3dRC;
      d3dRC.x1 = m_videoRect.left;
      d3dRC.x2 = m_videoRect.right;
      d3dRC.y1 = m_videoRect.top;
      d3dRC.y2 = m_videoRect.bottom;
      Get3DDevice()->Clear( 1, &d3dRC, D3DCLEAR_TARGET, 0x00010001, 1.0f, 0L );
    }
  }
}

void CGraphicContext::ClipToViewWindow()
{
  D3DRECT clip = { m_videoRect.left, m_videoRect.top, m_videoRect.right, m_videoRect.bottom };
  if (m_videoRect.left < 0) clip.x1 = 0;
  if (m_videoRect.top < 0) clip.y1 = 0;
  if (m_videoRect.right > m_iScreenWidth) clip.x2 = m_iScreenWidth;
  if (m_videoRect.bottom > m_iScreenHeight) clip.y2 = m_iScreenHeight;
  m_pd3dDevice->SetScissors(1, FALSE, &clip);
}

void CGraphicContext::SetFullScreenViewWindow(RESOLUTION &res)
{
  m_videoRect.left = g_settings.m_ResInfo[res].Overscan.left;
  m_videoRect.top = g_settings.m_ResInfo[res].Overscan.top;
  m_videoRect.right = g_settings.m_ResInfo[res].Overscan.right;
  m_videoRect.bottom = g_settings.m_ResInfo[res].Overscan.bottom;
}

void CGraphicContext::SetFullScreenVideo(bool bOnOff)
{
  Lock();
  m_bFullScreenVideo = bOnOff;
  SetFullScreenViewWindow(m_Resolution);
  Unlock();
}

bool CGraphicContext::IsFullScreenVideo() const
{
  return m_bFullScreenVideo;
}

void CGraphicContext::EnablePreviewWindow(bool bEnable)
{
  m_bShowPreviewWindow = bEnable;
}


bool CGraphicContext::IsCalibrating() const
{
  return m_bCalibrating;
}

void CGraphicContext::SetCalibrating(bool bOnOff)
{
  m_bCalibrating = bOnOff;
}

bool CGraphicContext::IsValidResolution(RESOLUTION res)
{
  return g_videoConfig.IsValidResolution(res);
}

void CGraphicContext::GetAllowedResolutions(vector<RESOLUTION> &res, bool bAllowPAL60)
{
  bool bCanDoWidescreen = g_videoConfig.HasWidescreen();
  res.clear();
  res.push_back(AUTORES);
  if (g_videoConfig.HasPAL())
  {
    res.push_back(PAL_4x3);
    if (bCanDoWidescreen) res.push_back(PAL_16x9);
    if (bAllowPAL60 && g_videoConfig.HasPAL60())
    {
      res.push_back(PAL60_4x3);
      if (bCanDoWidescreen) res.push_back(PAL60_16x9);
    }
  }
  else
  {
    res.push_back(NTSC_4x3);
    if (bCanDoWidescreen) res.push_back(NTSC_16x9);
    if (g_videoConfig.Has480p())
    {
      res.push_back(HDTV_480p_4x3);
      if (bCanDoWidescreen) res.push_back(HDTV_480p_16x9);
    }
    if (g_videoConfig.Has720p())
      res.push_back(HDTV_720p);
    if (g_videoConfig.Has1080i())
      res.push_back(HDTV_1080i);
  }
}

void CGraphicContext::SetGUIResolution(RESOLUTION &res)
{
  CLog::Log(LOGDEBUG, "Setting resolution %i", res);
  SetVideoResolution(res, TRUE);
  CLog::Log(LOGDEBUG, "We set resolution %i", m_Resolution);
  if (!m_pd3dParams) return ;
  m_iScreenWidth = m_pd3dParams->BackBufferWidth ;
  m_iScreenHeight = m_pd3dParams->BackBufferHeight;
  m_bWidescreen = (m_pd3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN) != 0;
}

void CGraphicContext::SetVideoResolution(RESOLUTION &res, BOOL NeedZ)
{
  if (res == AUTORES)
  {
    res = g_videoConfig.GetBestMode();
  }
  if (!IsValidResolution(res))
  { // Choose a failsafe resolution that we can actually display
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    res = g_videoConfig.GetSafeMode();
  }
  if (!m_pd3dParams)
  {
    m_Resolution = res;
    return ;
  }
  bool NeedReset = false;
  if (m_bFullScreenVideo)
  {
    if (m_pd3dParams->FullScreen_PresentationInterval != D3DPRESENT_INTERVAL_IMMEDIATE )
    {
      m_pd3dParams->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
      NeedReset = true;
    }
  }
  else
  {
    if (m_pd3dParams->FullScreen_PresentationInterval != D3DPRESENT_INTERVAL_ONE)
    {
      m_pd3dParams->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
      NeedReset = true;
    }
  }


  if (NeedZ != m_pd3dParams->EnableAutoDepthStencil)
  {
    m_pd3dParams->EnableAutoDepthStencil = NeedZ;
    NeedReset = true;
  }
  if (m_Resolution != res)
  {
    NeedReset = true;
    m_pd3dParams->BackBufferWidth = g_settings.m_ResInfo[res].iWidth;
    m_pd3dParams->BackBufferHeight = g_settings.m_ResInfo[res].iHeight;
    m_pd3dParams->Flags = g_settings.m_ResInfo[res].dwFlags;
    if (res == PAL60_4x3 || res == PAL60_16x9)
    {
      if (m_pd3dParams->BackBufferWidth <= 720 && m_pd3dParams->BackBufferHeight <= 480)
      {
        m_pd3dParams->FullScreen_RefreshRateInHz = 60;
      }
      else
      {
        m_pd3dParams->FullScreen_RefreshRateInHz = 0;
      }
    }
    else
    {
      m_pd3dParams->FullScreen_RefreshRateInHz = 0;
    }
  }
  Lock();
  if (NeedReset && m_pd3dDevice)
  {
    m_pd3dDevice->Reset(m_pd3dParams);
  }
  if ((g_settings.m_ResInfo[m_Resolution].iWidth != g_settings.m_ResInfo[res].iWidth) || (g_settings.m_ResInfo[m_Resolution].iHeight != g_settings.m_ResInfo[res].iHeight))
  { // set the mouse resolution
    g_Mouse.SetResolution(g_settings.m_ResInfo[res].iWidth, g_settings.m_ResInfo[res].iHeight, 1, 1);
  }
  if (m_pd3dDevice)
  {
    SetFullScreenViewWindow(res);
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );
    m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
  }
  if ( /*NeedReset && */m_pd3dDevice)
  {
    // These are only valid here and nowhere else
    // set soften on/off
    m_pd3dDevice->SetSoftDisplayFilter(m_bFullScreenVideo ? g_guiSettings.GetBool("Filters.Soften") : g_guiSettings.GetBool("UIFilters.Soften"));
    m_pd3dDevice->SetFlickerFilter(m_bFullScreenVideo ? g_guiSettings.GetInt("Filters.Flicker") : g_guiSettings.GetInt("UIFilters.Flicker"));
  }
  Unlock();
  m_Resolution = res;
}

RESOLUTION CGraphicContext::GetVideoResolution() const
{
  return m_Resolution;
}

void CGraphicContext::ScaleRectToScreenResolution(DWORD& left, DWORD& top, DWORD& right, DWORD& bottom, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentX = ((float)m_iScreenWidth ) / ((float)g_settings.m_ResInfo[res].iWidth);
  float fPercentY = ((float)m_iScreenHeight) / ((float)g_settings.m_ResInfo[res].iHeight);
  left = (DWORD) ( (float(left)) * fPercentX);
  top = (DWORD) ( (float(top)) * fPercentY);
  right = (DWORD) ( (float(right)) * fPercentX);
  bottom = (DWORD) ( (float(bottom)) * fPercentY);
}

void CGraphicContext::ScalePosToScreenResolution(DWORD& x, DWORD& y, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentX = ((float)m_iScreenWidth ) / ((float)g_settings.m_ResInfo[res].iWidth);
  float fPercentY = ((float)m_iScreenHeight) / ((float)g_settings.m_ResInfo[res].iHeight);
  x = (DWORD) ( (float(x)) * fPercentX);
  y = (DWORD) ( (float(y)) * fPercentY);
}

void CGraphicContext::ScaleXCoord(DWORD& x, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentX = ((float)m_iScreenWidth ) / ((float)g_settings.m_ResInfo[res].iWidth);
  x = (DWORD) ( (float(x)) * fPercentX);
}

void CGraphicContext::ScaleXCoord(int& x, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentX = ((float)m_iScreenWidth ) / ((float)g_settings.m_ResInfo[res].iWidth);
  x = (int) ( (float(x)) * fPercentX);
}

void CGraphicContext::ScaleXCoord(long& x, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentX = ((float)m_iScreenWidth ) / ((float)g_settings.m_ResInfo[res].iWidth);
  x = (long) ( (float(x)) * fPercentX);
}

void CGraphicContext::ScaleYCoord(DWORD& y, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentY = ((float)m_iScreenHeight ) / ((float)g_settings.m_ResInfo[res].iHeight);
  y = (DWORD) ( (float(y)) * fPercentY);
}

void CGraphicContext::ScaleYCoord(int& y, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentY = ((float)m_iScreenHeight ) / ((float)g_settings.m_ResInfo[res].iHeight);
  y = (int) ( (float(y)) * fPercentY);
}

void CGraphicContext::ScaleYCoord(long& y, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentY = ((float)m_iScreenHeight ) / ((float)g_settings.m_ResInfo[res].iHeight);
  y = (long) ( (float(y)) * fPercentY);
}

void CGraphicContext::ResetOverscan(RESOLUTION res, OVERSCAN &overscan)
{
  overscan.left = 0;
  overscan.top = 0;
  switch (res)
  {
  case HDTV_1080i:
    overscan.right = 1920;
    overscan.bottom = 1080;
    break;
  case HDTV_720p:
    overscan.right = 1280;
    overscan.bottom = 720;
    break;
  case HDTV_480p_16x9:
  case HDTV_480p_4x3:
  case NTSC_16x9:
  case NTSC_4x3:
  case PAL60_16x9:
  case PAL60_4x3:
    overscan.right = 720;
    overscan.bottom = 480;
    break;
  case PAL_16x9:
  case PAL_4x3:
    overscan.right = 720;
    overscan.bottom = 576;
    break;
  }
}

void CGraphicContext::ResetScreenParameters(RESOLUTION res)
{
  ResetOverscan(res, g_settings.m_ResInfo[res].Overscan);
  g_settings.m_ResInfo[res].iOSDYOffset = 0;
  g_settings.m_ResInfo[res].fPixelRatio = GetPixelRatio(res);
  // 1080i
  switch (res)
  {
  case HDTV_1080i:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 1080);
    g_settings.m_ResInfo[res].iWidth = 1920;
    g_settings.m_ResInfo[res].iHeight = 1080;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "1080i 16:9");
    break;
  case HDTV_720p:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 720);
    g_settings.m_ResInfo[res].iWidth = 1280;
    g_settings.m_ResInfo[res].iHeight = 720;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "720p 16:9");
    break;
  case HDTV_480p_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
    strcpy(g_settings.m_ResInfo[res].strMode, "480p 4:3");
    break;
  case HDTV_480p_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "480p 16:9");
    break;
  case NTSC_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = 0;
    strcpy(g_settings.m_ResInfo[res].strMode, "NTSC 4:3");
    break;
  case NTSC_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "NTSC 16:9");
    break;
  case PAL_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 576);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 576;
    g_settings.m_ResInfo[res].dwFlags = 0;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL 4:3");
    break;
  case PAL_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 576);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 576;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL 16:9");
    break;
  case PAL60_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = 0;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL60 4:3");
    break;
  case PAL60_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL60 16:9");
    break;
  }
}

float CGraphicContext::GetPixelRatio(RESOLUTION iRes) const
{
  if (iRes == HDTV_1080i || iRes == HDTV_720p) return 1.0f;
  if (iRes == HDTV_480p_4x3 || iRes == NTSC_4x3 || iRes == PAL60_4x3) return 4320.0f / 4739.0f;
  if (iRes == HDTV_480p_16x9 || iRes == NTSC_16x9 || iRes == PAL60_16x9) return 4320.0f / 4739.0f*4.0f / 3.0f;
  if (iRes == PAL_16x9) return 128.0f / 117.0f*4.0f / 3.0f;
  return 128.0f / 117.0f;
}

void CGraphicContext::Clear()
{
  //Not trying to clear the zbuffer when there is none is 7 fps faster (pal resolution)
  if ((!m_pd3dParams) || (m_pd3dParams->EnableAutoDepthStencil == TRUE))
  {
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00010001, 1.0f, 0L );
  }
  else
  {
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, 0x00010001, 1.0f, 0L );
  }
}

void CGraphicContext::CaptureStateBlock()
{
  if (m_stateBlock != 0xffffffff)
  {
    Get3DDevice()->DeleteStateBlock(m_stateBlock);
  }

  if (D3D_OK != Get3DDevice()->CreateStateBlock(D3DSBT_ALL, &m_stateBlock))
  {
    // Creation failure
    m_stateBlock = 0xffffffff;
  }
}

void CGraphicContext::ApplyStateBlock()
{
  if (m_stateBlock != 0xffffffff)
  {
    Get3DDevice()->ApplyStateBlock(m_stateBlock);
  }
}

void CGraphicContext::SetScalingResolution(RESOLUTION res, int posX, int posY, bool needsScaling)
{
  // calculate necessary scalings
  float fFromWidth = (float)g_settings.m_ResInfo[res].iWidth;
  float fFromHeight = (float)g_settings.m_ResInfo[res].iHeight;
  float fToPosX = (float)g_settings.m_ResInfo[m_Resolution].GUIOverscan.left;
  float fToPosY = (float)g_settings.m_ResInfo[m_Resolution].GUIOverscan.top;
  float fToWidth = (float)g_settings.m_ResInfo[m_Resolution].GUIOverscan.right - fToPosX;
  float fToHeight = (float)g_settings.m_ResInfo[m_Resolution].GUIOverscan.bottom - fToPosY;

  if (needsScaling)
  {
    m_windowScaleX = fToWidth / fFromWidth;
    m_windowScaleY = fToHeight / fFromHeight;
    m_windowPosX = fToPosX + m_windowScaleX * posX;
    m_windowPosY = fToPosY + m_windowScaleY * posY;
  }
  else
  {
    m_windowScaleX = 1.0f;
    m_windowScaleY = 1.0f;
    m_windowPosX = (float)posX;
    m_windowPosY = (float)posY;
  }
}

inline float CGraphicContext::ScaleFinalXCoord(float x) const
{
  return (x + m_controlOffsetX) * m_windowScaleX + m_windowPosX + m_windowOffsetX;
}

inline float CGraphicContext::ScaleFinalYCoord(float y) const
{
  return (y + m_controlOffsetY) * m_windowScaleY + m_windowPosY + m_windowOffsetY;
}

inline DWORD CGraphicContext::MergeAlpha(DWORD color) const
{
  DWORD alpha = ((color >> 24) & 0xff) * m_controlAlpha * m_windowAlpha / 65025;
  return ((alpha << 24) & 0xff000000) | (color & 0xffffff);
}
