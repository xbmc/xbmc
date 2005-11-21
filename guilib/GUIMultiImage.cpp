#include "include.h"
#include "../xbmc/FileItem.h"
#include "GUIMultiImage.h"
#include "TextureManager.h"
#include "../xbmc/FileSystem/HDDirectory.h"

CGUIMultiImage::CGUIMultiImage(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTexturePath, DWORD timePerImage, DWORD fadeTime)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
{
  m_texturePath = strTexturePath;
  m_currentImage = 0;
  m_timePerImage = timePerImage;
  m_fadeTime = fadeTime;
  m_keepAspectRatio = false;
  ControlType = GUICONTROL_MULTI_IMAGE;
  m_bDynamicResourceAlloc=false;
}

CGUIMultiImage::~CGUIMultiImage(void)
{
}

void CGUIMultiImage::Render()
{
  // check for conditional visibility
  bool bVisible = UpdateEffectState();

  if (m_bDynamicResourceAlloc && !bVisible && IsAllocated())
    FreeResources();

  if (!bVisible)
    return;

  if (m_bDynamicResourceAlloc && bVisible && !IsAllocated())
    AllocResources();
  else if (!m_bDynamicResourceAlloc && !IsAllocated())
    AllocResources();  // not dynamic, make sure we allocate!

  if (m_images.empty())
    return ;

  m_images[m_currentImage]->Render();

  if (m_images.size() <= 1)
    return; // no need for complicated stuff if we only have 1 picture!

  unsigned int nextImage = (m_currentImage + 1) % m_images.size();

  // check if we should be loading a new image yet
  if (m_imageTimer.IsRunning() && m_imageTimer.GetElapsedMilliseconds() > m_timePerImage)
  {
    m_imageTimer.Stop();
    // grab a new image
    // nextImage is the one we want.  Note that it will be loaded automagically
    // at time to Render (using GUIImage's auto-load functions)
    // start the fade timer
    m_fadeTimer.StartZero();
  }

  // check if we are still fading
  if (m_fadeTimer.IsRunning())
  {
    // check if the fade timer has run out
    float timeFading = m_fadeTimer.GetElapsedMilliseconds();
    if (timeFading > m_fadeTime)
    {
      m_fadeTimer.Stop();
      // swap images
      m_images[m_currentImage]->FreeResources();
      m_images[nextImage]->SetAlpha(255);
      m_currentImage = nextImage;
      // start the load timer
      m_imageTimer.StartZero();
    }
    else
    { // perform the fade
      float fadeAmount = timeFading / m_fadeTime;
      m_images[nextImage]->SetAlpha((DWORD)(255 * fadeAmount));
    }
    m_images[nextImage]->Render();
  }

  CGUIControl::Render();
}

bool CGUIMultiImage::OnAction(const CAction &action)
{
  return false;
}

void CGUIMultiImage::PreAllocResources()
{
  FreeResources();
}

void CGUIMultiImage::AllocResources()
{
  FreeResources();
  CGUIControl::AllocResources();

  // Load any images from our texture bundle first
  CStdStringArray images;
  g_TextureManager.GetBundledTexturesFromPath(m_texturePath, images);

  // Load in our images from the directory specified
  // m_texturePath is relative (as are all skin paths)
  CStdString realPath = g_TextureManager.GetTexturePath(m_texturePath);
  CHDDirectory dir;
  CFileItemList items;
  dir.GetDirectory(realPath, items);
  for (int i=0; i < items.Size(); i++)
  {
    CFileItem *pItem = items[i];
    if (pItem->IsPicture())
      images.push_back(pItem->m_strPath);
  }

  // and sort them
  sort(images.begin(), images.end());

  for (unsigned int i=0; i < images.size(); i++)
  {
    CGUIImage *pImage = new CGUIImage(GetParentID(), GetID(), m_iPosX, m_iPosY, m_dwWidth, m_dwHeight, images[i]);
    if (pImage)
      m_images.push_back(pImage);
  }
  // Load in the current image, and reset our timer
  m_imageTimer.StartZero();
  m_fadeTimer.Stop();
  m_currentImage = 0;
  if (m_images.empty())
    return;
  m_images[m_currentImage]->AllocResources();

  // Scale image so that it will fill our render area
  if (m_keepAspectRatio)
  {
    // image is scaled so that the aspect ratio is maintained (taking into account the TV pixel ratio)
    // and so that it fills the allocated space (so is zoomed then cropped)
    float sourceAspectRatio = (float)m_images[m_currentImage]->GetTextureWidth() / m_images[m_currentImage]->GetTextureHeight();
    float aspectRatio = sourceAspectRatio / g_graphicsContext.GetPixelRatio(g_graphicsContext.GetVideoResolution());

    unsigned int newWidth = m_dwWidth;
    unsigned int newHeight = (unsigned int)((float)newWidth * aspectRatio);
    if (newHeight < m_dwHeight)
    {
      newHeight = m_dwHeight;
      newWidth = (unsigned int)((float)newHeight / aspectRatio);
    }
    m_images[m_currentImage]->SetPosition(m_iPosX - (int)(newWidth - m_dwWidth)/2, m_iPosY - (int)(newHeight - m_dwHeight)/2);
    m_images[m_currentImage]->SetWidth(newWidth);
    m_images[m_currentImage]->SetHeight(newHeight);
  }
}

void CGUIMultiImage::FreeResources()
{
  for (unsigned int i = 0; i < m_images.size(); ++i)
  {
    m_images[i]->FreeResources();
    delete m_images[i];
  }

  m_images.clear();
  m_currentImage = 0;
  CGUIControl::FreeResources();
}

void CGUIMultiImage::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_bDynamicResourceAlloc=bOnOff;
}

bool CGUIMultiImage::CanFocus() const
{
  return false;
}

void CGUIMultiImage::SetKeepAspectRatio(bool bOnOff)
{
  if (m_keepAspectRatio != bOnOff)
  {
    m_keepAspectRatio = bOnOff;
    m_bInvalidated = true;
  }
}

bool CGUIMultiImage::GetKeepAspectRatio() const
{
  return m_keepAspectRatio;
}

