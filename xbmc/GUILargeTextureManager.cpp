#include "stdafx.h"
#include "GUILargeTextureManager.h"
#include "Picture.h"

CGUILargeTextureManager g_largeTextureManager;

CGUILargeTextureManager::CGUILargeTextureManager()
{
  m_running = false;
}

CGUILargeTextureManager::~CGUILargeTextureManager()
{
  StopThread();
}

// Process loop for this thread
// Check and deallocate images that have been finished with.
// And allocate new images that have been queued.
// Once there's nothing queued or allocated, end the thread.
void CGUILargeTextureManager::Process()
{
  // lock item list
  CSingleLock lock(m_listSection);
  m_running = true;
  while (m_queued.size() && !m_bStop)
  { // load the top item in the queue
    // take a copy of the details required for the load, as
    // it may be no longer required by the time the load is complete
    CLargeTexture *image = m_queued[0];
    CStdString path = image->GetPath();
    lock.Leave();
    // load the image using our image lib
    LPDIRECT3DTEXTURE8 texture = NULL;
    CPicture pic;
    CFileItem file(path, false);
    if (file.IsPicture() && !(file.IsZIP() || file.IsRAR() || file.IsCBR() || file.IsCBZ())) // ignore non-pictures
      texture = pic.Load(path, min(g_graphicsContext.GetWidth(), 1024), min(g_graphicsContext.GetHeight(), 720));
    // and add to our allocated list
    lock.Enter();
    if (m_queued.size() && m_queued[0]->GetPath() == path)
    {
      // still have the same image in the queue, so move it across to the
      // allocated list, even if it doesn't exist
      CLargeTexture *image = m_queued[0];
      image->SetTexture(texture, pic.GetWidth(), pic.GetHeight(), (g_guiSettings.GetBool("pictures.useexifrotation") && pic.GetExifInfo()->Orientation) ? pic.GetExifInfo()->Orientation - 1: 0);
      m_allocated.push_back(image);
      m_queued.erase(m_queued.begin());
    }
    else
    { // no need for the texture any more
      SAFE_RELEASE(texture);
    }
    lock.Leave();
    Sleep(1);
    lock.Enter();
  }
  m_running = false;
}

void CGUILargeTextureManager::CleanupUnusedImages()
{
  CSingleLock lock(m_listSection);
  // check for items to remove from allocated list, and remove
  listIterator it = m_allocated.begin();
  while (it != m_allocated.end())
  {
    CLargeTexture *image = *it;
    if (image->DeleteIfRequired())
      it = m_allocated.erase(it);
    else
      ++it;
  }
}

// if available, increment reference count, and return the image.
// else, add to the queue list if appropriate.
LPDIRECT3DTEXTURE8 CGUILargeTextureManager::GetImage(const CStdString &path, int &width, int &height, int &orientation, bool firstRequest)
{
  // note: max size to load images: 2048x1024? (8MB)
  CSingleLock lock(m_listSection);
  for (listIterator it = m_allocated.begin(); it != m_allocated.end(); ++it)
  {
    CLargeTexture *image = *it;
    if (image->GetPath() == path)
    {
      if (firstRequest)
        image->AddRef();
      width = image->GetWidth();
      height = image->GetHeight();
      orientation = image->GetOrientation();
      return image->GetTexture();
    }
  }
  if (firstRequest)
  {
    for (listIterator it = m_queued.begin(); it != m_queued.end(); ++it)
    {
      CLargeTexture *image = *it;
      if (image->GetPath() == path)
      {
        image->AddRef();
        return NULL; // already queued - it'll be returned above.
      }
    }
    // queue the item
    CLargeTexture *image = new CLargeTexture(path);
    QueueImage(image);
  }
  return NULL;
}

void CGUILargeTextureManager::ReleaseImage(const CStdString &path)
{
  CSingleLock lock(m_listSection);
  for (listIterator it = m_allocated.begin(); it != m_allocated.end(); ++it)
  {
    CLargeTexture *image = *it;
    if (image->GetPath() == path)
    {
      image->DecrRef(false);
      return;
    }
  }
  for (listIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    CLargeTexture *image = *it;
    if (image->GetPath() == path && image->DecrRef(true))
    {
      m_queued.erase(it);
      return;
    }
  }
}

// queue the image, and start the background loader if necessary
void CGUILargeTextureManager::QueueImage(CLargeTexture *image)
{
  CSingleLock lock(m_listSection);
  m_queued.push_back(image);

  if (!m_running)
  {
    StopThread();
    Create();
  }
}

