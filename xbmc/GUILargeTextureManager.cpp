/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUILargeTextureManager.h"
#include "Picture.h"
#include "GUISettings.h"
#include "Surface.h"
#include "FileItem.h"
#include "Util.h"

using namespace std;

CGUILargeTextureManager g_largeTextureManager;

CGUILargeTextureManager::CGUILargeTextureManager()
{
  m_running = false;
}

CGUILargeTextureManager::~CGUILargeTextureManager()
{
  m_bStop = true;
  m_listEvent.Set();
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
    SDL_Surface * texture = NULL;
    CPicture pic;
    CFileItem file(path, false);
    if (file.IsPicture() && !(file.IsZIP() || file.IsRAR() || file.IsCBR() || file.IsCBZ())) // ignore non-pictures
    { // check for filename only (i.e. lookup in skin/media/)
      CStdString loadPath(path);
      if ((size_t)path.FindOneOf("/\\") == CStdString::npos)
      {
        loadPath = g_TextureManager.GetTexturePath(path);
      }
      texture = pic.Load(loadPath, min(g_graphicsContext.GetWidth(), 1024), min(g_graphicsContext.GetHeight(), 720));
    }
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
      SDL_FreeSurface(texture);
      texture = NULL;
    }
    lock.Leave();
    m_listEvent.WaitMSec(1000);
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
  m_listEvent.Set();
}

// if available, increment reference count, and return the image.
// else, add to the queue list if appropriate.
CBaseTexture CGUILargeTextureManager::GetImage(const CStdString &path, int &orientation, bool firstRequest)
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
      orientation = image->GetOrientation();
      return CBaseTexture(image->GetTexture(), image->GetWidth(), image->GetHeight(), NULL, false);
    }
  }
  lock.Leave();

  if (firstRequest)
    QueueImage(path);

  return CBaseTexture();
}

void CGUILargeTextureManager::ReleaseImage(const CStdString &path, bool immediately)
{
  CSingleLock lock(m_listSection);
  for (listIterator it = m_allocated.begin(); it != m_allocated.end(); ++it)
  {
    CLargeTexture *image = *it;
    if (image->GetPath() == path)
    {
      if (image->DecrRef(immediately) && immediately)
        m_allocated.erase(it);
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
void CGUILargeTextureManager::QueueImage(const CStdString &path)
{
  CSingleLock lock(m_listSection);
  for (listIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    CLargeTexture *image = *it;
    if (image->GetPath() == path)
    {
      image->AddRef();
      return; // already queued
    }
  }

  // queue the item
  CLargeTexture *image = new CLargeTexture(path);
  m_queued.push_back(image);
  m_listEvent.Set();
  
  if(m_running)
    return;

  lock.Leave(); // done with our lock

  StopThread();
  Create();
}

