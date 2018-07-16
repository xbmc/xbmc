/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/auto_buffer.h"

#include <string>
#include <sstream>

class CDVDInputStream;

// buffered class for subtitle reading

class CDVDSubtitleStream
{
public:
  CDVDSubtitleStream();
  virtual ~CDVDSubtitleStream();

  bool Open(const std::string& strFile);

  /** \brief Checks if the subtitle associated with the pInputStream
   *         is known to be incompatible, e.g., vob sub files.
   *  \param[in] pInputStream The input stream for the subtitle to check.
   */
  bool IsIncompatible(CDVDInputStream* pInputStream, XUTILS::auto_buffer& buf, size_t* bytesRead);

  int Read(char* buf, int buf_size);
  long Seek(long offset, int whence);

  char* ReadLine(char* pBuffer, int iLen);
  //wchar* ReadLineW(wchar* pBuffer, int iLen) { return NULL; };

  std::stringstream m_stringstream;
};

