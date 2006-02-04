
#include "stdafx.h"
#include "GUIWindowSlideShow.h"
#include "Application.h"
#include "Picture.h"
#include "Util.h"
#include "TextureManager.h"
#include "GUILabelControl.h"
#include "utils/GUIInfoManager.h"

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
        D3DTexture *pTexture = pic.Load(m_strFileName, m_maxWidth, m_maxHeight);
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

CGUIWindowSlideShow::CGUIWindowSlideShow(void)
    : CGUIWindow(WINDOW_SLIDESHOW, "Slideshow.xml")
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
  g_infoManager.SetShowInfo(false);
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
  if (m_vecSlides.size() == 1)
    return;
  
  m_iNextSlide = m_iCurrentSlide + 1;
  if (m_iNextSlide >= (int)m_vecSlides.size())
    m_iNextSlide = 0;
  m_bLoadNextPic = true;
}

void CGUIWindowSlideShow::ShowPrevious()
{
  if (m_vecSlides.size() == 1)
    return;

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
      CLog::Log(LOGERROR, "Error loading the next image %s", m_vecSlides[m_iNextSlide].c_str());
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
    if ((bSlideShow || m_bLoadNextPic) && m_Image[m_iCurrentPic].IsLoaded() && !m_Image[1 - m_iCurrentPic].IsLoaded() && !m_pBackgroundLoader->IsLoading() && !m_bWaitForNextPic)
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
    CLog::Log(LOGDEBUG, "Image %s is finished rendering, switching to %s", m_vecSlides[m_iCurrentSlide].c_str(), m_vecSlides[m_iNextSlide].c_str());
    m_Image[m_iCurrentPic].Close();
    if (m_Image[1 - m_iCurrentPic].IsLoaded())
      m_iCurrentPic = 1 - m_iCurrentPic;
    m_iCurrentSlide = m_iNextSlide;
    if (bSlideShow)
    {
      m_iNextSlide++;
      if (m_iNextSlide >= (int)m_vecSlides.size())
        m_iNextSlide = 0;
    }
//    m_iZoomFactor = 1;
    m_iRotate = 0;
  }

  RenderPause();

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

  RenderErrorMessage();

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
  if (pFont)
  {
    wstring wszText = g_localizeStrings.Get(747);
    pFont->DrawText((float)g_graphicsContext.GetWidth() / 2, (float)g_graphicsContext.GetHeight() / 2, 0xffffffff, 0, wszText.c_str(), XBFONT_CENTER_X | XBFONT_CENTER_Y);
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
        m_ImageLib.Unload();
      }
      g_graphicsContext.Lock();
      m_gWindowManager.ShowOverlay(true);
      g_graphicsContext.Get3DDevice()->EnableOverlay(FALSE);
      // reset to gui mode so that we use it's filters
      g_graphicsContext.SetFullScreenVideo(false);
      g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution, TRUE);
      g_graphicsContext.Unlock();
      FreeResources();
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
        m_ImageLib.Load();
      }
      g_graphicsContext.Lock();
      m_gWindowManager.ShowOverlay(false);
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
        m_Image[iPic].SetTexture(iSlideNumber, pTexture, iWidth, iHeight, iRotate, g_guiSettings.GetBool("Slideshow.DisplayEffects") ? CSlideShowPic::EFFECT_RANDOM : CSlideShowPic::EFFECT_NONE);
      else
        m_Image[iPic].SetTexture(iSlideNumber, pTexture, iWidth, iHeight, iRotate, CSlideShowPic::EFFECT_NO_TIMEOUT);
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
  random_shuffle(m_vecSlides.begin(), m_vecSlides.end());
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
      Add(items[i]->m_strPath);
    }
  }
}