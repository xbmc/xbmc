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

#include "IDirectory.h"
#include "Settings.h"

namespace DIRECTORY
{

  /*!
  \ingroup windows
  \brief Get access to shares and it's directories.
  */
  class CVirtualDirectory : public IDirectory
  {
  public:
    CVirtualDirectory(void);
    virtual ~CVirtualDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items, bool bUseFileDirectories);
    void SetSources(VECSOURCES& vecSources);
    inline unsigned int GetNumberOfSources() {
      if (m_vecSources)
        return m_vecSources->size();
      else
        return 0;
      }
    bool IsSource(const CStdString& strPath) const;
    bool IsInSource(const CStdString& strPath) const;

    inline const CMediaSource& operator [](const int index) const
    {
      return m_vecSources->at(index);
    }

    inline CMediaSource& operator[](const int index)
    {
      return m_vecSources->at(index);
    }

    void GetSources(VECSOURCES &sources) const;

    void AllowNonLocalSources(bool allow) { m_allowNonLocalSources = allow; };

  protected:
    void CacheThumbs(CFileItemList &items);

    VECSOURCES* m_vecSources;
    bool       m_allowNonLocalSources;
  };
}
