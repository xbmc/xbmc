/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDInputStreamStack.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/File.h"
#include "filesystem/StackDirectory.h"
#include "utils/log.h"

#include <limits.h>

using namespace XFILE;

CDVDInputStreamStack::CDVDInputStreamStack(const CFileItem& fileitem) : CDVDInputStream(DVDSTREAM_TYPE_FILE, fileitem)
{
  m_eof = true;
  m_pos = 0;
  m_length = 0;
}

CDVDInputStreamStack::~CDVDInputStreamStack()
{
  Close();
}

bool CDVDInputStreamStack::IsEOF()
{
  return m_eof;
}

bool CDVDInputStreamStack::Open()
{
  if (!CDVDInputStream::Open())
    return false;

  CStackDirectory dir;
  CFileItemList   items;

  const CURL pathToUrl(m_item.GetDynPath());
  if(!dir.GetDirectory(pathToUrl, items))
  {
    CLog::Log(LOGERROR, "CDVDInputStreamStack::Open - failed to get list of stacked items");
    return false;
  }

  m_length = 0;
  m_eof    = false;

  for(int index = 0; index < items.Size(); index++)
  {
    TFile file(new CFile());

    if (!file->Open(items[index]->GetDynPath(), READ_TRUNCATED))
    {
      CLog::Log(LOGERROR, "CDVDInputStreamStack::Open - failed to open stack part '{}' - skipping",
                items[index]->GetDynPath());
      continue;
    }
    TSeg segment;
    segment.file   = file;
    segment.length = file->GetLength();

    if(segment.length <= 0)
    {
      CLog::Log(LOGERROR,
                "CDVDInputStreamStack::Open - failed to get file length for '{}' - skipping",
                items[index]->GetDynPath());
      continue;
    }

    m_length += segment.length;

    m_files.push_back(segment);
  }

  if(m_files.empty())
    return false;

  m_file = m_files[0].file;
  m_eof  = false;

  return true;
}

// close file and reset everything
void CDVDInputStreamStack::Close()
{
  CDVDInputStream::Close();
  m_files.clear();
  m_file.reset();
  m_eof = true;
}

int CDVDInputStreamStack::Read(uint8_t* buf, int buf_size)
{
  if(m_file == NULL || m_eof)
    return 0;

  unsigned int ret = m_file->Read(buf, buf_size);

  if(ret > INT_MAX)
    return -1;

  if(ret == 0)
  {
    m_eof = true;
    if(Seek(m_pos, SEEK_SET) < 0)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamStack::Read - failed to seek into next file");
      m_eof  = true;
      m_file.reset();
      return -1;
    }
  }

  m_pos += ret;

  return (int)ret;
}

int64_t CDVDInputStreamStack::Seek(int64_t offset, int whence)
{
  int64_t pos, len;

  if     (whence == SEEK_SET)
    pos = offset;
  else if(whence == SEEK_CUR)
    pos = offset + m_pos;
  else if(whence == SEEK_END)
    pos = offset + m_length;
  else
    return -1;

  len = 0;
  for(TSegVec::iterator it = m_files.begin(); it != m_files.end(); ++it)
  {
    if(len + it->length > pos)
    {
      TFile   file     = it->file;
      int64_t file_pos = pos - len;
      if(file->GetPosition() != file_pos)
      {
        if(file->Seek(file_pos, SEEK_SET) < 0)
          return false;
      }

      m_file = file;
      m_pos  = pos;
      m_eof  = false;
      return pos;
    }
    len += it->length;
  }

  return -1;
}

int64_t CDVDInputStreamStack::GetLength()
{
  return m_length;
}


