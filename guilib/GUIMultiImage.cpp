#include "include.h"
#include "../xbmc/FileItem.h"
#include "GUIMultiImage.h"
#include "TextureManager.h"
#include "../xbmc/FileSystem/HDDirectory.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "../xbmc/Util.h"

using namespace DIRECTORY;

CGUIMultiImage::CGUIMultiImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CStdString& strTexturePath, DWORD timePerImage, DWORD fadeTime, bool randomized, bool loop, DWORD timeToPauseAtEnd)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_currentPath = m_texturePath = strTexturePath;
  CUtil::AddSlashAtEnd(m_currentPath);
  CUtil::AddSlashAtEnd(m_texturePath);
  m_currentImage = 0;
  m_timePerImage = timePerImage;
  m_timeToPauseAtEnd = timeToPauseAtEnd;
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

void CGUIMultiImage::UpdateVisibility(const CGUIListItem *item)
{
  CGUIControl::UpdateVisibility(item);

  // check if we're hidden, and deallocate if so
  if (!IsVisible() && m_visible != DELAYED)
  {
    if (m_bDynamicResourceAlloc && IsAllocated())
      FreeResources();
    return;
  }

  // we are either delayed or visible, so we can allocate our resources

  // check for conditional information before we
  // alloc as this can free our resources
  if (m_Info)
  {
    CStdString texturePath = g_infoManager.GetImage(m_Info, WINDOW_INVALID);
    if (texturePath != m_currentPath && !texturePath.IsEmpty())
    {
      m_currentPath = texturePath;
      FreeResources();
      LoadDirectory();
    }
    else if (texturePath.IsEmpty() && m_currentPath != m_texturePath)
    {
      m_currentPath = m_texturePath;
      FreeResources();
      LoadDirectory();
    }
  }

  // and allocate our resources
  if (!IsAllocated())
    AllocResources();
}

void CGUIMultiImage::Render()
{
  if (!m_images.empty())
  {
    // Set a viewport so that we don't render outside the defined area
    g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);

    unsigned int nextImage = m_currentImage + 1;
    if (nextImage >= m_images.size())
      nextImage = m_loop ? 0 : m_currentImage;  // stay on the last image if <loop>no</loop>

    if (nextImage != m_currentImage)
    {
      // check if we should be loading a new image yet
      DWORD timeToShow = m_timePerImage;
      if (0 == nextImage) // last image should be paused for a bit longer if that's what the skinner wishes.
        timeToShow += m_timeToPauseAtEnd;
      if (m_imageTimer.IsRunning() && m_imageTimer.GetElapsedMilliseconds() > timeToShow)
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
        if (timeFading >= m_fadeTime)
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
        { // perform the fade in of next image
          float fadeAmount = timeFading / m_fadeTime;
          float alpha = (float)(m_diffuseColor >> 24) / 255.0f;
          if (alpha < 1 && alpha > 0)
          { // we have a semi-transparent image, so we need to use a more complicated
            // fade technique.  Assuming a black background (not generally true, but still...)
            // we have
            // b(t) = [a - b(1-t)*a] / a*(1-b(1-t)*a),
            // where a = alpha, and b(t):[0,1] -> [0,1] is the blend function.
            // solving, we get
            // b(t) = [1 - (1-a)^t] / a
            float blendIn = (1 - pow(1-alpha, fadeAmount)) / alpha;
            m_images[nextImage]->SetAlpha((unsigned char)(255 * blendIn));
            float blendOut = (1 - blendIn) / (1 - blendIn*alpha); // no need to use pow() again here
            m_images[m_currentImage]->SetAlpha((unsigned char)(255 * blendOut));
          }
          else
          { // simple case, just fade in the second image
            m_images[m_currentImage]->SetAlpha(255);
            m_images[nextImage]->SetAlpha((unsigned char)(255*fadeAmount));
          }
          m_images[m_currentImage]->Render();
        }
        m_images[nextImage]->Render();
      }
      else
      { // only one image - render it.
        m_images[m_currentImage]->Render();
      }
    }
    else
    { // only one image - render it.
      m_images[m_currentImage]->Render();
    }
    g_graphicsContext.RestoreClipRegion();
  }
  CGUIControl::Render();
}

bool CGUIMultiImage::OnAction(const CAction &action)
{
  return false;
}

bool CGUIMultiImage::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_REFRESH_THUMBS)
  {
    if (m_Info)
      FreeResources();
    return true;
  }
  return CGUIControl::OnMessage(message);
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
    CGUIImage *pImage = new CGUIImage(GetParentID(), GetID(), m_posX, m_posY, m_width, m_height, m_files[i]);
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
  m_images[image]->SetColorDiffuse(m_diffuseColor);

  // Scale image so that it will fill our render area
  if (m_aspectRatio != CGUIImage::ASPECT_RATIO_STRETCH)
  {
    // to get the pixel ratio, we must use the SCALED output sizes
    float pixelRatio = g_graphicsContext.GetScalingPixelRatio();

    float sourceAspectRatio = (float)m_images[image]->GetTextureWidth() / m_images[image]->GetTextureHeight();
    float aspectRatio = sourceAspectRatio / pixelRatio;

    float newWidth = m_width;
    float newHeight = newWidth / aspectRatio;
    if ((m_aspectRatio == CGUIImage::ASPECT_RATIO_SCALE && newHeight < m_height) ||
        (m_aspectRatio == CGUIImage::ASPECT_RATIO_KEEP && newHeight > m_height))
    {
      newHeight = m_height;
      newWidth = newHeight * aspectRatio;
    }
    m_images[image]->SetPosition(m_posX - (newWidth - m_width)*0.5f, m_posY - (newHeight - m_height)*0.5f);
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

void CGUIMultiImage::LoadDirectory()
{
  // Load any images from our texture bundle first
  m_files.clear();

  // don't load any images if our path is empty
  if (m_currentPath.IsEmpty()) return;

  // check to see if we have a single image or a folder of images
  CFileItem item(m_currentPath, false);
  if (item.IsPicture())
  {
    m_files.push_back(g_TextureManager.GetTexturePath(m_currentPath));
  }
  else
  { // folder of images
    g_TextureManager.GetBundledTexturesFromPath(m_currentPath, m_files);

    // Load in our images from the directory specified
    // m_currentPath is relative (as are all skin paths)
    CStdString realPath = g_TextureManager.GetTexturePath(m_currentPath);
    CHDDirectory dir;
    CFileItemList items;
    dir.GetDirectory(realPath, items);
    for (int i=0; i < items.Size(); i++)
    {
      CFileItem *pItem = items[i];
      if (pItem->IsPicture())
        m_files.push_back(pItem->m_strPath);
    }
  }

  // sort our images - they'll be randomized in AllocResources() if necessary
  sort(m_files.begin(), m_files.end());

  // flag as loaded - no point in constantly reloading them
  m_directoryLoaded = true;
}

