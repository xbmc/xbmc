#include "include.h"
#include "../xbmc/FileItem.h"
#include "GUIMultiImage.h"
#include "TextureManager.h"
#include "../xbmc/FileSystem/HDDirectory.h"
#include "../xbmc/utils/GUIInfoManager.h"

CGUIMultiImage::CGUIMultiImage(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTexturePath, DWORD timePerImage, DWORD fadeTime, bool randomized, bool loop)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
{
  m_texturePath = strTexturePath;
  m_currentImage = 0;
  m_timePerImage = timePerImage;
  m_fadeTime = fadeTime;
  m_randomized = randomized;
  m_loop = loop;
  m_aspectRatio = CGUIImage::ASPECT_RATIO_STRETCH;
  ControlType = GUICONTROL_MULTI_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_Info = 0;
  m_directoryLoaded = false;
}

CGUIMultiImage::~CGUIMultiImage(void)
{
}

void CGUIMultiImage::Render()
{
  if (!IsVisible())
  {
    if (m_bDynamicResourceAlloc && IsAllocated())
      FreeResources();
    return;
  }

  // check for conditional information before we
  // alloc as this can free our resources
  if (m_Info)
  {
    CStdString texturePath = g_infoManager.GetImage(m_Info);
    if (texturePath != m_texturePath)
    {
      m_texturePath = texturePath;
      FreeResources();
      LoadDirectory();
    }
  }

  if (m_bDynamicResourceAlloc && !IsAllocated())
    AllocResources();
  else if (!m_bDynamicResourceAlloc && !IsAllocated())
    AllocResources();  // not dynamic, make sure we allocate!

  if (!m_images.empty())
  {
    // Set a viewport so that we don't render outside the defined area
    g_graphicsContext.SetViewPort((float)m_iPosX, (float)m_iPosY, (float)m_dwWidth, (float)m_dwHeight);
    m_images[m_currentImage]->Render();

    unsigned int nextImage = m_currentImage + 1;
    if (nextImage >= m_images.size())
      nextImage = m_loop ? 0 : m_currentImage;  // stay on the last image if <loop>no</loop>

    if (nextImage != m_currentImage)
    {
      // check if we should be loading a new image yet
      if (m_imageTimer.IsRunning() && m_imageTimer.GetElapsedMilliseconds() > m_timePerImage)
      {
        m_imageTimer.Stop();
        // grab a new image
        LoadImage(nextImage);
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
    }
    g_graphicsContext.RestoreViewPort();
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

  if (!m_directoryLoaded)
    LoadDirectory();

  // Randomize or sort our images if necessary
  if (m_randomized)
    random_shuffle(m_files.begin(), m_files.end());

  for (unsigned int i=0; i < m_files.size(); i++)
  {
    CGUIImage *pImage = new CGUIImage(GetParentID(), GetID(), m_iPosX, m_iPosY, m_dwWidth, m_dwHeight, m_files[i]);
    if (pImage)
      m_images.push_back(pImage);
  }
  // Load in the current image, and reset our timer
  m_imageTimer.StartZero();
  m_fadeTimer.Stop();
  m_currentImage = 0;
  if (m_images.empty())
    return;

  LoadImage(m_currentImage);
}

void CGUIMultiImage::LoadImage(int image)
{
  if (image < 0 || image >= (int)m_images.size())
    return;

  m_images[image]->AllocResources();

  // Scale image so that it will fill our render area
  if (m_aspectRatio != CGUIImage::ASPECT_RATIO_STRETCH)
  {
    // image is scaled so that the aspect ratio is maintained (taking into account the TV pixel ratio)
    // and so that it fills the allocated space (so is zoomed then cropped)
    float sourceAspectRatio = (float)m_images[image]->GetTextureWidth() / m_images[image]->GetTextureHeight();
    float aspectRatio = sourceAspectRatio / g_graphicsContext.GetPixelRatio(g_graphicsContext.GetVideoResolution());

    unsigned int newWidth = m_dwWidth;
    unsigned int newHeight = (unsigned int)((float)newWidth / aspectRatio);
    if ((m_aspectRatio == CGUIImage::ASPECT_RATIO_SCALE && newHeight < m_dwHeight) ||
        (m_aspectRatio == CGUIImage::ASPECT_RATIO_KEEP && newHeight > m_dwHeight))
    {
      newHeight = m_dwHeight;
      newWidth = (unsigned int)((float)newHeight * aspectRatio);
    }
    m_images[image]->SetPosition(m_iPosX - (int)(newWidth - m_dwWidth)/2, m_iPosY - (int)(newHeight - m_dwHeight)/2);
    m_images[image]->SetWidth(newWidth);
    m_images[image]->SetHeight(newHeight);
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

void CGUIMultiImage::SetAspectRatio(CGUIImage::GUIIMAGE_ASPECT_RATIO ratio)
{
  if (m_aspectRatio != ratio)
  {
    m_aspectRatio = ratio;
    m_bInvalidated = true;
  }
}

CGUIImage::GUIIMAGE_ASPECT_RATIO CGUIMultiImage::GetAspectRatio() const
{ 
  return m_aspectRatio;
}

void CGUIMultiImage::LoadDirectory()
{
  // Load any images from our texture bundle first
  m_files.clear();

  // don't load any images if our path is empty
  if (m_texturePath.IsEmpty()) return;

  g_TextureManager.GetBundledTexturesFromPath(m_texturePath, m_files);

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
      m_files.push_back(pItem->m_strPath);
  }

  // sort our images - they'll be randomized in AllocResources() if necessary
  sort(m_files.begin(), m_files.end());

  // flag as loaded - no point in constantly reloading them
  m_directoryLoaded = true;
}