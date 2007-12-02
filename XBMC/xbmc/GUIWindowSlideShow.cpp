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
#include "GUIWindowSlideShow.h"
#include "Application.h"
#include "Picture.h"
#include "Util.h"
#include "TextureManager.h"
#include "GUILabelControl.h"
#include "utils/GUIInfoManager.h"
#include "FileSystem/FactoryDirectory.h"
#include "GUIDialogPictureInfo.h"

using namespace DIRECTORY;

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
  m_loadPic = CreateEvent(NULL,false,false,NULL);
  m_isLoading = false;
}

CBackgroundPicLoader::~CBackgroundPicLoader()
{}

void CBackgroundPicLoader::Create(CGUIWindowSlideShow *pCallback)
{
  m_pCallback = pCallback;
  m_isLoading = false;
  CThread::Create(false);
}

void CBackgroundPicLoader::Process()
{
  DWORD totalTime = 0;
  DWORD count = 0;
  while (!m_bStop)
  { // loop around forever, waiting for the app to call LoadPic
    if (WaitForSingleObject(m_loadPic, 10) == WAIT_OBJECT_0)
    {
      if (m_pCallback)
      {
        CPicture pic;
        DWORD start = timeGetTime();
        IDirect3DTexture8 *pTexture = pic.Load(m_strFileName, m_maxWidth, m_maxHeight);
        totalTime += timeGetTime() - start;
        count++;
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
        m_pCallback->OnLoadPic(m_iPic, m_iSlideNumber, pTexture, pic.GetWidth(), pic.GetHeight(), pic.GetOriginalWidth(), pic.GetOriginalHeight(), pic.GetExifInfo()->Orientation, bFullSize);
        m_isLoading = false;
      }
    }
  }
  CLog::Log(LOGDEBUG, "Time for loading %i images: %i ms, average %i ms", count, totalTime, totalTime / count);
}

void CBackgroundPicLoader::LoadPic(int iPic, int iSlideNumber, const CStdString &strFileName, const int maxWidth, const int maxHeight)
{
  m_iPic = iPic;
  m_iSlideNumber = iSlideNumber;
  m_strFileName = strFileName;
  m_maxWidth = maxWidth;
  m_maxHeight = maxHeight;
  m_isLoading = true;
  SetEvent(m_loadPic);
}

CGUIWindowSlideShow::CGUIWindowSlideShow(void)
    : CGUIWindow(WINDOW_SLIDESHOW, "Slideshow.xml")
{
  m_pBackgroundLoader = NULL;
  Reset();
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
  g_infoManager.SetShowCodec(false);
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
  m_slides.Clear();
  m_Resolution = INVALID;
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
  g_infoManager.ResetCurrentSlide();
}

void CGUIWindowSlideShow::Add(const CFileItem *picture)
{
  m_slides.Add(new CFileItem(*picture));
}

void CGUIWindowSlideShow::ShowNext()
{
  if (m_slides.Size() == 1)
    return;

  m_iNextSlide = m_iCurrentSlide + 1;
  if (m_iNextSlide >= m_slides.Size())
    m_iNextSlide = 0;

  m_bLoadNextPic = true;
}

void CGUIWindowSlideShow::ShowPrevious()
{
  if (m_slides.Size() == 1)
    return;

  m_iNextSlide = m_iCurrentSlide - 1;
  if (m_iNextSlide < 0)
    m_iNextSlide = m_slides.Size() - 1;
  m_bLoadNextPic = true;
}


void CGUIWindowSlideShow::Select(const CStdString& strPicture)
{
  for (int i = 0; i < m_slides.Size(); ++i)
  {
    const CFileItem *item = m_slides[i];
    if (item->m_strPath == strPicture)
    {
      m_iCurrentSlide = i;
      m_iNextSlide = m_iCurrentSlide + 1;
      if (m_iNextSlide >= m_slides.Size())
        m_iNextSlide = 0;
      return ;
    }
  }
}

const CFileItemList &CGUIWindowSlideShow::GetSlideShowContents()
{
  return m_slides;
}

const CFileItem *CGUIWindowSlideShow::GetCurrentSlide()
{
  if (m_iCurrentSlide >= 0 && m_iCurrentSlide < m_slides.Size())
    return m_slides[m_iCurrentSlide];
  return NULL;
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
  int iSlides = m_slides.Size();
  if (!iSlides) return ;

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

  bool bSlideShow = m_bSlideShow && !m_bPause;

  if (m_bErrorMessage)
  { // we have an error when loading either the current or next picture
    // check to see if we have a picture loaded
    CLog::Log(LOGDEBUG, "We have an error loading a picture!");
    if (m_Image[m_iCurrentPic].IsLoaded())
    { // Yes.  Let's let it transistion out, wait for it to be released, then try loading again.
      CLog::Log(LOGERROR, "Error loading the next image %s", m_slides[m_iNextSlide]->m_strPath.c_str());
      if (!bSlideShow)
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
      if (bSlideShow)
      {
        CLog::Log(LOGERROR, "Error loading the current image %s", m_slides[m_iCurrentSlide]->m_strPath.c_str());
        m_iCurrentSlide = m_iNextSlide;
        ShowNext();
        m_bErrorMessage = false;
      }
      else if (m_bLoadNextPic)
      {
        m_iCurrentSlide = m_iNextSlide;
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
    CLog::Log(LOGDEBUG, "Loading the current image %s", m_slides[m_iCurrentSlide]->m_strPath.c_str());
    m_bWaitForNextPic = false;
    m_bLoadNextPic = false;
    // load using the background loader
    int maxWidth, maxHeight;
    GetCheckedSize((float)g_settings.m_ResInfo[m_Resolution].iWidth * zoomamount[m_iZoomFactor - 1], 
                    (float)g_settings.m_ResInfo[m_Resolution].iHeight * zoomamount[m_iZoomFactor - 1],
                    maxWidth, maxHeight);
    m_pBackgroundLoader->LoadPic(m_iCurrentPic, m_iCurrentSlide, m_slides[m_iCurrentSlide]->m_strPath, maxWidth, maxHeight);
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
      CLog::Log(LOGDEBUG, "Reloading the current image %s at zoom level %i", m_slides[m_iCurrentSlide]->m_strPath.c_str(), m_iZoomFactor);
      // first, our maximal size for this zoom level
      int maxWidth = (int)((float)g_settings.m_ResInfo[m_Resolution].iWidth * zoomamount[m_iZoomFactor - 1]);
      int maxHeight = (int)((float)g_settings.m_ResInfo[m_Resolution].iWidth * zoomamount[m_iZoomFactor - 1]);

      // the actual maximal size of the image to optimize the sizing based on the known sizing (aspect ratio)
      int width, height;
      GetCheckedSize((float)m_Image[m_iCurrentPic].GetOriginalWidth(), (float)m_Image[m_iCurrentPic].GetOriginalHeight(), width, height);

      // use the smaller of the two (no point zooming in more than we have to)
      if (maxWidth < width) width = maxWidth;
      if (maxHeight < height) height = maxHeight;

      m_pBackgroundLoader->LoadPic(m_iCurrentPic, m_iCurrentSlide, m_slides[m_iCurrentSlide]->m_strPath, maxWidth, maxHeight);
    }
  }
  else
  {
    if ((bSlideShow || m_bLoadNextPic) && m_Image[m_iCurrentPic].IsLoaded() && !m_Image[1 - m_iCurrentPic].IsLoaded() && !m_pBackgroundLoader->IsLoading() && !m_bWaitForNextPic)
    { // load the next image
      CLog::Log(LOGDEBUG, "Loading the next image %s", m_slides[m_iNextSlide]->m_strPath.c_str());
      int maxWidth, maxHeight;
      GetCheckedSize((float)g_settings.m_ResInfo[m_Resolution].iWidth * zoomamount[m_iZoomFactor - 1], 
                     (float)g_settings.m_ResInfo[m_Resolution].iHeight * zoomamount[m_iZoomFactor - 1],
                     maxWidth, maxHeight);
      m_pBackgroundLoader->LoadPic(1 - m_iCurrentPic, m_iNextSlide, m_slides[m_iNextSlide]->m_strPath, maxWidth, maxHeight);
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
    CLog::Log(LOGDEBUG, "Starting immediate transistion due to user wanting slide %s", m_slides[m_iNextSlide]->m_strPath.c_str());
    if (m_Image[m_iCurrentPic].StartTransistion())
    {
      m_Image[m_iCurrentPic].SetTransistionTime(1, IMMEDIATE_TRANSISTION_TIME); // only 20 frames for the transistion
      m_bLoadNextPic = false;
    }
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
//        CLog::Log(LOGDEBUG, "Having to hold the current image (%s) while we load %s", m_vecSlides[m_iCurrentSlide].c_str(), m_vecSlides[m_iNextSlide].c_str());
        m_Image[m_iCurrentPic].Keep();
      }
    }
  }

  // check if we should swap images now
  if (m_Image[m_iCurrentPic].IsFinished())
  {
    CLog::Log(LOGDEBUG, "Image %s is finished rendering, switching to %s", m_slides[m_iCurrentSlide]->m_strPath.c_str(), m_slides[m_iNextSlide]->m_strPath.c_str());
    m_Image[m_iCurrentPic].Close();
    if (m_Image[1 - m_iCurrentPic].IsLoaded())
      m_iCurrentPic = 1 - m_iCurrentPic;
    m_iCurrentSlide = m_iNextSlide;
    if (bSlideShow)
    {
      m_iNextSlide++;
      if (m_iNextSlide >= m_slides.Size())
        m_iNextSlide = 0;
    }
//    m_iZoomFactor = 1;
    m_iRotate = 0;
  }

  RenderPause();

  if (m_Image[m_iCurrentPic].IsLoaded())
    g_infoManager.SetCurrentSlide(*m_slides[m_iCurrentSlide]);

  RenderErrorMessage();

  CGUIWindow::Render();
}

bool CGUIWindowSlideShow::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_SHOW_CODEC:
    {
      CGUIDialogPictureInfo *pictureInfo = (CGUIDialogPictureInfo *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PICTURE_INFO);
      if (pictureInfo)
      {
        // no need to set the picture here, it's done in Render()
        pictureInfo->DoModal();
      }
    }
    break;
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
  CGUIFont *pFont = ((CGUILabelControl *)GetControl(LABEL_ROW1))->GetLabelInfo().font;
  CGUITextLayout::DrawText(pFont, 0.5f*g_graphicsContext.GetWidth(), 0.5f*g_graphicsContext.GetHeight(), 0xffffffff, 0, g_localizeStrings.Get(747), XBFONT_CENTER_X | XBFONT_CENTER_Y);
}

bool CGUIWindowSlideShow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_Resolution != g_guiSettings.m_LookAndFeelResolution)
      {
        g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution, TRUE);
      }

      //   Reset();
      if (message.GetParam1() != WINDOW_PICTURES)
      {
        m_ImageLib.Unload();
      }
      m_gWindowManager.ShowOverlay(OVERLAY_STATE_SHOWN);
      // set screen filters to video filters so that we
      // get sharper images
      g_graphicsContext.SetScreenFilters(false);
      FreeResources();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_Resolution = (RESOLUTION) g_guiSettings.GetInt("pictures.displayresolution");
      if (m_Resolution != g_guiSettings.m_LookAndFeelResolution && m_Resolution != INVALID)
      {
        g_graphicsContext.SetVideoResolution(m_Resolution, TRUE);
      }

      CGUIWindow::OnMessage(message);
      if (g_application.IsPlayingVideo())
        g_application.StopPlaying();
      // clear as much memory as possible
      g_TextureManager.Flush();
      if (message.GetParam1() != WINDOW_PICTURES)
      {
        m_ImageLib.Load();
      }
      m_gWindowManager.ShowOverlay(OVERLAY_STATE_HIDDEN);
      // set screen filters to video filters so that we
      // get sharper images
      g_graphicsContext.SetScreenFilters(true);

      // turn off slideshow if we only have 1 image
      if (m_slides.Size() <= 1)
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

void CGUIWindowSlideShow::OnLoadPic(int iPic, int iSlideNumber, LPDIRECT3DTEXTURE8 pTexture, int iWidth, int iHeight, int iOriginalWidth, int iOriginalHeight, int iRotate, bool bFullSize)
{
  if (!g_guiSettings.GetBool("pictures.useexifrotation"))
    iRotate = 1;
  if (pTexture)
  {
    // set the pic's texture + size etc.
    CLog::Log(LOGDEBUG, "Finished background loading %s", m_slides[iSlideNumber]->m_strPath.c_str());
    if (m_bReloadImage)
    {
      if (m_Image[m_iCurrentPic].IsLoaded() && m_Image[m_iCurrentPic].SlideNumber() != iSlideNumber)
      { // wrong image (ie we finished loading the next image, not the current image)
        pTexture->Release();
        return;
      }
      m_Image[m_iCurrentPic].UpdateTexture(pTexture, iWidth, iHeight);
      m_Image[m_iCurrentPic].SetOriginalSize(iOriginalWidth, iOriginalHeight, bFullSize);
      m_bReloadImage = false;
    }
    else
    {
      if (m_bSlideShow)
        m_Image[iPic].SetTexture(iSlideNumber, pTexture, iWidth, iHeight, iRotate, g_guiSettings.GetBool("slideshow.displayeffects") ? CSlideShowPic::EFFECT_RANDOM : CSlideShowPic::EFFECT_NONE);
      else
        m_Image[iPic].SetTexture(iSlideNumber, pTexture, iWidth, iHeight, iRotate, CSlideShowPic::EFFECT_NO_TIMEOUT);
      m_Image[iPic].SetOriginalSize(iOriginalWidth, iOriginalHeight, bFullSize);
      m_Image[iPic].Zoom(m_iZoomFactor, true);

      m_Image[iPic].m_bIsComic = false;
      if (CUtil::IsInRAR(m_slides[m_iCurrentSlide]->m_strPath) || CUtil::IsInZIP(m_slides[m_iCurrentSlide]->m_strPath)) // move to top for cbr/cbz
      {
        CURL url(m_slides[m_iCurrentSlide]->m_strPath);
        CStdString strHostName = url.GetHostName();
        if (CUtil::GetExtension(strHostName).Equals(".cbr", false) || CUtil::GetExtension(strHostName).Equals(".cbz", false))
        {
          m_Image[iPic].m_bIsComic = true;
          m_Image[iPic].Move((float)m_Image[iPic].GetOriginalWidth(),(float)m_Image[iPic].GetOriginalHeight());
        }
      }
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
  m_slides.Randomize();
  m_iCurrentSlide = 0;
  m_iNextSlide = 1;
}

int CGUIWindowSlideShow::NumSlides() const
{
  return m_slides.Size();
}

int CGUIWindowSlideShow::CurrentSlide() const
{
  return m_iCurrentSlide + 1;
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
  if (g_guiSettings.GetBool("slideshow.shuffle"))
    Shuffle();

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
  pDir->SetMask(g_stSettings.m_pictureExtensions);
  bool bResult = pDir->GetDirectory(strPath, items);
  delete pDir;
  if (!bResult) return;
  // now sort it as necessary
  items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  // need to go into all subdirs
  for (int i = 0; i < items.Size(); i++)
  {
    if (items[i]->m_bIsFolder && bRecursive)
    {
      AddItems(items[i]->m_strPath, bRecursive);
    }
    else
    { // add to the slideshow
      Add(items[i]);
    }
  }
}

void CGUIWindowSlideShow::GetCheckedSize(float width, float height, int &maxWidth, int &maxHeight)
{
  if (width * height > MAX_PICTURE_SIZE)
  {
    float fScale = sqrt((float)MAX_PICTURE_SIZE / (width * height));
    width = fScale * width;
    height = fScale * height;
  }
  maxWidth = (int)width;
  maxHeight = (int)height;
  if (maxWidth > MAX_PICTURE_WIDTH) maxWidth = MAX_PICTURE_WIDTH;
  if (maxHeight > MAX_PICTURE_HEIGHT) maxHeight = MAX_PICTURE_HEIGHT;
}
