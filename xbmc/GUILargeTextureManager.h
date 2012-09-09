#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/CriticalSection.h"
#include "utils/Job.h"
#include "guilib/TextureManager.h"

/*!
 \ingroup textures,jobs
 \brief Image loader job class

 Used by the CGUILargeTextureManager to perform asynchronous loading of textures.

 \sa CGUILargeTextureManager and CJob
 */
class CImageLoader : public CJob
{
public:
  CImageLoader(const CStdString &path);
  virtual ~CImageLoader();

  /*!
   \brief Work function that loads in a particular image.
   */
  virtual bool DoWork();

  CStdString    m_path; ///< path of image to load
  CBaseTexture *m_texture; ///< Texture object to load the image into \sa CBaseTexture.
};

/*!
 \ingroup textures
 \brief Background texture loading manager

 Used to load textures for the user interface asynchronously, allowing fluid framerates
 while background loading textures.

 \sa IJobCallback, CGUITexture
 */
class CGUILargeTextureManager : public IJobCallback
{
public:
  CGUILargeTextureManager();
  virtual ~CGUILargeTextureManager();

  /*!
   \brief Callback from CImageLoader on completion of a loaded image

   Transfers texture information from the loading job to our allocated texture list.

   \sa CImageLoader, IJobCallback
   */
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

  /*!
   \brief Request a texture to be loaded in the background.

   Loaded textures are reference counted, hence this call may immediately return with the texture
   object filled if the texture has been previously loaded, else will return with an empty texture
   object if it is being loaded.

   \param path path of the image to load.
   \param texture texture object to hold the resulting texture
   \param orientation orientation of resulting texture
   \param firstRequest true if this is the first time we are requesting this texture
   \return true if the image exists, else false.
   \sa CGUITextureArray and CGUITexture
   */
  bool GetImage(const CStdString &path, CTextureArray &texture, bool firstRequest);

  /*!
   \brief Request a texture to be unloaded.

   When textures are finished with, this function should be called.  This decrements the texture's
   reference count, and schedules it to be unloaded once the reference count reaches zero.  If the
   texture is still queued for loading, or is in the process of loading, the image load is cancelled.

   \param path path of the image to release.
   \param immediately if set true the image is immediately unloaded once its reference count reaches zero
                      rather than being unloaded after a delay.
   */
  void ReleaseImage(const CStdString &path, bool immediately = false);

  /*!
   \brief Cleanup images that are no longer in use.

   Loaded textures are reference counted, and upon reaching reference count 0 through ReleaseImage()
   they are flagged as unused with the current time.  After a delay they may be unloaded, hence
   CleanupUnusedImages() should be called periodically to ensure this occurs.

   \param immediately set to true to cleanup images regardless of whether the delay has passed
   */
  void CleanupUnusedImages(bool immediately = false);

private:
  class CLargeTexture
  {
  public:
    CLargeTexture(const CStdString &path);
    virtual ~CLargeTexture();

    void AddRef();
    bool DecrRef(bool deleteImmediately);
    bool DeleteIfRequired(bool deleteImmediately = false);
    void SetTexture(CBaseTexture* texture);

    const CStdString &GetPath() const { return m_path; };
    const CTextureArray &GetTexture() const { return m_texture; };

  private:
    static const unsigned int TIME_TO_DELETE = 2000;

    unsigned int m_refCount;
    CStdString m_path;
    CTextureArray m_texture;
    unsigned int m_timeToDelete;
  };

  void QueueImage(const CStdString &path);

  std::vector< std::pair<unsigned int, CLargeTexture *> > m_queued;
  std::vector<CLargeTexture *> m_allocated;
  typedef std::vector<CLargeTexture *>::iterator listIterator;
  typedef std::vector< std::pair<unsigned int, CLargeTexture *> >::iterator queueIterator;

  CCriticalSection m_listSection;
};

extern CGUILargeTextureManager g_largeTextureManager;


