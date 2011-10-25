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

#include "client.h"
#include "FileReader.h"
#include "utils/StdString.h"

class CTsReader
{
public:
  CTsReader();
  ~CTsReader(void) {};
  long Open(const char* pszFileName);
  long Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes);
  void Close();
  void OnZap(void);

private:
  bool            m_bTimeShifting;
  bool            m_bRecording;
  bool            m_bLiveTv;
  CStdString      m_fileName;
  FileReader*     m_fileReader;
};
#endif //TSREADER
