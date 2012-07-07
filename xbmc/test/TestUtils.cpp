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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "TestUtils.h"
#include "Util.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"

#ifndef _LINUX
#include <windows.h>
#else
#include <cstdlib>
#include <climits>
#endif

class CTempFile : public XFILE::CFile
{
public:
  CTempFile(){};
  ~CTempFile()
  {
    Delete();
  }
  bool Create(const CStdString &suffix)
  {
    char tmp[MAX_PATH];
    int fd;

    m_ptempFilePath = CSpecialProtocol::TranslatePath("special://temp/");
    m_ptempFilePath += "xbmctempfileXXXXXX";
    m_ptempFilePath += suffix;
    if (m_ptempFilePath.length() >= MAX_PATH)
    {
      m_ptempFilePath = "";
      return false;
    }
    strcpy(tmp, m_ptempFilePath.c_str());

#ifndef _LINUX
    if (!GetTempFileName(CSpecialProtocol::TranslatePath("special://temp/"),
                         "xbmctempfile", 0, tmp))
    {
      m_ptempFilePath = "";
      return false;
    }
    m_ptempFilePath = tmp;
#else
    if ((fd = mkstemps(tmp, suffix.length())) < 0)
    {
      m_ptempFilePath = "";
      return false;
    }
    close(fd);
    m_ptempFilePath = tmp;
#endif

    OpenForWrite(m_ptempFilePath.c_str(), true);
    return true;
  }
  bool Delete()
  {
    Close();
    return CFile::Delete(m_ptempFilePath);
  };
private:
  CStdString m_ptempFilePath;
};

CXBMCTestUtils::CXBMCTestUtils(){}

CXBMCTestUtils &CXBMCTestUtils::Instance()
{
  static CXBMCTestUtils instance;
  return instance;
}

CStdString CXBMCTestUtils::ReferenceFilePath(CStdString const& path)
{
  return CSpecialProtocol::TranslatePath("special://xbmc") + path;
}

bool CXBMCTestUtils::SetReferenceFileBasePath()
{
  CStdString xbmcPath;
  CUtil::GetHomePath(xbmcPath);
  if (xbmcPath.IsEmpty())
    return false;

  /* Set xbmc path and xbmcbin path */
  CSpecialProtocol::SetXBMCPath(xbmcPath);
  CSpecialProtocol::SetXBMCBinPath(xbmcPath);

  return true;
}

XFILE::CFile *CXBMCTestUtils::CreateTempFile(CStdString const& suffix)
{
  CTempFile *f = new CTempFile();
  if (f->Create(suffix))
    return f;
  delete f;
  return NULL;
}

bool CXBMCTestUtils::DeleteTempFile(XFILE::CFile *tempfile)
{
  if (!tempfile)
    return true;
  CTempFile *f = static_cast<CTempFile*>(tempfile);
  bool retval = f->Delete();
  delete f;
  return retval;
}
