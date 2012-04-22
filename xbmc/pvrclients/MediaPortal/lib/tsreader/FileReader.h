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

#include "os-dependent.h"

class FileReader
{
  public:
    FileReader();
    virtual ~FileReader();

    // Open and write to the file
    virtual long GetFileName(char* *lpszFileName);
    virtual long SetFileName(const char* pszFileName);
    virtual long OpenFile();
    virtual long CloseFile();
    virtual long Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes);
    virtual long Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes, int64_t llDistanceToMove, unsigned long dwMoveMethod);
    virtual long get_ReadOnly(bool *ReadOnly);
    virtual long get_DelayMode(bool *DelayMode);
    virtual long set_DelayMode(bool DelayMode);
    virtual long get_ReaderMode(unsigned short *ReaderMode);
    virtual long GetFileSize(int64_t *pStartPosition, int64_t *pLength);
    long GetStartPosition(int64_t *lpllpos);
    virtual bool IsFileInvalid();
    virtual unsigned long SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod);
    virtual int64_t GetFilePointer();
    virtual unsigned long setFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod);
    virtual int64_t getFilePointer();
    virtual int64_t getBufferPointer();
    virtual void setBufferPointer();

    void SetDebugOutput(bool bDebugOutput);

    virtual int64_t GetFileSize();
    virtual bool IsBuffer(){return false;};
    virtual bool HasMoreData(int bytes){return false;};
    virtual int HasData(){return 0; } ;

  protected:
#if defined(TARGET_WINDOWS)
    HANDLE   m_hFile;               // Handle to file for streaming
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
    XFILE::CFile m_hFile;           // Handle to file for streaming
#endif
    char*    m_pFileName;           // The filename where we stream
    bool     m_bReadOnly;
    bool     m_bDelay;
    int64_t  m_fileSize;
    int64_t  m_fileStartPos;
    int64_t  m_llBufferPointer;

    bool     m_bDebugOutput;
};

#endif //_WIN32 && TSREADER
