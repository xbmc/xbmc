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

#if (defined HAVE_CONFIG_H)
  #include "config.h"
#endif
#include "stdafx.h"
#include "OggTag.h"
#include "Util.h"


using namespace MUSIC_INFO;

//  From EMUmsvcrt.cpp to open a file for a dll
extern "C" FILE * dll_fopen (const char * filename, const char * mode);

COggTag::COggTag()
{

}

COggTag::~COggTag()
{
}

bool COggTag::Read(const CStdString& strFile1)
{
  if (!m_dll.Load())
    return false;

  CVorbisTag::Read(strFile1);

  CStdString strFile=strFile1;
  int currentStream=0;

  m_musicInfoTag.SetURL(strFile);

  CStdString strExtension;
  CUtil::GetExtension(strFile, strExtension);
  if (strExtension==".oggstream")
  {
    CStdString strFileName=CUtil::GetFileName(strFile);
    int iStart=strFileName.ReverseFind("-")+1;
    currentStream = atoi(strFileName.substr(iStart, strFileName.size()-iStart-10).c_str())-1;
    CStdString strPath=strFile;
    CUtil::GetDirectory(strPath, strFile);
    CUtil::RemoveSlashAtEnd(strFile);   // we want the filename
  }

  #if (defined USE_EXTERNAL_LIBRARIES) || (defined USE_EXTERNAL_LIBVORBIS)
  // Call fopen() directly if we're using external libraries
  FILE* file=fopen(strFile.c_str(), "r");
  #else
  //Use the emulated fopen() as its only used inside the dll
  FILE* file=dll_fopen (strFile.c_str(), "r");
  #endif
  if (!file)
    return false;

  OggVorbis_File vf;
  //  open ogg file with decoder
  if (m_dll.ov_open(file, &vf, NULL, 0)!=0)
    return false;

  int iStreams=m_dll.ov_streams(&vf);
  if (iStreams>1)
  {
    if (currentStream > iStreams)
    {
      m_dll.ov_clear(&vf);
      return false;
    }
  }

  m_musicInfoTag.SetDuration((int)m_dll.ov_time_total(&vf, currentStream));

  vorbis_comment* pComments=m_dll.ov_comment(&vf, currentStream);
  if (pComments)
  {
    for (int i=0; i<pComments->comments; ++i)
    {
      CStdString strEntry=pComments->user_comments[i];
      ParseTagEntry(strEntry);
    }
  }
  m_dll.ov_clear(&vf);
  return true;
}

int COggTag::GetStreamCount(const CStdString& strFile)
{
  if (!m_dll.Load())
    return 0;

  #if (defined USE_EXTERNAL_LIBRARIES) || (defined USE_EXTERNAL_LIBVORBIS)
  // Call fopen() directly if we're using external libraries
  FILE* file=fopen(strFile.c_str(), "r");
  #else
  FILE* file=dll_fopen (strFile.c_str(), "r");
  #endif
  if (!file)
    return 0;

  OggVorbis_File vf;
  //  open ogg file with decoder
  if (m_dll.ov_open(file, &vf, NULL, 0)!=0)
    return 0;

  int iStreams=m_dll.ov_streams(&vf);

  m_dll.ov_clear(&vf);

  return iStreams;
}

