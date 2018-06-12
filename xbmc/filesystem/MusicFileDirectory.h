/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "IFileDirectory.h"
#include "music/tags/MusicInfoTag.h"

namespace XFILE
{
  class CMusicFileDirectory : public IFileDirectory
  {
    public:
      CMusicFileDirectory(void);
      ~CMusicFileDirectory(void) override;
      bool GetDirectory(const CURL& url, CFileItemList &items) override;
      bool Exists(const CURL& url) override;
      bool ContainsFiles(const CURL& url) override;
      bool AllowAll() const override { return true; }
    protected:
      virtual int GetTrackCount(const std::string& strPath) = 0;
      std::string m_strExt;
      MUSIC_INFO::CMusicInfoTag m_tag;
  };
}
