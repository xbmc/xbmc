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

/*
 *  This file originates from TSFileSource, a GPL directshow push
 *  source filter that provides an MPEG transport stream output.
 *  Copyright (C) 2005      nate
 *  Copyright (C) 2006      bear
 *
 *  nate can be reached on the forums at
 *    http://forums.dvbowners.com/
 */

#if defined TSREADER

#include "FileReader.h"
#include <vector>
#include <string>

class MultiFileReaderFile
{
  public:
    std::string filename;
    int64_t startPosition;
    int64_t length;
    long filePositionId;
};

class MultiFileReader : public FileReader
{
  public:
    MultiFileReader();
    virtual ~MultiFileReader();

    virtual long GetFileName(char* *lpszFileName);
    virtual long SetFileName(const char* pszFileName);
    virtual long OpenFile();
    virtual long CloseFile();
    virtual long Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes);
    virtual bool IsFileInvalid();
    virtual unsigned long SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod);
    virtual int64_t GetFilePointer();
    virtual int64_t GetFileSize();
    virtual void OnZap(void);

  protected:
    long RefreshTSBufferFile();
    long GetFileLength(const char* pFilename, int64_t &length);
    size_t WcsLen(const void *str);
    size_t WcsToMbs(char *s, const void *w, size_t n);

    //  SharedMemory* m_pSharedMemory;
    FileReader m_TSBufferFile;
    int64_t m_startPosition;
    int64_t m_endPosition;
    int64_t m_currentReadPosition;
    long m_filesAdded;
    long m_filesRemoved;
    int64_t m_lastZapPosition;

    std::vector<MultiFileReaderFile *> m_tsFiles;

    FileReader m_TSFile;
    long     m_TSFileId;
    bool     m_bReadOnly;
    bool     m_bDelay;
    bool     m_bDebugOutput;
};

#endif
