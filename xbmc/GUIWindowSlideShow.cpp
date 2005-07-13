
#include "stdafx.h"
#include "GUIWindowSlideShow.h"
#include "Application.h"
#include "Picture.h"
#include "Util.h"
#include "TextureManager.h"
#include "GUILabelControl.h"
#include "cores/ssrc.h"
#include "GUIFontManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MAX_RENDER_METHODS 9
#define MAX_ZOOM_FACTOR    10
#define MAX_PICTURE_WIDTH  4096
#define MAX_PICTURE_HEIGHT 4096
#define MAX_PICTURE_SIZE 2048*2048

#define IMMEDIATE_TRANSISTION_TIME 20

#define PICTURE_MOVE_AMOUNT     0.02f
#define PICTURE_MOVE_AMOUNT_ANALOG 0.01f
#define PICTURE_VIEW_BOX_COLOR   0xffffff00 // YELLOW
#define PICTURE_VIEW_BOX_BACKGROUND 0xff000000 // BLACK

#define FPS        25

#define BAR_IMAGE     1
#define LABEL_ROW1    10
#define LABEL_ROW2    11
#define LABEL_ROW2_EXTRA 12
#define CONTROL_PAUSE   13

static float zoomamount[10] = { 1.0f, 1.2f, 1.5f, 2.0f, 2.8f, 4.0f, 6.0f, 9.0f, 13.5f, 20.0f };

CBackgroundPicLoader::CBackgroundPicLoader()
{
  m_pCallback = NULL;
}

CBackgroundPicLoader::~CBackgroundPicLoader()
{}

void CBackgroundPicLoader::Create(CGUIWindowSlideShow *pCallback)
{
  m_pCallback = pCallback;
  m_bLoadPic = false;
  CThread::Create(false);
}

void CBackgroundPicLoader::Process()
{
  while (!m_bStop)
  { // loop around forever, waiting for the app to call LoadPic
    while (!m_bLoadPic && !m_bStop)
    {
      Sleep(10);
    }
    if (m_bLoadPic)
    { // m_bLoadPic = true, indicating LoadPic() has been called
      if (m_pCallback)
      {
        CPicture pic;
        D3DTexture *pTexture = pic.Load(m_strFileName, m_maxWidth, m_maxHeight, g_guiSettings.GetBool("Slideshow.GenerateThumbs"));
        // tell our parent
        bool bFullSize = ((int)pic.GetWidth() < m_maxWidth) && ((int)pic.GetHeight() < m_maxHeight);
        if (!bFullSize)
        {
          int iSize = pic.GetWidth() * pic.GetHeight() - MAX_PICTURE_SIZE;
          if ((iSize + (int)pic.GetWidth() > 0) || (iSize + (int)pic.GetHeight() > 0))
            bFullSize = true;
          if (!bFullSize && pic.GetWidth() == MAX_PICTURE_WIDTH)
            bFullSize = true;
          if (!bFullSize && pic.GetHeight() == MAX_PICTURE_HEIGHT)
            bFullSize = true;
        }
        m_pCallback->OnLoadPic(m_iPic, m_iSlideNumber, pTexture, pic.GetWidth(), pic.GetHeight(), pic.GetOriginalWidth(), pic.GetOriginalHeight(), pic.GetExifOrientation(), bFullSize);
      }
    }
    m_bLoadPic = false;
  }
}

void CBackgroundPicLoader::LoadPic(int iPic, int iSlideNumber, const CStdString &strFileName, const int maxWidth, const int maxHeight)
{
  m_iPic = iPic;
  m_iSlideNumber = iSlideNumber;
  m_strFileName = strFileName;
  m_maxWidth = maxWidth;
  m_maxHeight = maxHeight;
  m_bLoadPic = true;
}

CSlideShowPic::CSlideShowPic()
{
  m_pImage = NULL;
  m_bIsLoaded = false;
  m_bIsFinished = false;
  m_bDrawNextImage = false;
}

CSlideShowPic::~CSlideShowPic()
{
  Close();
}

void CSlideShowPic::Close()
{
  if (m_pImage)
  {
    m_pImage->Release();
    m_pImage = NULL;
  }
  m_bIsLoaded = false;
  m_bIsFinished = false;
  m_bDrawNextImage = false;
}

void CSlideShowPic::SetTexture(int iSlideNumber, D3DTexture *pTexture, int iWidth, int iHeight, int iRotate, DISPLAY_EFFECT dispEffect, TRANSISTION_EFFECT transEffect)
{
  Close();
  m_bPause = false;
  m_bNoEffect = false;
  m_iSlideNumber = iSlideNumber;
  m_pImage = pTexture;
  m_fWidth = (float)iWidth;
  m_fHeight = (float)iHeight;
  // reset our counter
  m_iCounter = 0;
  // initialize our transistion effect
  m_transistionStart.type = transEffect;
  m_transistionStart.start = 0;
  m_transistionStart.length = 2 * g_guiSettings.GetInt("Slideshow.TransistionTime") / (1000 / FPS);
  m_transistionEnd.type = transEffect;
  m_transistionEnd.start = m_transistionStart.length + g_guiSettings.GetInt("Slideshow.StayTime") * FPS;
  m_transistionEnd.length = 2 * g_guiSettings.GetInt("Slideshow.TransistionTime") / (1000 / FPS);
  m_transistionTemp.type = TRANSISTION_NONE;
  m_fTransistionAngle = 0;
  m_fTransistionZoom = 0;
  m_fAngle = 0;
  if (iRotate == 8)
  { // rotate to 270 degrees
    m_fAngle = 3.0f;
  }
  if (iRotate == 6)
  { // rotate to 90 degrees
    m_fAngle = 1.0f;
  }
  m_fZoomAmount = 1;
  m_fZoomLeft = 0;
  m_fZoomTop = 0;
  m_iTotalFrames = m_transistionStart.length + m_transistionEnd.length + g_guiSettings.GetInt("Slideshow.StayTime") * FPS;
  // initialize our display effect
  if (dispEffect == EFFECT_RANDOM)
    m_displayEffect = (DISPLAY_EFFECT)((rand() % (EFFECT_RANDOM - 1)) + 1);
  else
    m_displayEffect = dispEffect;
  m_fPosX = m_fPosY = 0.0f;
  m_fPosZ = 1.0f;
  m_fVelocityX = m_fVelocityY = m_fVelocityZ = 0.0f;
  if (m_displayEffect == EFFECT_FLOAT)
  {
    // Calculate start and end positions
    // choose a random direction
    float angle = (rand() % 1000) / 1000.0f * 2 * (float)M_PI;
    m_fPosX = cos(angle) * g_guiSettings.GetInt("Slideshow.MoveAmount") / 100.0f;
    m_fPosY = sin(angle) * g_guiSettings.GetInt("Slideshow.MoveAmount") / 100.0f;
    m_fVelocityX = -m_fPosX * 2.0f / (float)m_iTotalFrames;
    m_fVelocityY = -m_fPosY * 2.0f / (float)m_iTotalFrames;
  }
  else if (m_displayEffect == EFFECT_ZOOM)
  {
    m_fPosZ = 1.0f;
    m_fVelocityZ = (0.01f * g_guiSettings.GetInt("Slideshow.ZoomAmount")) / m_iTotalFrames;
  }

  m_bIsFinished = false;
  m_bDrawNextImage = false;
  m_bIsLoaded = true;
  return ;
}

void CSlideShowPic::SetOriginalSize(int iOriginalWidth, int iOriginalHeight, bool bFullSize)
{
  m_iOriginalWidth = iOriginalWidth;
  m_iOriginalHeight = iOriginalHeight;
  m_bFullSize = bFullSize;
}

int CSlideShowPic::GetOriginalWidth()
{
  int iAngle = (int)(m_fAngle + 0.4f);
  if (iAngle % 2)
    return m_iOriginalHeight;
  else
    return m_iOriginalWidth;
}

int CSlideShowPic::GetOriginalHeight()
{
  int iAngle = (int)(m_fAngle + 0.4f);
  if (iAngle % 2)
    return m_iOriginalWidth;
  else
    return m_iOriginalHeight;
}

void CSlideShowPic::UpdateTexture(D3DTexture *pTexture, int iWidth, int iHeight)
{
  if (m_pImage)
  {
    while (m_pImage->IsBusy())
      Sleep(1);
    m_pImage->Release();
  }
  m_pImage = pTexture;
  m_fWidth = (float)iWidth;
  m_fHeight = (float)iHeight;
}

void CSlideShowPic::Process()
{
  if (!m_pImage || !m_bIsLoaded || m_bIsFinished) return ;
  if (m_iCounter < m_transistionStart.length)
  { // do start transistion
    if (m_transistionStart.type == CROSSFADE)
    { // fade in at 1x speed
      m_dwAlpha = (DWORD)((float)m_iCounter / (float)m_transistionStart.length * 255.0f);
    }
    else if (m_transistionStart.type == FADEIN_FADEOUT)
    { // fade in at 2x speed, then keep solid
      m_dwAlpha = (DWORD)((float)m_iCounter / (float)m_transistionStart.length * 255.0f * 2);
      if (m_dwAlpha > 255) m_dwAlpha = 255;
    }
    else // m_transistionEffect == TRANSISTION_NONE
    {
      m_dwAlpha = 0xFF; // opaque
    }
  }
  bool bPaused = m_bPause;
  // check if we're doing a temporary effect (such as rotate + zoom)
  if (m_transistionTemp.type != TRANSISTION_NONE)
  {
    bPaused = true;
    if (m_iCounter >= m_transistionTemp.start)
    {
      if (m_iCounter >= m_transistionTemp.start + m_transistionTemp.length)
      { // we're finished this transistion
        if (m_transistionTemp.type == TRANSISTION_ZOOM)
        { // correct for any introduced inaccuracies.
          int i;
          for (i = 0; i < 10; i++)
            if (fabs(m_fZoomAmount - zoomamount[i]) < 0.01*zoomamount[i])
              break;
          m_fZoomAmount = zoomamount[i];
          m_bNoEffect = (m_fZoomAmount != 1.0f); // turn effect rendering back on.
        }
        if (m_transistionTemp.type == TRANSISTION_ROTATE)
        { // round to nearest integer for accuracy purposes
          m_fAngle = floor(m_fAngle + 0.4f);
        }
        m_transistionTemp.type = TRANSISTION_NONE;
      }
      else
      {
        if (m_transistionTemp.type == TRANSISTION_ROTATE)
        {
          m_fAngle += m_fTransistionAngle;
        }
        if (m_transistionTemp.type == TRANSISTION_ZOOM)
        {
          m_fZoomAmount += m_fTransistionZoom;
        }
      }
    }
  }
  // now just display
  if (!m_bNoEffect && !bPaused)
  {
    if (m_displayEffect == EFFECT_FLOAT)
    {
      m_fPosX += m_fVelocityX;
      m_fPosY += m_fVelocityY;
      float fMoveAmount = 0.01f * g_guiSettings.GetInt("Slideshow.MoveAmount");
      if (m_fPosX > fMoveAmount)
      {
        m_fPosX = fMoveAmount;
        m_fVelocityX = -m_fVelocityX;
      }
      if (m_fPosX < -fMoveAmount)
      {
        m_fPosX = -fMoveAmount;
        m_fVelocityX = -m_fVelocityX;
      }
      if (m_fPosY > fMoveAmount)
      {
        m_fPosY = fMoveAmount;
        m_fVelocityY = -m_fVelocityY;
      }
      if (m_fPosY < -fMoveAmount)
      {
        m_fPosY = -fMoveAmount;
        m_fVelocityY = -m_fVelocityY;
      }
    }
    else if (m_displayEffect == EFFECT_ZOOM)
    {
      m_fPosZ += m_fVelocityZ;
      if (m_fPosZ > 1.0f + 0.01f*g_guiSettings.GetInt("Slideshow.ZoomAmount"))
      {
        m_fPosZ = 1.0f + 0.01f * g_guiSettings.GetInt("Slideshow.ZoomAmount");
        m_fVelocityZ = -m_fVelocityZ;
      }
      if (m_fPosZ < 1.0f)
      {
        m_fPosZ = 1.0f;
        m_fVelocityZ = -m_fVelocityZ;
      }
    }
  }
  if (bPaused)
  { // paused - increment the last transistion start time
    m_transistionEnd.start++;
  }
  if (m_iCounter >= m_transistionEnd.start)
  { // do end transistion
    CLog::DebugLog("Transistioning");
    m_bDrawNextImage = true;
    if (m_transistionEnd.type == CROSSFADE)
    { // fade out at 1x speed
      m_dwAlpha = 255 - (DWORD)((float)(m_iCounter - m_transistionEnd.start) / (float)m_transistionEnd.length * 255.0f);
    }
    else if (m_transistionEnd.type == FADEIN_FADEOUT)
    { // keep solid, then fade out at 2x speed
      m_dwAlpha = (DWORD)((float)(m_transistionEnd.length - m_iCounter + m_transistionEnd.start) / (float)m_transistionEnd.length * 255.0f * 2);
      if (m_dwAlpha > 255) m_dwAlpha = 255;
    }
    else // m_transistionEffect == TRANSISTION_NONE
    {
      m_dwAlpha = 0xFF; // opaque
    }
  }
  if (m_displayEffect != EFFECT_NO_TIMEOUT || m_iCounter < m_transistionStart.length || m_iCounter >= m_transistionEnd.start || (m_iCounter >= m_transistionTemp.start && m_iCounter < m_transistionTemp.start + m_transistionTemp.length))
    m_iCounter ++;
  if (m_iCounter > m_transistionEnd.start + m_transistionEnd.length)
    m_bIsFinished = true;
}

void CSlideShowPic::Keep()
{
  // this is called if we need to keep the current pic on screen
  // to wait for the next pic to load
  if (!m_bDrawNextImage) return ; // don't need to keep pic
  // hold off the start of the next frame
  m_transistionEnd.start = m_iCounter;
}

void CSlideShowPic::StartTransistion()
{
  // this is called if we need to start transistioning immediately to the new picture
  if (m_bDrawNextImage) return ; // don't need to do anything as we are already transistioning
  // decrease the number of display frame
  m_transistionEnd.start = m_iCounter;
}

int CSlideShowPic::GetTransistionTime(int iType) const
{
  if (iType == 0) // start transistion
    return m_transistionStart.length;
  else // iType == 1 // end transistion
    return m_transistionEnd.length;
}

void CSlideShowPic::SetTransistionTime(int iType, int iTime)
{
  if (iType == 0) // start transistion
    m_transistionStart.length = iTime;
  else // iType == 1 // end transistion
    m_transistionEnd.length = iTime;
}

void CSlideShowPic::Rotate(int iRotate)
{
  if (m_bDrawNextImage) return ;
  if (m_transistionTemp.type == TRANSISTION_ZOOM) return ;
  m_transistionTemp.type = TRANSISTION_ROTATE;
  m_transistionTemp.start = m_iCounter;
  m_transistionTemp.length = IMMEDIATE_TRANSISTION_TIME;
  m_fTransistionAngle = (float)(iRotate - m_fAngle) / (float)m_transistionTemp.length;
  // reset the timer
  m_transistionEnd.start = m_iCounter + m_transistionStart.length + g_guiSettings.GetInt("Slideshow.StayTime") * FPS;
}

void CSlideShowPic::Zoom(int iZoom, bool immediate /*= false*/)
{
  if (m_bDrawNextImage) return ;
  if (m_transistionTemp.type == TRANSISTION_ROTATE) return ;
  if (immediate)
  {
    m_fZoomAmount = zoomamount[iZoom - 1];
    return;
  }
  m_transistionTemp.type = TRANSISTION_ZOOM;
  m_transistionTemp.start = m_iCounter;
  m_transistionTemp.length = IMMEDIATE_TRANSISTION_TIME;
  m_fTransistionZoom = (float)(zoomamount[iZoom - 1] - m_fZoomAmount) / (float)m_transistionTemp.length;
  // reset the timer
  m_transistionEnd.start = m_iCounter + m_transistionStart.length + g_guiSettings.GetInt("Slideshow.StayTime") * FPS;
  // turn off the render effects until we're back down to normal zoom
  m_bNoEffect = true;
}

void CSlideShowPic::Move(float fDeltaX, float fDeltaY)
{
  m_fZoomLeft += fDeltaX;
  m_fZoomTop += fDeltaY;
  // reset the timer
  m_transistionEnd.start = m_iCounter + m_transistionStart.length + g_guiSettings.GetInt("Slideshow.StayTime") * FPS;
}

void CSlideShowPic::Render()
{
  if (!m_pImage || !m_bIsLoaded || m_bIsFinished) return ;
  // update the image
  Process();
  // calculate where we should render (and how large it should be)
  // calculate aspect ratio correction factor
  RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
  float fOffsetX = (float)g_settings.m_ResInfo[iRes].Overscan.left;
  float fOffsetY = (float)g_settings.m_ResInfo[iRes].Overscan.top;
  float fScreenWidth = (float)g_settings.m_ResInfo[iRes].Overscan.right - g_settings.m_ResInfo[iRes].Overscan.left;
  float fScreenHeight = (float)g_settings.m_ResInfo[iRes].Overscan.bottom - g_settings.m_ResInfo[iRes].Overscan.top;

  float fPixelRatio = g_settings.m_ResInfo[iRes].fPixelRatio;

  // Rotate the image as needed
  float x[4];
  float y[4];
  float si = (float)sin(m_fAngle * M_PI * 0.5);
  float co = (float)cos(m_fAngle * M_PI * 0.5);
  x[0] = -m_fWidth * co + m_fHeight * si;
  y[0] = -m_fWidth * si - m_fHeight * co;
  x[1] = m_fWidth * co + m_fHeight * si;
  y[1] = m_fWidth * si - m_fHeight * co;
  x[2] = m_fWidth * co - m_fHeight * si;
  y[2] = m_fWidth * si + m_fHeight * co;
  x[3] = -m_fWidth * co - m_fHeight * si;
  y[3] = -m_fWidth * si + m_fHeight * co;

  // calculate our scale amounts
  float fSourceAR = m_fWidth / m_fHeight;
  float fSourceInvAR = 1 / fSourceAR;
  float fAR = si * si * (fSourceInvAR - fSourceAR) + fSourceAR;

  float fOutputFrameAR = fAR / fPixelRatio;

  float fScaleNorm = fScreenWidth / m_fWidth;
  float fScaleInv = fScreenWidth / m_fHeight;

  bool bFillScreen = false;
  float fComp = 1.0f + 0.01f * g_guiSettings.GetInt("Slideshow.BlackBarCompensation");
  float fScreenRatio = fScreenWidth / fScreenHeight * fPixelRatio;
  // work out if we should be compensating the zoom to minimize blackbars
  // we should compute this based on the % of black bars on screen perhaps??
  // TODO: change m_displayEffect != EFFECT_NO_TIMEOUT to whether we're running the slideshow
  if (m_displayEffect != EFFECT_NO_TIMEOUT && fScreenRatio < fSourceAR * fComp && fSourceAR < fScreenRatio * fComp)
    bFillScreen = true;
  if ((!bFillScreen && fScreenWidth*fPixelRatio > fScreenHeight*fSourceAR) || (bFillScreen && fScreenWidth*fPixelRatio < fScreenHeight*fSourceAR))
    fScaleNorm = fScreenHeight / (m_fHeight * fPixelRatio);
  bFillScreen = false;
  if (m_displayEffect != EFFECT_NO_TIMEOUT && fScreenRatio < fSourceInvAR * fComp && fSourceInvAR < fScreenRatio * fComp)
    bFillScreen = true;
  if ((!bFillScreen && fScreenWidth*fPixelRatio > fScreenHeight*fSourceInvAR) || (bFillScreen && fScreenWidth*fPixelRatio < fScreenHeight*fSourceInvAR))
    fScaleInv = fScreenHeight / (m_fWidth * fPixelRatio);

  float fScale = si * si * (fScaleInv - fScaleNorm) + fScaleNorm;
  // scale if we need to due to the effect we're using
  if (m_displayEffect == EFFECT_FLOAT)
    fScale *= (1.0f + 0.02f * g_guiSettings.GetInt("Slideshow.MoveAmount"));
  if (m_displayEffect == EFFECT_ZOOM)
    fScale *= m_fPosZ;
  // zoom image
  fScale *= m_fZoomAmount;

  // calculate the resultant coordinates
  for (int i = 0; i < 4; i++)
  {
    x[i] *= fScale * 0.5f; // as the offsets x[] and y[] are from center
    y[i] *= fScale * fPixelRatio * 0.5f;
    // center it
    x[i] += 0.5f * fScreenWidth + fOffsetX;
    y[i] += 0.5f * fScreenHeight + fOffsetY;
  }
  // shift if we're zooming
  if (m_fZoomAmount > 1)
  {
    float minx = x[0];
    float maxx = x[0];
    float miny = y[0];
    float maxy = y[0];
    for (int i = 1; i < 4; i++)
    {
      if (x[i] < minx) minx = x[i];
      if (x[i] > maxx) maxx = x[i];
      if (y[i] < miny) miny = y[i];
      if (y[i] > maxy) maxy = y[i];
    }
    float w = maxx - minx;
    float h = maxy - miny;
    if (w >= fScreenWidth)
    { // must have no black bars
      if (minx + m_fZoomLeft*w > fOffsetX)
        m_fZoomLeft = (fOffsetX - minx) / w;
      if (maxx + m_fZoomLeft*w < fOffsetX + fScreenWidth)
        m_fZoomLeft = (fScreenWidth + fOffsetX - maxx) / w;
      for (int i = 0; i < 4; i++)
        x[i] += w * m_fZoomLeft;
    }
    if (h >= fScreenHeight)
    { // must have no black bars
      if (miny + m_fZoomTop*h > fOffsetY)
        m_fZoomTop = (fOffsetY - miny) / h;
      if (maxy + m_fZoomTop*h < fOffsetY + fScreenHeight)
        m_fZoomTop = (fScreenHeight + fOffsetY - maxy) / h;
      for (int i = 0; i < 4; i++)
        y[i] += m_fZoomTop * h;
    }
  }
  // add offset from display effects
  for (int i = 0; i < 4; i++)
  {
    x[i] += m_fPosX * m_fWidth * fScale;
    y[i] += m_fPosY * m_fHeight * fScale;
  }
  // and render
  Render(x, y, m_pImage, (m_dwAlpha << 24) | 0xFFFFFF);

  // now render the image in the top right corner if we're zooming
  if (m_fZoomAmount == 1) return ;

  float sx[4], sy[4];
  sx[0] = -m_fWidth * co + m_fHeight * si;
  sy[0] = -m_fWidth * si - m_fHeight * co;
  sx[1] = m_fWidth * co + m_fHeight * si;
  sy[1] = m_fWidth * si - m_fHeight * co;
  sx[2] = m_fWidth * co - m_fHeight * si;
  sy[2] = m_fWidth * si + m_fHeight * co;
  sx[3] = -m_fWidth * co - m_fHeight * si;
  sy[3] = -m_fWidth * si + m_fHeight * co;
  // convert to the appropriate scale
  float fSmallArea = fScreenWidth * fScreenHeight / 50;
  float fSmallWidth = sqrt(fSmallArea * fAR / fPixelRatio); // fAR*height = width, so total area*far = width*width
  float fSmallHeight = fSmallArea / fSmallWidth;
  float fSmallX = fOffsetX + fScreenWidth * 0.95f - fSmallWidth * 0.5f;
  float fSmallY = fOffsetY + fScreenHeight * 0.05f + fSmallHeight * 0.5f;
  fScaleNorm = fSmallWidth / m_fWidth;
  fScaleInv = fSmallWidth / m_fHeight;
  fScale = si * si * (fScaleInv - fScaleNorm) + fScaleNorm;
  for (int i = 0; i < 4; i++)
  {
    sx[i] *= fScale * 0.5f;
    sy[i] *= fScale * fPixelRatio * 0.5f;
  }
  // calculate a black border
  float bx[4];
  float by[4];
  for (int i = 0; i < 4; i++)
  {
    if (sx[i] > 0)
      bx[i] = sx[i] + 1;
    else
      bx[i] = sx[i] - 1;
    if (sy[i] > 0)
      by[i] = sy[i] + 1;
    else
      by[i] = sy[i] - 1;
    sx[i] += fSmallX;
    sy[i] += fSmallY;
    bx[i] += fSmallX;
    by[i] += fSmallY;
  }

  fSmallX -= fSmallWidth * 0.5f;
  fSmallY -= fSmallHeight * 0.5f;

  Render(bx, by, NULL, PICTURE_VIEW_BOX_BACKGROUND);
  Render(sx, sy, m_pImage, 0xFFFFFFFF);

  // now we must render the wireframe image of the view window
  // work out the direction of the top of pic vector
  float scale;
  if (fabs(x[1] - x[0]) > fabs(x[3] - x[0]))
    scale = (sx[1] - sx[0]) / (x[1] - x[0]);
  else
    scale = (sx[3] - sx[0]) / (x[3] - x[0]);
  float ox[4];
  float oy[4];
  ox[0] = (fOffsetX - x[0]) * scale + sx[0];
  oy[0] = (fOffsetY - y[0]) * scale + sy[0];
  ox[1] = (fOffsetX + fScreenWidth - x[0]) * scale + sx[0];
  oy[1] = (fOffsetY - y[0]) * scale + sy[0];
  ox[2] = (fOffsetX + fScreenWidth - x[0]) * scale + sx[0];
  oy[2] = (fOffsetY + fScreenHeight - y[0]) * scale + sy[0];
  ox[3] = (fOffsetX - x[0]) * scale + sx[0];
  oy[3] = (fOffsetY + fScreenHeight - y[0]) * scale + sy[0];
  // crop to within the range of our piccy
  for (int i = 0; i < 4; i++)
  {
    if (ox[i] < fSmallX) ox[i] = fSmallX;
    if (ox[i] > fSmallX + fSmallWidth) ox[i] = fSmallX + fSmallWidth;
    if (oy[i] < fSmallY) oy[i] = fSmallY;
    if (oy[i] > fSmallY + fSmallHeight) oy[i] = fSmallY + fSmallHeight;
  }
  Render(ox, oy, NULL, PICTURE_VIEW_BOX_COLOR, D3DFILL_WIREFRAME);
}

void CSlideShowPic::Render(float *x, float *y, D3DTexture *pTexture, DWORD dwColor, _D3DFILLMODE fillmode)
{
  VERTEX* vertex = NULL;
  LPDIRECT3DVERTEXBUFFER8 pVB;

  g_graphicsContext.Get3DDevice()->CreateVertexBuffer( 4*sizeof(VERTEX), D3DUSAGE_WRITEONLY, 0L, D3DPOOL_DEFAULT, &pVB );
  pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );

  for (int i = 0; i < 4; i++)
  {
    vertex[i].p = D3DXVECTOR4( x[i], y[i], 0, 0 );
    vertex[i].tu = 0;
    vertex[i].tv = 0;
    vertex[i].col = dwColor;
  }
  vertex[1].tu = m_fWidth;
  vertex[2].tu = m_fWidth;
  vertex[2].tv = m_fHeight;
  vertex[3].tv = m_fHeight;

  pVB->Unlock();

  // Set state to render the image
  if (pTexture) g_graphicsContext.Get3DDevice()->SetTexture( 0, pTexture );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR /*g_stSettings.m_minFilter*/ );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR /*g_stSettings.m_maxFilter*/ );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ZENABLE, FALSE );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE, FALSE );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FILLMODE, fillmode );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_YUVENABLE, FALSE);
  g_graphicsContext.Get3DDevice()->SetVertexShader( FVF_VERTEX );
  // Render the image
  g_graphicsContext.Get3DDevice()->SetStreamSource( 0, pVB, sizeof(VERTEX) );
  g_graphicsContext.Get3DDevice()->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );
  if (pTexture) g_graphicsContext.Get3DDevice()->SetTexture(0, NULL);
  pVB->Release();
}

CGUIWindowSlideShow::CGUIWindowSlideShow(void)
    : CGUIWindow(0)
{
  m_pBackgroundLoader = NULL;
  Reset();
  srand( timeGetTime() );
}

CGUIWindowSlideShow::~CGUIWindowSlideShow(void)
{
  Reset();
}


bool CGUIWindowSlideShow::IsPlaying() const
{
  return m_Image[m_iCurrentPic].IsLoaded();
}

void CGUIWindowSlideShow::Reset()
{
  m_bShowInfo = false;
  m_bSlideShow = false;
  m_bPause = false;
  m_bErrorMessage = false;
  m_bReloadImage = false;
  m_Image[0].UnLoad();

  m_iRotate = 0;
  m_iZoomFactor = 1;
  m_iCurrentSlide = 0;
  m_iNextSlide = 1;
  m_iCurrentPic = 0;
  m_vecSlides.erase(m_vecSlides.begin(), m_vecSlides.end());
}

void CGUIWindowSlideShow::FreeResources()
{ // wait for any outstanding picture loads
  if (m_pBackgroundLoader)
  {
    // sleep until the loader finishes loading the current pic
    CLog::DebugLog("Waiting for BackgroundLoader thread to close");
    while (m_pBackgroundLoader->IsLoading())
      Sleep(10);
    // stop the thread
    CLog::DebugLog("Stopping BackgroundLoader thread");
    m_pBackgroundLoader->StopThread();
    delete m_pBackgroundLoader;
    m_pBackgroundLoader = NULL;
  }
  // and close the images.
  m_Image[0].Close();
  m_Image[1].Close();
}

void CGUIWindowSlideShow::Add(const CStdString& strPicture)
{
  m_vecSlides.push_back(strPicture);
}

void CGUIWindowSlideShow::ShowNext()
{
  m_iNextSlide = m_iCurrentSlide + 1;
  if (m_iNextSlide >= (int)m_vecSlides.size())
    m_iNextSlide = 0;
  m_bLoadNextPic = true;
}

void CGUIWindowSlideShow::ShowPrevious()
{
  m_iNextSlide = m_iCurrentSlide - 1;
  if (m_iNextSlide < 0)
    m_iNextSlide = m_vecSlides.size() - 1;
  m_bLoadNextPic = true;
}


void CGUIWindowSlideShow::Select(const CStdString& strPicture)
{
  for (int i = 0; i < (int)m_vecSlides.size(); ++i)
  {
    CStdString& strSlide = m_vecSlides[i];
    if (strSlide == strPicture)
    {
      m_iCurrentSlide = i;
      m_iNextSlide = m_iCurrentSlide + 1;
      if (m_iNextSlide >= (int)m_vecSlides.size())
        m_iNextSlide = 0;
      return ;
    }
  }
}

vector<CStdString> CGUIWindowSlideShow::GetSlideShowContents()
{
  return m_vecSlides ;
}

CStdString CGUIWindowSlideShow::GetCurrentSlide()
{
  if (m_vecSlides.size()>(unsigned int)m_iCurrentSlide)
    return m_vecSlides[m_iCurrentSlide];
  else
    return "";
}

bool CGUIWindowSlideShow::GetCurrentSlideInfo(int &width, int &height)
{
  
if (m_Image[m_iCurrentPic].IsLoaded())
{
  width=m_Image[m_iCurrentPic].GetWidth();
  height=m_Image[m_iCurrentPic].GetHeight();
  return true;
}
else
  return false;
}

bool CGUIWindowSlideShow::InSlideShow() const
{
  return m_bSlideShow;
}

void CGUIWindowSlideShow::StartSlideShow()
{
  m_bSlideShow = true;
}

void CGUIWindowSlideShow::Render()
{
  // reset the screensaver if we're in a slideshow
  if (m_bSlideShow) g_application.ResetScreenSaver();
  int iSlides = m_vecSlides.size();
  if (!iSlides) return ;
  //  g_graphicsContext.Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);

  // Create our background loader if necessary
  if (!m_pBackgroundLoader)
  {
    m_pBackgroundLoader = new CBackgroundPicLoader();

    if (!m_pBackgroundLoader)
    {
      throw 1;
    }
    m_pBackgroundLoader->Create(this);
  }

  if (m_bErrorMessage)
  { // we have an error when loading either the current or next picture
    // check to see if we have a picture loaded
    CLog::Log(LOGDEBUG, "We have an error loading a picture!");
    if (m_Image[m_iCurrentPic].IsLoaded())
    { // Yes.  Let's let it transistion out, wait for it to be released, then try loading again.
      CLog::Log(LOGERROR, "Error loading the next image %s", m_vecSlides[m_iNextSlide].c_str());
      if (!m_bSlideShow)
      { // tell the pic to start transistioning out now
        m_Image[m_iCurrentPic].StartTransistion();
        m_Image[m_iCurrentPic].SetTransistionTime(1, IMMEDIATE_TRANSISTION_TIME); // only 20 frames for the transistion
      }
      m_bWaitForNextPic = true;
      m_bErrorMessage = false;
    }
    else
    { // No.  Not much we can do here.  If we're in a slideshow, we mayaswell move on to the next picture
      // change to next image
      if (m_bSlideShow)
      {
        CLog::Log(LOGERROR, "Error loading the current image %s", m_vecSlides[m_iCurrentSlide].c_str());
        m_iCurrentSlide = m_iNextSlide;
        ShowNext();
        m_bErrorMessage = false;
      }
      // else just drop through - there's nothing we can do (error message will be displayed)
    }
  }
  if (m_bErrorMessage)
  {
    RenderErrorMessage();
    return ;
  }

  if (!m_Image[m_iCurrentPic].IsLoaded() && !m_pBackgroundLoader->IsLoading())
  { // load first image
    CLog::Log(LOGDEBUG, "Loading the current image %s", m_vecSlides[m_iCurrentSlide].c_str());
    m_bWaitForNextPic = false;
    m_bLoadNextPic = false;
    // load using the background loader
    m_pBackgroundLoader->LoadPic(m_iCurrentPic, m_iCurrentSlide, m_vecSlides[m_iCurrentSlide], g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iWidth, g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iHeight);
  }

  // check if we should discard an already loaded next slide
  if (m_bLoadNextPic && m_Image[1 - m_iCurrentPic].IsLoaded() && m_Image[1 - m_iCurrentPic].SlideNumber() != m_iNextSlide)
  {
    m_Image[1 - m_iCurrentPic].Close();
  }
  // if we're reloading an image (for better res on zooming we need to close any open ones as well)
  if (m_bReloadImage && m_Image[1 - m_iCurrentPic].IsLoaded() && m_Image[1 - m_iCurrentPic].SlideNumber() != m_iCurrentSlide)
  {
    m_Image[1 - m_iCurrentPic].Close();
  }

  if (m_bReloadImage)
  {
    if (m_Image[m_iCurrentPic].IsLoaded() && !m_Image[1 - m_iCurrentPic].IsLoaded() && !m_pBackgroundLoader->IsLoading() && !m_bWaitForNextPic)
    { // reload the image if we need to
      CLog::Log(LOGDEBUG, "Reloading the current image %s", m_vecSlides[m_iCurrentSlide].c_str());
      // first, our 1:1 pixel mapping size:
      int maxWidth = (int)((float)g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iWidth * zoomamount[m_iZoomFactor - 1]);
      int maxHeight = (int)((float)g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iHeight * zoomamount[m_iZoomFactor - 1]);
      // now determine how big the image will be:
      float fWidth = (float)m_Image[m_iCurrentPic].GetOriginalWidth();
      float fHeight = (float)m_Image[m_iCurrentPic].GetOriginalHeight();
      float fAR = fWidth / fHeight;
      if (fWidth*fHeight > MAX_PICTURE_SIZE)
      { // have to scale it down - it's bigger than 16Mb
        float fScale = sqrt((float)MAX_PICTURE_SIZE / (fWidth * fHeight));
        fWidth = (float)fWidth * fScale;
        fHeight = (float)fHeight * fScale;
      }
      if (fWidth < maxWidth) maxWidth = (int)fWidth;
      if (fHeight < maxHeight) maxHeight = (int)fHeight;
      // can't have images larger than MAX_PICTURE_WIDTH (hardware constraint)
      if (maxWidth > MAX_PICTURE_WIDTH) maxWidth = MAX_PICTURE_WIDTH;
      if (maxHeight > MAX_PICTURE_HEIGHT) maxHeight = MAX_PICTURE_HEIGHT;
      m_pBackgroundLoader->LoadPic(1 - m_iCurrentPic, m_iCurrentSlide, m_vecSlides[m_iCurrentSlide], maxWidth, maxHeight);
    }
  }
  else
  {
    if ((m_bSlideShow || m_bLoadNextPic) && m_Image[m_iCurrentPic].IsLoaded() && !m_Image[1 - m_iCurrentPic].IsLoaded() && !m_pBackgroundLoader->IsLoading() && !m_bWaitForNextPic)
    { // load the next image
      CLog::Log(LOGDEBUG, "Loading the next image %s", m_vecSlides[m_iNextSlide].c_str());
      int maxWidth = (int)((float)g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iWidth * zoomamount[m_iZoomFactor - 1]);
      int maxHeight = (int)((float)g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iHeight * zoomamount[m_iZoomFactor - 1]);
      m_pBackgroundLoader->LoadPic(1 - m_iCurrentPic, m_iNextSlide, m_vecSlides[m_iNextSlide], maxWidth, maxHeight);
    }
  }

  // render the current image
  if (m_Image[m_iCurrentPic].IsLoaded())
  {
    m_Image[m_iCurrentPic].Pause(m_bPause);
    m_Image[m_iCurrentPic].Render();
  }

  // Check if we should be transistioning immediately
  if (m_bLoadNextPic)
  {
    CLog::Log(LOGDEBUG, "Starting immediate transistion due to user wanting slide %s", m_vecSlides[m_iNextSlide].c_str());
    m_Image[m_iCurrentPic].StartTransistion();
    m_Image[m_iCurrentPic].SetTransistionTime(1, IMMEDIATE_TRANSISTION_TIME); // only 20 frames for the transistion
    m_bLoadNextPic = false;
  }

  // render the next image
  if (m_Image[m_iCurrentPic].DrawNextImage())
  {
    if (m_Image[1 - m_iCurrentPic].IsLoaded())
    {
      // set the appropriate transistion time
      m_Image[1 - m_iCurrentPic].SetTransistionTime(0, m_Image[m_iCurrentPic].GetTransistionTime(1));
      m_Image[1 - m_iCurrentPic].Pause(m_bPause);
      m_Image[1 - m_iCurrentPic].Render();
    }
    else // next pic isn't loaded.  We should hang around if it is in progress
    {
      if (m_pBackgroundLoader->IsLoading())
      {
        CLog::Log(LOGDEBUG, "Having to hold the current image (%s) while we load %s", m_vecSlides[m_iCurrentSlide].c_str(), m_vecSlides[m_iNextSlide].c_str());
        m_Image[m_iCurrentPic].Keep();
      }
    }
  }

  // check if we should swap images now
  if (m_Image[m_iCurrentPic].IsFinished())
  {
    CLog::Log(LOGDEBUG, "Image %s is finished rendering, switching to %s", m_vecSlides[m_iCurrentSlide].c_str(), m_vecSlides[m_iNextSlide].c_str());
    m_Image[m_iCurrentPic].Close();
    if (m_Image[1 - m_iCurrentPic].IsLoaded())
      m_iCurrentPic = 1 - m_iCurrentPic;
    m_iCurrentSlide = m_iNextSlide;
    if (m_bSlideShow)
    {
      m_iNextSlide++;
      if (m_iNextSlide >= (int)m_vecSlides.size())
        m_iNextSlide = 0;
    }
//    m_iZoomFactor = 1;
    m_iRotate = 0;
  }

  RenderPause();

  if ( m_bShowInfo )
  {
    CStdString strSlideInfo;
    if (m_Image[m_iCurrentPic].IsLoaded())
    {
      CStdString strFileInfo;
      CStdString strFile;
      strFile = CUtil::GetFileName(m_vecSlides[m_iCurrentSlide]);
      strFileInfo.Format("%ix%i %s", m_Image[m_iCurrentPic].GetOriginalWidth(), m_Image[m_iCurrentPic].GetOriginalHeight(), strFile.c_str());
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1);
      msg.SetLabel(strFileInfo);
      OnMessage(msg);
    }
    strSlideInfo.Format("%i/%i", m_iCurrentSlide + 1 , m_vecSlides.size());
    {
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2);
      msg.SetLabel(strSlideInfo);
      OnMessage(msg);
    }
    {
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2_EXTRA);
      msg.SetLabel("");
      OnMessage(msg);
    }
  }

  RenderErrorMessage();

  if (m_bShowInfo)
  {
    SET_CONTROL_VISIBLE(BAR_IMAGE);
    SET_CONTROL_VISIBLE(LABEL_ROW1);
    SET_CONTROL_VISIBLE(LABEL_ROW2);
    SET_CONTROL_VISIBLE(LABEL_ROW2_EXTRA);
  }
  else
  {
    SET_CONTROL_HIDDEN(BAR_IMAGE);
    SET_CONTROL_HIDDEN(LABEL_ROW1);
    SET_CONTROL_HIDDEN(LABEL_ROW2);
    SET_CONTROL_HIDDEN(LABEL_ROW2_EXTRA);
  }

  CGUIWindow::Render();
}

bool CGUIWindowSlideShow::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PREVIOUS_MENU:
  case ACTION_STOP:
    m_gWindowManager.PreviousWindow();
    break;
  case ACTION_NEXT_PICTURE:
//    if (m_iZoomFactor == 1)
      ShowNext();
    break;
  case ACTION_PREV_PICTURE:
//    if (m_iZoomFactor == 1)
      ShowPrevious();
    break;
  case ACTION_MOVE_RIGHT:
    if (m_iZoomFactor == 1)
      ShowNext();
    else
      Move(PICTURE_MOVE_AMOUNT, 0);
    break;

  case ACTION_MOVE_LEFT:
    if (m_iZoomFactor == 1)
      ShowPrevious();
    else
      Move( -PICTURE_MOVE_AMOUNT, 0);
    break;

  case ACTION_MOVE_DOWN:
    Move(0, PICTURE_MOVE_AMOUNT);
    break;

  case ACTION_MOVE_UP:
    Move(0, -PICTURE_MOVE_AMOUNT);
    break;

  case ACTION_SHOW_INFO:
    m_bShowInfo = !m_bShowInfo;
    break;

  case ACTION_PAUSE:
    if (m_bSlideShow)
      m_bPause = !m_bPause;
    break;

  case ACTION_ZOOM_OUT:
    Zoom(m_iZoomFactor - 1);
    break;

  case ACTION_ZOOM_IN:
    Zoom(m_iZoomFactor + 1);
    break;

  case ACTION_ROTATE_PICTURE:
    Rotate();
    break;

  case ACTION_ZOOM_LEVEL_NORMAL:
  case ACTION_ZOOM_LEVEL_1:
  case ACTION_ZOOM_LEVEL_2:
  case ACTION_ZOOM_LEVEL_3:
  case ACTION_ZOOM_LEVEL_4:
  case ACTION_ZOOM_LEVEL_5:
  case ACTION_ZOOM_LEVEL_6:
  case ACTION_ZOOM_LEVEL_7:
  case ACTION_ZOOM_LEVEL_8:
  case ACTION_ZOOM_LEVEL_9:
    Zoom((action.wID - ACTION_ZOOM_LEVEL_NORMAL) + 1);
    break;
  case ACTION_ANALOG_MOVE:
    Move(action.fAmount1*PICTURE_MOVE_AMOUNT_ANALOG, -action.fAmount2*PICTURE_MOVE_AMOUNT_ANALOG);
    break;
  default:
    return CGUIWindow::OnAction(action);
  }
  return true;
}

void CGUIWindowSlideShow::RenderErrorMessage()
{
  if (!m_bErrorMessage)
    return ;
  CGUIFont *pFont = g_fontManager.GetFont(((CGUILabelControl *)GetControl(LABEL_ROW1))->GetFontName());
  if (pFont)
  {
    wstring wszText = g_localizeStrings.Get(747);
    pFont->DrawText((float)g_graphicsContext.GetWidth() / 2, (float)g_graphicsContext.GetHeight() / 2, 0xffffffff, wszText.c_str(), XBFONT_CENTER_X | XBFONT_CENTER_Y);
  }
}

bool CGUIWindowSlideShow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      //   Reset();
      if (message.GetParam1() != WINDOW_PICTURES)
      {
        CSectionLoader::UnloadDLL(IMAGE_DLL);
      }
      g_graphicsContext.Lock();
      g_graphicsContext.SetOverlay(true);
      g_graphicsContext.Get3DDevice()->EnableOverlay(FALSE);
      // reset to gui mode so that we use it's filters
      g_graphicsContext.SetFullScreenVideo(false);
      g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution, TRUE);
      g_graphicsContext.Unlock();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      if (g_application.IsPlayingVideo())
        g_application.StopPlaying();
      // clear as much memory as possible
      g_TextureManager.Flush();
      if (message.GetParam1() != WINDOW_PICTURES)
      {
        CSectionLoader::LoadDLL(IMAGE_DLL);
      }
      g_graphicsContext.Lock();
      g_graphicsContext.SetOverlay(false);
      g_graphicsContext.Get3DDevice()->EnableOverlay(TRUE);
      // set to video mode so that we use it's filters (to make pics sharper)
      g_graphicsContext.SetFullScreenVideo(true);
      g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution, FALSE);
      g_graphicsContext.Unlock();

      // shuffle
      if (g_guiSettings.GetBool("Slideshow.Shuffle") && m_bSlideShow)
        Shuffle();

      // turn off slideshow if we only have 1 image
      if (m_vecSlides.size() <= 1)
        m_bSlideShow = false;
      return true;
    }
    break;
  case GUI_MSG_START_SLIDESHOW:
    {
      CStdString strFolder = message.GetStringParam();
      bool bRecursive = message.GetParam1() != 0;
        RunSlideShow(strFolder, bRecursive);
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSlideShow::RenderPause()
{ // display the pause icon
  if (m_bPause)
  {
    SET_CONTROL_VISIBLE(CONTROL_PAUSE);
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_PAUSE);
  }
  /*
   static DWORD dwCounter=0;
   dwCounter++;
   if (dwCounter > 25)
   {
    dwCounter=0;
   }
   if (!m_bPause) return;
   if (dwCounter <13) return;*/

}

void CGUIWindowSlideShow::Rotate()
{
  if (!m_Image[m_iCurrentPic].DrawNextImage() && m_iZoomFactor == 1)
  {
    m_Image[m_iCurrentPic].Rotate(++m_iRotate);
  }
}

void CGUIWindowSlideShow::Zoom(int iZoom)
{
  if (iZoom > MAX_ZOOM_FACTOR || iZoom < 1)
    return ;
  // set the zoom amount and then set so that the image is reloaded at the higher (or lower)
  // resolution as necessary
  if (!m_Image[m_iCurrentPic].DrawNextImage())
  {
    m_Image[m_iCurrentPic].Zoom(iZoom);
    // check if we need to reload the image for better resolution
    if (iZoom > m_iZoomFactor && !m_Image[m_iCurrentPic].FullSize())
      m_bReloadImage = true;
    if (iZoom == 1)
      m_bReloadImage = true;
    m_iZoomFactor = iZoom;
  }
}

void CGUIWindowSlideShow::Move(float fX, float fY)
{
  if (m_Image[m_iCurrentPic].IsLoaded() && m_Image[m_iCurrentPic].GetZoom() > 1)
  { // we move in the opposite direction, due to the fact we are moving
    // the viewing window, not the picture.
    m_Image[m_iCurrentPic].Move( -fX, -fY);
  }
}

void CGUIWindowSlideShow::OnLoadPic(int iPic, int iSlideNumber, D3DTexture *pTexture, int iWidth, int iHeight, int iOriginalWidth, int iOriginalHeight, int iRotate, bool bFullSize)
{
  if (pTexture)
  {
    // set the pic's texture + size etc.
    CLog::Log(LOGDEBUG, "Finished background loading %s", m_vecSlides[iSlideNumber].c_str());
    if (m_bReloadImage)
    {
      m_Image[m_iCurrentPic].UpdateTexture(pTexture, iWidth, iHeight);
      m_Image[m_iCurrentPic].SetOriginalSize(iOriginalWidth, iOriginalHeight, bFullSize);
      m_bReloadImage = false;
    }
    else
    {
      if (m_bSlideShow)
        m_Image[iPic].SetTexture(iSlideNumber, pTexture, iWidth, iHeight, iRotate);
      else
        m_Image[iPic].SetTexture(iSlideNumber, pTexture, iWidth, iHeight, iRotate, EFFECT_NO_TIMEOUT);
      m_Image[iPic].SetOriginalSize(iOriginalWidth, iOriginalHeight, bFullSize);
      m_Image[iPic].Zoom(m_iZoomFactor, true);
    }
  }
  else
  { // Failed to load image.  What should be done??
    // We should wait for the current pic to finish rendering, then transistion it out,
    // release the texture, and try and reload this pic from scratch
    m_bErrorMessage = true;
  }
}

void CGUIWindowSlideShow::Shuffle()
{
  srand( timeGetTime() );
  int iNumToShuffle = (int)m_vecSlides.size();
  // first pic is the current pic
  vector<CStdString> vecTemp(m_vecSlides);
  m_vecSlides.clear();
  int iDone = 0;
  ivecSlides it = m_vecSlides.begin();
  m_vecSlides.push_back(vecTemp[m_iCurrentSlide]);
  vecTemp.erase(vecTemp.begin() + m_iCurrentSlide);
  iNumToShuffle--;
  while (iNumToShuffle)
  {
    int slide = rand() % iNumToShuffle;
    m_vecSlides.push_back(vecTemp[slide]);
    vecTemp.erase(vecTemp.begin() + slide);
    iNumToShuffle--;
  }
  m_iCurrentSlide = 0;
  m_iNextSlide = 1;
}

int CGUIWindowSlideShow::NumSlides()
{
  return (int)m_vecSlides.size();
}

void CGUIWindowSlideShow::RunSlideShow(const CStdString &strPath, bool bRecursive)
{
  // stop any video
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();
  if (strPath!="")
  {
    // reset the slideshow
    Reset();
    AddItems(strPath, bRecursive);
    // ok, now run the slideshow
  }
  StartSlideShow();
  if (NumSlides())
    m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowSlideShow::AddItems(const CStdString &strPath, bool bRecursive)
{
  // read the directory in
  IDirectory *pDir = CFactoryDirectory::Create(strPath);
  if (!pDir) return;
  CFileItemList items;
  pDir->SetMask(g_stSettings.m_szMyPicturesExtensions);
  bool bResult = pDir->GetDirectory(strPath, items);
  delete pDir;
  if (!bResult) return;
  // now sort it as necessary
  CUtil::SortFileItemsByName(items);
  // need to go into all subdirs
  for (int i = 0; i < items.Size(); i++)
  {
    if (items[i]->m_bIsFolder && bRecursive)
    {
      AddItems(items[i]->m_strPath, bRecursive);
    }
    else
    { // add to the slideshow
      Add(items[i]->m_strPath);
    }
  }
}