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

#include "client.h"
#include "FileReader.h"
#include "platform/util/StdString.h"
#include "Cards.h"

#ifdef LIVE555
class CRTSPClient;
class CMemoryBuffer;
#endif
typedef enum _TsReaderState
{
  State_Stopped = 0,
  State_Paused = 1,
  State_Running = 2
} TsReaderState;

class CTsReader
{
public:
  CTsReader();
  ~CTsReader(void);
  long Open(const char* pszFileName);
  long Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes);
  void Close();
  unsigned long SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod);
  int64_t GetFileSize();
  int64_t GetFilePointer();
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
  void SetCardId( int id );
  bool IsTimeShifting();
  bool IsSeeking();
  long Pause();

  TsReaderState State() {return m_State;};

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
  CRTSPClient*    m_rtspClient;
  CMemoryBuffer*  m_buffer;
#endif
  CCards*         m_cardSettings;     ///< Pointer to the MediaPortal card settings. Will be used to determine the base path of the timeshift buffer
  int             m_cardId;           ///< Card id for the current Card used for this timeshift buffer
  string          m_basePath;         ///< The base path shared by all timeshift buffers (to be determined from the Card settings)
  TsReaderState   m_State;            ///< The current state of the TsReader
  unsigned long   m_lastPause;        ///< Last time instance at which the playback was paused
  int             m_WaitForSeekToEof;
};
