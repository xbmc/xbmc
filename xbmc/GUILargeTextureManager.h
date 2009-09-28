#pragma once

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

#include "utils/CriticalSection.h"
#include "utils/Job.h"
#include "TextureManager.h"

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
  virtual void DoWork();
  
  CStdString    m_path; ///< path of image to load
  CBaseTexture *m_texture; ///< Texture object to load the image into \sa CBaseTexture.
  int           m_width; ///< width of loaded image
  int           m_height; ///< height of loaded image
  int           m_orientation; ///< orientation of loaded image
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

  virtual void OnJobComplete(unsigned int jobID, CJob *job);

  bool GetImage(const CStdString &path, CTextureArray &texture, int &orientation, bool firstRequest);
  void ReleaseImage(const CStdString &path, bool immediately = false);

  void CleanupUnusedImages();

protected:
  class CLargeTexture
  {
  public:
    CLargeTexture(const CStdString &path);
    virtual ~CLargeTexture();

    void AddRef();
    bool DecrRef(bool deleteImmediately);
    bool DeleteIfRequired();
    void SetTexture(CBaseTexture* texture, int width, int height, int orientation);
    
    const CStdString &GetPath() const { return m_path; };
    const CTextureArray &GetTexture() const { return m_texture; };
    int GetOrientation() const { return m_orientation; };

  private:
    static const unsigned int TIME_TO_DELETE = 2000;

    unsigned int m_refCount;
    CStdString m_path;
    CTextureArray m_texture;
    int m_orientation;
    unsigned int m_timeToDelete;
  };

  void QueueImage(const CStdString &path);

private:
  std::vector< std::pair<unsigned int, CLargeTexture *> > m_queued;
  std::vector<CLargeTexture *> m_allocated;
  typedef std::vector<CLargeTexture *>::iterator listIterator;
  typedef std::vector< std::pair<unsigned int, CLargeTexture *> >::iterator queueIterator;

  CCriticalSection m_listSection;
};

extern CGUILargeTextureManager g_largeTextureManager;


