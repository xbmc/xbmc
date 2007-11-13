#include "include.h"
#include "GraphicContext.h"
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"
#include "../xbmc/Settings.h"
#include "../xbmc/XBVideoConfig.h"
#ifdef HAS_XBOX_D3D
 #include "xgraphics.h"
 #define D3D_CLEAR_STENCIL D3DCLEAR_STENCIL
#else
 #define D3D_CLEAR_STENCIL 0x0l
#endif
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
  m_stateBlock = 0xffffffff;
  m_dwID = 0;
  m_strMediaDir = "D:\\media";
  m_bCalibrating = false;
  m_Resolution = INVALID;
  m_pCallback = NULL;
  m_guiScaleX = m_guiScaleY = 1.0f;
  m_windowResolution = INVALID;
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

void CGraphicContext::SetOrigin(float x, float y)
{
  if (m_origins.size())
    m_origins.push(CPoint(x,y) + m_origins.top());
  else
    m_origins.push(CPoint(x,y));
  AddTransform(TransformMatrix::CreateTranslation(x, y));
}

void CGraphicContext::RestoreOrigin()
{
  m_origins.pop();
  RemoveTransform();
}

// add a new clip region, intersecting with the previous clip region.
bool CGraphicContext::SetClipRegion(float x, float y, float w, float h)
{ // transform from our origin
  CPoint origin;
  if (m_origins.size())
    origin = m_origins.top();
  // ok, now intersect with our old clip region
  CRect rect(x, y, x + w, y + h);
  rect += origin;
  if (m_clipRegions.size())
  { // intersect with original clip region
    rect.Intersect(m_clipRegions.top());
  }
  if (rect.IsEmpty())
    return false;
  m_clipRegions.push(rect);

  // here we could set the hardware clipping, if applicable
  return true;
}

void CGraphicContext::RestoreClipRegion()
{
  if (m_clipRegions.size())
    m_clipRegions.pop();

  // here we could reset the hardware clipping, if applicable
}

void CGraphicContext::ClipRect(CRect &vertex, CRect &texture, CRect *texture2)
{
  // this is the software clipping routine.  If the graphics hardware is set to do the clipping
  // (eg via SetClipPlane in D3D for instance) then this routine is unneeded.
  if (m_clipRegions.size())
  {
    // take a copy of the vertex rectangle and intersect
    // it with our clip region (moved to the same coordinate system)
    CRect clipRegion(m_clipRegions.top());
    if (m_origins.size())
      clipRegion -= m_origins.top();
    CRect original(vertex);
    vertex.Intersect(clipRegion);
    // and use the original to compute the texture coordinates
    if (original != vertex)
    {
      float scaleX = texture.Width() / original.Width();
      float scaleY = texture.Height() / original.Height();
      texture.x1 += (vertex.x1 - original.x1) * scaleX;
      texture.y1 += (vertex.y1 - original.y1) * scaleY;
      texture.x2 += (vertex.x2 - original.x2) * scaleX;
      texture.y2 += (vertex.y2 - original.y2) * scaleY;
      if (texture2)
      {
        scaleX = texture2->Width() / original.Width();
        scaleY = texture2->Height() / original.Height();
        texture2->x1 += (vertex.x1 - original.x1) * scaleX;
        texture2->y1 += (vertex.y1 - original.y1) * scaleY;
        texture2->x2 += (vertex.x2 - original.x2) * scaleX;
        texture2->y2 += (vertex.y2 - original.y2) * scaleY;
      }
    }
  }
}

bool CGraphicContext::SetViewPort(float fx, float fy , float fwidth, float fheight, bool intersectPrevious /* = false */)
{
  D3DVIEWPORT8 newviewport;
  D3DVIEWPORT8 *oldviewport = new D3DVIEWPORT8;
  Get3DDevice()->GetViewport(oldviewport);
  // transform coordinates - we may have a rotation which changes the positioning of the
  // minimal and maximal viewport extents.  We currently go to the maximal extent.
  float x[4], y[4];
  x[0] = x[3] = fx;
  x[1] = x[2] = fx + fwidth;
  y[0] = y[1] = fy;
  y[2] = y[3] = fy + fheight;
  float minX = (float)m_iScreenWidth;
  float maxX = 0;
  float minY = (float)m_iScreenHeight;
  float maxY = 0;
  for (int i = 0; i < 4; i++)
  {
    float z = 0;
    ScaleFinalCoords(x[i], y[i], z);
    if (x[i] < minX) minX = x[i];
    if (x[i] > maxX) maxX = x[i];
    if (y[i] < minY) minY = y[i];
    if (y[i] > maxY) maxY = y[i];
  }

  int newLeft = (int)(minX + 0.5f);
  int newTop = (int)(minY + 0.5f);
  int newRight = (int)(maxX + 0.5f);
  int newBottom = (int)(maxY + 0.5f);
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
  if (newRight <= 0 || newBottom <= 0 ||
      newTop >= m_iScreenHeight || newLeft >= m_iScreenWidth ||
      newLeft >= newRight || newTop >= newBottom)
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
  m_pd3dDevice->SetViewport(&newviewport);
  m_viewStack.push(oldviewport);
 
  UpdateCameraPosition(m_cameras.top());
  return true;
}

void CGraphicContext::RestoreViewPort()
{
  if (!m_viewStack.size()) return;
  D3DVIEWPORT8 *oldviewport = (D3DVIEWPORT8*)m_viewStack.top();
  m_viewStack.pop();
  Get3DDevice()->SetViewport(oldviewport);

  if (oldviewport) delete oldviewport;

  UpdateCameraPosition(m_cameras.top());
}

const RECT& CGraphicContext::GetViewWindow() const
{
  return m_videoRect;
}
void CGraphicContext::SetViewWindow(float left, float top, float right, float bottom)
{
  if (m_bCalibrating)
  {
    SetFullScreenViewWindow(m_Resolution);
  }
  else
  {
    m_videoRect.left = (long)(ScaleFinalXCoord(left, top) + 0.5f);
    m_videoRect.top = (long)(ScaleFinalYCoord(left, top) + 0.5f);
    m_videoRect.right = (long)(ScaleFinalXCoord(right, bottom) + 0.5f);
    m_videoRect.bottom = (long)(ScaleFinalYCoord(right, bottom) + 0.5f);
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
#ifdef HAS_XBOX_D3D
  m_pd3dDevice->SetScissors(1, FALSE, &clip);
#endif
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
  if (g_videoConfig.HasNTSC())
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

void CGraphicContext::SetVideoResolution(RESOLUTION &res, BOOL NeedZ, bool forceClear /* = false */)
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
  //if( m_bFullScreenVideo )
  //  interval = D3DPRESENT_INTERVAL_IMMEDIATE;

#ifdef PROFILE
  interval = D3DPRESENT_INTERVAL_IMMEDIATE;
#endif

#ifndef HAS_XBOX_D3D
  interval = 0;
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
    m_pd3dParams->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

    if (res == HDTV_1080i || res == HDTV_720p || m_bFullScreenVideo)
      m_pd3dParams->BackBufferCount = 1;
    else
      m_pd3dParams->BackBufferCount = 2;

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
  if (m_pd3dDevice)
  {
    if (NeedReset)
    {
      CLog::Log(LOGDEBUG, "Setting resolution %i", res);
      m_pd3dDevice->Reset(m_pd3dParams);
    }

    /* need to clear and preset, otherwise flicker filters won't take effect */
    if (NeedReset || forceClear)
    {
      m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3D_CLEAR_STENCIL, 0x00010001, 1.0f, 0L );
      m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
    }

    m_iScreenWidth = m_pd3dParams->BackBufferWidth;
    m_iScreenHeight = m_pd3dParams->BackBufferHeight;
    m_bWidescreen = (m_pd3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN) != 0;
  }
  if ((g_settings.m_ResInfo[m_Resolution].iWidth != g_settings.m_ResInfo[res].iWidth) || (g_settings.m_ResInfo[m_Resolution].iHeight != g_settings.m_ResInfo[res].iHeight))
  { // set the mouse resolution
    g_Mouse.SetResolution(g_settings.m_ResInfo[res].iWidth, g_settings.m_ResInfo[res].iHeight, 1, 1);
  }

  SetFullScreenViewWindow(res);
  SetScreenFilters(m_bFullScreenVideo);
  
  m_Resolution = res;
  if(NeedReset)
    CLog::Log(LOGDEBUG, "We set resolution %i", m_Resolution);

  Unlock();  
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
#ifdef HAS_XBOX_D3D
    m_pd3dDevice->SetSoftDisplayFilter(useFullScreenFilters ? g_guiSettings.GetBool("videoplayer.soften") : g_guiSettings.GetBool("videoscreen.soften"));
    m_pd3dDevice->SetFlickerFilter(useFullScreenFilters ? g_guiSettings.GetInt("videoplayer.flicker") : g_guiSettings.GetInt("videoscreen.flickerfilter"));
#endif
  }
  Unlock();
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
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    strcpy(g_settings.m_ResInfo[res].strMode, "NTSC 4:3");
    break;
  case NTSC_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "NTSC 16:9");
    break;
  case PAL_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 576);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 576;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL 4:3");
    break;
  case PAL_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 576);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 576;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL 16:9");
    break;
  case PAL60_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL60 4:3");
    break;
  case PAL60_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
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
  if (!m_pd3dDevice) return;
  //Not trying to clear the zbuffer when there is none is 7 fps faster (pal resolution)
  if ((!m_pd3dParams) || (m_pd3dParams->EnableAutoDepthStencil == TRUE))
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3D_CLEAR_STENCIL, 0x00010001, 1.0f, 0L );
  else
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, 0x00010001, 1.0f, 0L );
}

void CGraphicContext::CaptureStateBlock()
{
  if (m_stateBlock != 0xffffffff)
  {
    Get3DDevice()->DeleteStateBlock(m_stateBlock);
  }

  if (D3D_OK != Get3DDevice()->CreateStateBlock(D3DSBT_PIXELSTATE, &m_stateBlock))
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

void CGraphicContext::SetScalingResolution(RESOLUTION res, float posX, float posY, bool needsScaling)
{
  m_windowResolution = res;
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
    
    m_guiScaleX = fFromWidth / fToWidth;
    m_guiScaleY = fFromHeight / fToHeight;
    TransformMatrix windowOffset = TransformMatrix::CreateTranslation(posX, posY);
    TransformMatrix guiScaler = TransformMatrix::CreateScaler(fToWidth / fFromWidth, fToHeight / fFromHeight, fToHeight / fFromHeight);
    TransformMatrix guiOffset = TransformMatrix::CreateTranslation(fToPosX, fToPosY);
    m_guiTransform = guiOffset * guiScaler * windowOffset;
  }
  else
  {
    m_guiTransform = TransformMatrix::CreateTranslation(posX, posY);
    m_guiScaleX = 1.0f;
    m_guiScaleY = 1.0f;
  }
  // reset our origin and camera
  while (m_origins.size())
    m_origins.pop();
  m_origins.push(CPoint(posX, posY));
  while (m_cameras.size())
    m_cameras.pop();
  m_cameras.push(CPoint(0.5f*m_iScreenWidth, 0.5f*m_iScreenHeight));
  UpdateCameraPosition(m_cameras.top());

  // and reset the final transform
  UpdateFinalTransform(m_guiTransform);
}

void CGraphicContext::UpdateFinalTransform(const TransformMatrix &matrix)
{
  m_finalTransform = matrix;
  // We could set the world transform here to GPU-ize the animation system.
  // trouble is that we require the resulting x,y coords to be rounded to
  // the nearest pixel (vertex shader perhaps?)
}

void CGraphicContext::InvertFinalCoords(float &x, float &y) const
{
  m_finalTransform.InverseTransformPosition(x, y);
}

float CGraphicContext::GetScalingPixelRatio() const
{
  if (m_Resolution == m_windowResolution)
    return GetPixelRatio(m_windowResolution);

  RESOLUTION checkRes = m_windowResolution;
  if (checkRes == INVALID)
    checkRes = m_Resolution;
  // resolutions are different - we want to return the aspect ratio of the video resolution
  // but only once it's been corrected for the skin -> screen coordinates scaling
  float winWidth = (float)g_settings.m_ResInfo[checkRes].iWidth;
  float winHeight = (float)g_settings.m_ResInfo[checkRes].iHeight;
  float outWidth = (float)g_settings.m_ResInfo[m_Resolution].iWidth;
  float outHeight = (float)g_settings.m_ResInfo[m_Resolution].iHeight;
  float outPR = GetPixelRatio(m_Resolution);

  return outPR * (outWidth / outHeight) / (winWidth / winHeight);
}

void CGraphicContext::SetCameraPosition(const CPoint &camera)
{
  // offset the camera from our current location (this is in XML coordinates) and scale it up to
  // the screen resolution
  CPoint cam(camera);
  if (m_origins.size())
    cam += m_origins.top();

  RESOLUTION windowRes = (m_windowResolution == INVALID) ? m_Resolution : m_windowResolution;
  cam.x *= (float)m_iScreenWidth / g_settings.m_ResInfo[windowRes].iWidth;
  cam.y *= (float)m_iScreenHeight / g_settings.m_ResInfo[windowRes].iHeight;

  m_cameras.push(cam);
  UpdateCameraPosition(m_cameras.top());
}

void CGraphicContext::RestoreCameraPosition()
{ // remove the top camera from the stack
  ASSERT(m_cameras.size());
  m_cameras.pop();
  UpdateCameraPosition(m_cameras.top());
}

void CGraphicContext::UpdateCameraPosition(const CPoint &camera)
{
  // NOTE: This routine is currently called (twice) every time there is a <camera>
  //       tag in the skin.  It actually only has to be called before we render
  //       something, so another option is to just save the camera coordinates
  //       and then have a routine called before every draw that checks whether
  //       the camera has changed, and if so, changes it.  Similarly, it could set
  //       the world transform at that point as well (or even combine world + view
  //       to cut down on one setting)
 
  // and calculate the offset from the screen center
  CPoint offset = camera - CPoint(m_iScreenWidth*0.5f, m_iScreenHeight*0.5f);

  // grab the viewport dimensions and location
  D3DVIEWPORT8 viewport;
  m_pd3dDevice->GetViewport(&viewport);
  float w = viewport.Width*0.5f;
  float h = viewport.Height*0.5f;

  // world view.  Until this is moved onto the GPU (via a vertex shader for instance), we set it to the identity
  // here.
  D3DXMATRIX mtxWorld;
  D3DXMatrixIdentity(&mtxWorld);
  m_pd3dDevice->SetTransform(D3DTS_WORLD, &mtxWorld);

  // camera view.  Multiply the Y coord by -1 then translate so that everything is relative to the camera
  // position.
  D3DXMATRIX flipY, translate, mtxView;
  D3DXMatrixScaling(&flipY, 1.0f, -1.0f, 1.0f);
  D3DXMatrixTranslation(&translate, -(viewport.X + w + offset.x), -(viewport.Y + h + offset.y), 2*h);
  D3DXMatrixMultiply(&mtxView, &translate, &flipY);
  m_pd3dDevice->SetTransform(D3DTS_VIEW, &mtxView);

  // projection onto screen space
  D3DXMATRIX mtxProjection;
  D3DXMatrixPerspectiveOffCenterLH(&mtxProjection, (-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100*h);
  m_pd3dDevice->SetTransform(D3DTS_PROJECTION, &mtxProjection);
}

bool CGraphicContext::RectIsAngled(float x1, float y1, float x2, float y2) const
{ // need only test 3 points, as they must be co-planer
  if (m_finalTransform.TransformZCoord(x1, y1, 0)) return true;
  if (m_finalTransform.TransformZCoord(x2, y2, 0)) return true;
  if (m_finalTransform.TransformZCoord(x1, y2, 0)) return true;
  return false;
}

int CGraphicContext::GetFPS() const
{
  if (m_Resolution == PAL_4x3 || m_Resolution == PAL_16x9)
    return 50;
  else if (m_Resolution == HDTV_1080i)
    return 30;
  return 60;
}