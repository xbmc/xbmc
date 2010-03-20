/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "DVDInputStreamMPLS.h"
#include "DVDFactoryInputStream.h"
#include "Util.h"
#include "FileSystem/StackDirectory.h"

extern "C"
{
#include "libbdnav/mpls_parse.h"
#include "libbdnav/navigation.h"
}

#include <vector>

using namespace std;
using namespace XFILE;

CDVDInputStreamMPLS::CDVDInputStreamMPLS() :
  CDVDInputStream(DVDSTREAM_TYPE_MPLS)
{
  m_stream = NULL;
}

CDVDInputStreamMPLS::~CDVDInputStreamMPLS()
{
  Close();
}

bool CDVDInputStreamMPLS::IsEOF()
{
  return m_stream->IsEOF();
}

bool CDVDInputStreamMPLS::Open(const char* strFile, const std::string& content)
{
  CStdString strPath;
  CUtil::GetDirectory(strFile,strPath);
  strPath = CUtil::GetParentPath(strPath);
  CStdString strParentPath = CUtil::GetParentPath(strPath);
  CUtil::RemoveSlashAtEnd(strParentPath);
  char* mainTitle=NULL;
  if (CUtil::GetFileName(strFile).Equals("main.mpls"))
    mainTitle = nav_find_main_title(const_cast<char*>(strParentPath.c_str()));
  else
    mainTitle = strdup(CUtil::GetFileName(strFile).c_str());
  if (!mainTitle || strlen(mainTitle) == 0)
  {
    free(mainTitle);
    mainTitle = strdup("00000.mpls");
  }

  CStdString strPlaylist = CUtil::AddFileToFolder(strPath,CStdString("PLAYLIST/")+mainTitle);
  MPLS_PL* playList = mpls_parse(const_cast<char*>(strPlaylist.c_str()),0);
  if (playList)
  {
    vector<CStdString> paths;
/* TODO: convert mpls offsets to edl */
    for( int i=0;i<playList->list_count;++i)
    {
      CStdString strFile;
      strFile.Format("STREAM/%s.m2ts",playList->play_item[i].clip[0].clip_id);
      paths.push_back(CUtil::AddFileToFolder(strPath,strFile));
    }
    if (paths.size() > 1)
    {
      CStackDirectory dir;
      dir.ConstructStackPath(paths,strPath);
    }
    else
      strPath = paths[0];
    mpls_free(playList);
  }
  free(mainTitle);
  if (!(m_stream = CDVDFactoryInputStream::CreateInputStream(NULL,strPath,content)))
    return false;

  return m_stream->Open(strPath.c_str(),content);
}

// close file and reset everyting
void CDVDInputStreamMPLS::Close()
{
  m_stream->Close();
  delete m_stream;
  m_stream = NULL;
}

int CDVDInputStreamMPLS::Read(BYTE* buf, int buf_size)
{
  return m_stream->Read(buf,buf_size);
}

__int64 CDVDInputStreamMPLS::Seek(__int64 offset, int whence)
{
  return m_stream->Seek(offset,whence);
}

__int64 CDVDInputStreamMPLS::GetLength()
{
  return m_stream->GetLength();
}

BitstreamStats CDVDInputStreamMPLS::GetBitstreamStats() const
{
  return m_stream->GetBitstreamStats();
}

