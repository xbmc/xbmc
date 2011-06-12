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

#include "GUILargeTextureManager.h"
#include "pictures/Picture.h"
#include "settings/GUISettings.h"
#include "FileItem.h"
#include "guilib/Texture.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"
#include "utils/JobManager.h"
#include "guilib/GraphicContext.h"
#include "utils/log.h"
#include "TextureCache.h"

using namespace std;


CImageLoader::CImageLoader(const CStdString &path)
{
  m_path = path;
  m_texture = NULL;
}

CImageLoader::~CImageLoader()
{
  delete(m_texture);
}

bool CImageLoader::DoWork()
{
  CFileItem file(m_path, false);
  if (file.IsPicture() && !(file.IsArchive() || file.IsZIP() || file.IsRAR() || file.IsCBR() || file.IsCBZ())) // ignore non-pictures
  { // check for filename only (i.e. lookup in skin/media/)
    CStdString loadPath = g_TextureManager.GetTexturePath(m_path);

    // cache the image if necessary
    loadPath = CTextureCache::Get().CheckAndCacheImage(loadPath);
    if (loadPath.IsEmpty())
      return false;

    m_texture = new CTexture();
    DWORD start = CTimeUtils::GetTimeMS();
    if (!m_texture->LoadFromFile(loadPath, min(g_graphicsContext.GetWidth(), 2048), min(g_graphicsContext.GetHeight(), 1080), g_guiSettings.GetBool("pictures.useexifrotation")))
    {
      delete m_texture;
      m_texture = NULL;
    }
    else if (CTimeUtils::GetTimeMS() - start > 100)
      CLog::Log(LOGDEBUG, "%s - took %d ms to load %s", __FUNCTION__, CTimeUtils::GetTimeMS() - start, loadPath.c_str());
  }

  return true;
}

CGUILargeTextureManager::CLargeTexture::CLargeTexture(const CStdString &path)
{
  m_path = path;
  m_refCount = 1;
  m_timeToDelete = 0;
}

CGUILargeTextureManager::CLargeTexture::~CLargeTexture()
{
  assert(m_refCount == 0);
  m_texture.Free();
}

void CGUILargeTextureManager::CLargeTexture::AddRef()
{
  m_refCount++;
}

bool CGUILargeTextureManager::CLargeTexture::DecrRef(bool deleteImmediately)
{
  assert(m_refCount);
  m_refCount--;
  if (m_refCount == 0)
  {
    if (deleteImmediately)
      delete this;
    else
      m_timeToDelete = CTimeUtils::GetFrameTime() + TIME_TO_DELETE;
    return true;
  }
  return false;
}

bool CGUILargeTextureManager::CLargeTexture::DeleteIfRequired(bool deleteImmediately)
{
  if (m_refCount == 0 && (deleteImmediately || m_timeToDelete < CTimeUtils::GetFrameTime()))
  {
    delete this;
    return true;
  }
  return false;
}

void CGUILargeTextureManager::CLargeTexture::SetTexture(CBaseTexture* texture)
{
  assert(!m_texture.size());
  if (texture)
    m_texture.Set(texture, texture->GetWidth(), texture->GetHeight());
}

CGUILargeTextureManager::CGUILargeTextureManager()
{
}

CGUILargeTextureManager::~CGUILargeTextureManager()
{
}

void CGUILargeTextureManager::CleanupUnusedImages(bool immediately)
{
  CSingleLock lock(m_listSection);
  // check for items to remove from allocated list, and remove
  listIterator it = m_allocated.begin();
  while (it != m_allocated.end())
  {
    CLargeTexture *image = *it;
    if (image->DeleteIfRequired(immediately))
      it = m_allocated.erase(it);
    else
      ++it;
  }
}

// if available, increment reference count, and return the image.
// else, add to the queue list if appropriate.
bool CGUILargeTextureManager::GetImage(const CStdString &path, CTextureArray &texture, bool firstRequest)
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
      texture = image->GetTexture();
      return texture.size() > 0;
    }
  }

  if (firstRequest)
    QueueImage(path);

  return true;
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
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    unsigned int id = it->first;
    CLargeTexture *image = it->second;
    if (image->GetPath() == path && image->DecrRef(true))
    {
      // cancel this job
      CJobManager::GetInstance().CancelJob(id);
      m_queued.erase(it);
      return;
    }
  }
}

// queue the image, and start the background loader if necessary
void CGUILargeTextureManager::QueueImage(const CStdString &path)
{
  CSingleLock lock(m_listSection);
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    CLargeTexture *image = it->second;
    if (image->GetPath() == path)
    {
      image->AddRef();
      return; // already queued
    }
  }

  // queue the item
  CLargeTexture *image = new CLargeTexture(path);
  unsigned int jobID = CJobManager::GetInstance().AddJob(new CImageLoader(path), this, CJob::PRIORITY_NORMAL);
  m_queued.push_back(make_pair(jobID, image));
}

void CGUILargeTextureManager::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  // see if we still have this job id
  CSingleLock lock(m_listSection);
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    if (it->first == jobID)
    { // found our job
      CImageLoader *loader = (CImageLoader *)job;
      CLargeTexture *image = it->second;
      image->SetTexture(loader->m_texture);
      loader->m_texture = NULL; // we want to keep the texture, and jobs are auto-deleted.
      m_queued.erase(it);
      m_allocated.push_back(image);
      return;
    }
  }
}



