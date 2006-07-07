#include "include.h"
#include "GraphicContext.h"
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"
#include "../xbmc/Settings.h"
#include "../xbmc/XBVideoConfig.h"
#include "xgraphics.h"
#include "SkinInfo.h"

CGraphicContext g_graphicsContext;

/* quick access to a skin setting, fine unless we starts clearing video settings */
static CSettingInt* g_guiSkinzoom = NULL;

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
  MergeAlpha(10); // this just here so the inline function is included (why doesn't it include it normally??)
  float x=0,y=0;
  ScaleFinalCoords(x, y);
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
  ScaleFinalCoords(fx, fy);
  fwidth *= m_windowScaleX;
  fheight *= m_windowScaleY;

  int newLeft = (int)(fx + 0.5f);
  int newTop = (int)(fy + 0.5f);
  int newRight = (int)(fx + fwidth + 0.5f);
  int newBottom = (int)(fy + fheight + 0.5f);
  if (intersectPrevious)
  {
    // do the intersection
    int oldLeft = (int)oldviewport->X;
    int oldTop = (int)oldviewport->Y;
    int oldRight = (int)oldviewport->X + oldviewport->Width;
    int oldBottom = (int)oldviewport->Y + oldviewport->Height;
    if (newLeft >= oldRight || newTop >= oldBottom || newRight <= oldLeft || newBottom <= oldTop)
    { // empty intersection - return false to indicate no rendering should occur
      delete oldviewport;
      return false;
    }
    // ok, they intersect, do the intersection
    if (newLeft < oldLeft) newLeft = oldLeft;
    if (newTop < oldTop) newTop = oldTop;
    if (newRight > oldRight) newRight = oldRight;
    if (newBottom > oldBottom) newBottom = oldBottom;
  }
  // check range against screen size
  if (newRight <= 0 || newBottom <= 0 || newTop >= m_iScreenHeight || newLeft >= m_iScreenWidth)
  { // no intersection with the screen
    delete oldviewport;
    return false;
  }
  // intersection with the screen
  if (newLeft < 0) newLeft = 0;
  if (newTop < 0) newTop = 0;
  if (newRight > m_iScreenWidth) newRight = m_iScreenWidth;
  if (newBottom > m_iScreenHeight) newBottom = m_iScreenHeight;

  ASSERT(newLeft < newRight);
  ASSERT(newTop < newBottom);

  newviewport.MinZ = 0.0f;
  newviewport.MaxZ = 1.0f;
  newviewport.X = newLeft;
  newviewport.Y = newTop;
  newviewport.Width = newRight - newLeft;
  newviewport.Height = newBottom - newTop;
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
    m_videoRect.left = (long)(ScaleFinalXCoord((float)rc.left, (float)rc.top) + 0.5f);
    m_videoRect.top = (long)(ScaleFinalYCoord((float)rc.left, (float)rc.top) + 0.5f);
    m_videoRect.right = (long)(ScaleFinalXCoord((float)rc.right, (float)rc.bottom) + 0.5f);
    m_videoRect.bottom = (long)(ScaleFinalYCoord((float)rc.right, (float)rc.bottom) + 0.5f);
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
  if (m_videoRect.left > m_iScreenWidth - 1) clip.x1 = m_iScreenWidth - 1;
  if (m_videoRect.top > m_iScreenHeight - 1) clip.y1 = m_iScreenHeight - 1;
  if (m_videoRect.right > m_iScreenWidth) clip.x2 = m_iScreenWidth;
  if (m_videoRect.bottom > m_iScreenHeight) clip.y2 = m_iScreenHeight;
  if (clip.x2 < clip.x1) clip.x2 = clip.x1 + 1;
  if (clip.y2 < clip.y1) clip.y2 = clip.y1 + 1;
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

  UINT interval = D3DPRESENT_INTERVAL_ONE;  
  if( m_bFullScreenVideo )
    interval = D3DPRESENT_INTERVAL_IMMEDIATE;

#ifdef PROFILE
  interval = D3DPRESENT_INTERVAL_IMMEDIATE;
#endif
 
  if (interval != m_pd3dParams->FullScreen_PresentationInterval)
  {
    m_pd3dParams->FullScreen_PresentationInterval = interval;
    NeedReset = true;
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
  SetScreenFilters(m_bFullScreenVideo);
  Unlock();
  m_Resolution = res;
}

RESOLUTION CGraphicContext::GetVideoResolution() const
{
  return m_Resolution;
}

void CGraphicContext::SetScreenFilters(bool useFullScreenFilters)
{
  Lock();
  if (m_pd3dDevice)
  {
    // These are only valid here and nowhere else
    // set soften on/off
    m_pd3dDevice->SetSoftDisplayFilter(useFullScreenFilters ? g_guiSettings.GetBool("filters.soften") : g_guiSettings.GetBool("uifilters.soften"));
    m_pd3dDevice->SetFlickerFilter(useFullScreenFilters ? g_guiSettings.GetInt("filters.flicker") : g_guiSettings.GetInt("uifilters.flicker"));
  }
  Unlock();
}

void CGraphicContext::ScaleRectToScreenResolution(DWORD& left, DWORD& top, DWORD& right, DWORD& bottom, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentX = ((float)m_iScreenWidth ) / ((float)g_settings.m_ResInfo[res].iWidth);
  float fPercentY = ((float)m_iScreenHeight) / ((float)g_settings.m_ResInfo[res].iHeight);
  left = (DWORD) ( (float(left)) * fPercentX + 0.5f);
  top = (DWORD) ( (float(top)) * fPercentY + 0.5f);
  right = (DWORD) ( (float(right)) * fPercentX + 0.5f);
  bottom = (DWORD) ( (float(bottom)) * fPercentY + 0.5f);
}

void CGraphicContext::ScalePosToScreenResolution(DWORD& x, DWORD& y, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentX = ((float)m_iScreenWidth ) / ((float)g_settings.m_ResInfo[res].iWidth);
  float fPercentY = ((float)m_iScreenHeight) / ((float)g_settings.m_ResInfo[res].iHeight);
  x = (DWORD) ( (float(x)) * fPercentX + 0.5f);
  y = (DWORD) ( (float(y)) * fPercentY + 0.5f);
}

void CGraphicContext::ScaleXCoord(DWORD& x, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentX = ((float)m_iScreenWidth ) / ((float)g_settings.m_ResInfo[res].iWidth);
  x = (DWORD) ( (float(x)) * fPercentX + 0.5f);
}

void CGraphicContext::ScaleXCoord(int& x, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentX = ((float)m_iScreenWidth ) / ((float)g_settings.m_ResInfo[res].iWidth);
  x = (int) ( (float(x)) * fPercentX + 0.5f);
}

void CGraphicContext::ScaleXCoord(long& x, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentX = ((float)m_iScreenWidth ) / ((float)g_settings.m_ResInfo[res].iWidth);
  x = (long) ( (float(x)) * fPercentX + 0.5f);
}

void CGraphicContext::ScaleXCoord(float& x, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentX = ((float)m_iScreenWidth ) / ((float)g_settings.m_ResInfo[res].iWidth);
  x *= fPercentX;
}

void CGraphicContext::ScaleYCoord(DWORD& y, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentY = ((float)m_iScreenHeight ) / ((float)g_settings.m_ResInfo[res].iHeight);
  y = (DWORD) ( (float(y)) * fPercentY + 0.5f);
}

void CGraphicContext::ScaleYCoord(int& y, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentY = ((float)m_iScreenHeight ) / ((float)g_settings.m_ResInfo[res].iHeight);
  y = (int) ( (float(y)) * fPercentY + 0.5f);
}

void CGraphicContext::ScaleYCoord(long& y, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentY = ((float)m_iScreenHeight ) / ((float)g_settings.m_ResInfo[res].iHeight);
  y = (long) ( (float(y)) * fPercentY + 0.5f);
}

void CGraphicContext::ScaleYCoord(float& y, RESOLUTION res)
{
  if (res == INVALID) return ;
  float fPercentY = ((float)m_iScreenHeight ) / ((float)g_settings.m_ResInfo[res].iHeight);
  y *= fPercentY;
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
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );
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
  if (needsScaling)
  {
    // calculate necessary scalings
    float fFromWidth = (float)g_settings.m_ResInfo[res].iWidth;
    float fFromHeight = (float)g_settings.m_ResInfo[res].iHeight;
    float fToPosX = (float)g_settings.m_ResInfo[m_Resolution].Overscan.left;
    float fToPosY = (float)g_settings.m_ResInfo[m_Resolution].Overscan.top;
    float fToWidth = (float)g_settings.m_ResInfo[m_Resolution].Overscan.right - fToPosX;
    float fToHeight = (float)g_settings.m_ResInfo[m_Resolution].Overscan.bottom - fToPosY;

    // add additional zoom to compensate for any overskan built in skin
    float fZoom = g_SkinInfo.GetSkinZoom();

    if(!g_guiSkinzoom) // lookup gui setting if we didn't have it already
      g_guiSkinzoom = (CSettingInt*)g_guiSettings.GetSetting("lookandfeel.skinzoom");

    if(g_guiSkinzoom)
      fZoom *= (100 + g_guiSkinzoom->GetData()) * 0.01f;

    fZoom -= 1.0f;
    fToPosX -= fToWidth * fZoom * 0.5f;
    fToWidth *= fZoom + 1.0f;

    /* adjust for aspect ratio as zoom is given in the vertical direction and we don't /*
    /* do aspect ratio corrections in the gui code */
    fZoom = fZoom / g_settings.m_ResInfo[m_Resolution].fPixelRatio;
    fToPosY -= fToHeight * fZoom * 0.5f;
    fToHeight *= fZoom + 1.0f;
    
    m_windowScaleX = fToWidth / fFromWidth;
    m_windowScaleY = fToHeight / fFromHeight;
    TransformMatrix windowOffset = TransformMatrix::CreateTranslation((float)posX, (float)posY);
    TransformMatrix guiScaler = TransformMatrix::CreateScaler(fToWidth / fFromWidth, fToHeight / fFromHeight);
    TransformMatrix guiOffset = TransformMatrix::CreateTranslation(fToPosX, fToPosY);
    m_guiTransform = guiOffset * guiScaler * windowOffset;
  }
  else
  {
    m_guiTransform = TransformMatrix::CreateTranslation((float)posX, (float)posY);
    m_windowScaleX = 1.0f;
    m_windowScaleY = 1.0f;
  }
  // reset the final transform and window transforms
  m_finalWindowTransform = m_guiTransform;
  m_finalTransform = m_guiTransform;
}

inline void CGraphicContext::ScaleFinalCoords(float &x, float &y) const
{
  m_finalTransform.TransformPosition(x, y);
}

inline float CGraphicContext::ScaleFinalXCoord(float x, float y) const
{
  return m_finalTransform.TransformXCoord(x, y);
}

inline float CGraphicContext::ScaleFinalYCoord(float x, float y) const
{
  return m_finalTransform.TransformYCoord(x, y);
}

inline DWORD CGraphicContext::MergeAlpha(DWORD color) const
{
  DWORD alpha = m_finalTransform.TransformAlpha((color >> 24) & 0xff);
  return ((alpha << 24) & 0xff000000) | (color & 0xffffff);
}

