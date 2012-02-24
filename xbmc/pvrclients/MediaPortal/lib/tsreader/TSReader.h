#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef TSREADER

#include "RTSPClient.h"
#include "client.h"
#include "FileReader.h"
#include "MemoryBuffer.h"
#include "utils/StdString.h"
#include "Cards.h"

class CTsReader
{
public:
  CTsReader();
  ~CTsReader(void) {};
  long Open(const char* pszFileName);
  long Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes);
  void Close();
  bool OnZap(const char* pszFileName, int64_t timeShiftBufferPos, long timeshiftBufferID);

  /**
   * \brief Pass a pointer to the MediaPortal card settings to this class
   * \param the cardSettings
   */
  void SetCardSettings(CCards* cardSettings);

  /**
   * \brief Override the search directory for timeshift buffer files
   * \param the new search directory
   */
  void SetDirectory( string& directory );

private:

  /**
   * \brief Translate the given path using the m_basePath setting
   * \param The original (local) timeshift buffer file path on the TV server side
   */
  std::string TranslatePath(const char* pszFileName);

  bool            m_bTimeShifting;
  bool            m_bRecording;
  bool            m_bLiveTv;
  bool            m_bIsRTSP;
  CStdString      m_fileName;
  FileReader*     m_fileReader;
  FileReader*     m_fileDuration;
#ifdef LIVE555
  CRTSPClient     m_rtspClient;
  CMemoryBuffer   m_buffer;
#endif
  CCards*         m_cardSettings;     ///< Pointer to the MediaPortal card settings. Will be used to determine the base path of the timeshift buffer
  string          m_basePath;         ///< The base path shared by all timeshift buffers (to be determined from the Card settings)

};
#endif //TSREADER
