/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/CharArrayParser.h"

#include <sstream>
#include <string>
#include <vector>

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
  bool IsIncompatible(CDVDInputStream* pInputStream, std::vector<uint8_t>& buf, size_t* bytesRead);

  /*!
   *  \brief Read some data of specified length, from the current position
   *  \param length The length of data to be read
   *  \return The string read
   */
  std::string Read(int length);

  /*!
   *  \brief Change the current data position to the specified offset
   *  \param offset The new position
   *  \return True if success, otherwise false
   */
  bool Seek(int offset);

  /*!
   *  \brief Read a line of data
   *  \param[OUT] line The data read
   *  \return True if read, otherwise false if EOF
   */
  bool ReadLine(std::string& line);

  /*!
   *  \brief Get the full data
   *  \return The data
   */
  const std::string& GetData() { return m_subtitleData; }

private:
  std::string m_subtitleData;
  CCharArrayParser m_arrayParser;
};
