/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UDFFile.h"

#include "URL.h"

#include <udfread/udfread.h>

using namespace XFILE;

CUDFFile::~CUDFFile()
{
  Close();
}

bool CUDFFile::Open(const CURL& url)
{
  if (m_context && m_file)
    return true;

  m_context = CUDFContext::Get(url.GetHostName());
  if (!m_context)
    return false;

  m_file = udfread_file_open(m_context->GetHandle(), url.GetFileName().c_str());
  if (!m_file)
  {
    Close();
    return false;
  }

  return true;
}

void CUDFFile::Close()
{
  // The file must be closed before the volume it was opened from is let go
  if (m_file)
  {
    udfread_file_close(m_file);
    m_file = nullptr;
  }

  m_context.reset();
}

int CUDFFile::Stat(const CURL& url, struct __stat64* buffer)
{
  if (!m_context || !m_file || !buffer)
    return -1;

  *buffer = {};
  buffer->st_size = GetLength();

  return 0;
}

ssize_t CUDFFile::Read(void* buffer, size_t size)
{
  return udfread_file_read(m_file, buffer, size);
}

int64_t CUDFFile::Seek(int64_t filePosition, int whence)
{
  return udfread_file_seek(m_file, filePosition, whence);
}

int64_t CUDFFile::GetLength()
{
  return udfread_file_size(m_file);
}

int64_t CUDFFile::GetPosition()
{
  return udfread_file_tell(m_file);
}

bool CUDFFile::Exists(const CURL& url)
{
  return Open(url);
}
