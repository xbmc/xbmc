#pragma once
/*
 *      Copyright (C) 2014 Arne Morten Kvarving
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


#include "IFileDirectory.h"
extern "C" {
#include <libavformat/avformat.h>
}

namespace XFILE
{
  class CAudioBookFileDirectory : public IFileDirectory
  {
    public:
      CAudioBookFileDirectory(void);
      ~CAudioBookFileDirectory(void) override;
      bool GetDirectory(const CURL& url, CFileItemList &items) override;
      bool Exists(const CURL& url) override;
      bool ContainsFiles(const CURL& url) override;
      bool IsAllowed(const CURL& url) const override { return true; };
    protected:
      AVIOContext* m_ioctx;
      AVFormatContext* m_fctx;
  };
}
