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

// Code below is work in progress and not yet finished
//
// DONE:
// 1) make the code below work on Linux

#include "TSReader.h"
#include "client.h" //for XBMC->Log
#include "MultiFileReader.h"
#include "utils.h"
#if !defined(TARGET_WINDOWS)
#include "PlatformInclude.h"
#include "limits.h"
#define _strcmpi strcasecmp
#endif

using namespace ADDON;

CTsReader::CTsReader()
{
  m_fileReader=NULL;
  m_bLiveTv = false;
  m_bTimeShifting = false;
  liDelta.QuadPart = liCount.QuadPart = 0;
}

long CTsReader::Open(const char* pszFileName)//, const AM_MEDIA_TYPE *pmt)
{
  XBMC->Log(LOG_DEBUG, "CTsReader::Open(%s)", pszFileName);

  m_fileName = pszFileName;
  char url[MAX_PATH];
  strncpy(url, m_fileName.c_str(), MAX_PATH);
  //check file type
  int length = strlen(url);
  if ((length < 9) || (_strcmpi(&url[length-9], ".tsbuffer") != 0))
  {
    //local .ts file
    m_bTimeShifting = false;
    m_bLiveTv = false;
    m_fileReader = new FileReader();
  }
  else
  {
    //local timeshift buffer file file
    m_bTimeShifting = true;
    m_bLiveTv = true;
    m_fileReader = new MultiFileReader();
  }

  //open file
  if (m_fileReader->SetFileName(m_fileName.c_str()) != S_OK)
  {
    XBMC->Log(LOG_ERROR, "CTsReader::SetFileName failed.");
    return S_FALSE;
  }
  if (m_fileReader->OpenFile() != S_OK)
  {
    XBMC->Log(LOG_ERROR, "CTsReader::OpenFile failed.");
    return S_FALSE;
  }
  m_fileReader->SetFilePointer(0LL, FILE_BEGIN);

  return S_OK;
}

long CTsReader::Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes)
{
  LARGE_INTEGER liFrequency;
  LARGE_INTEGER liCurrent;
  LARGE_INTEGER liLast;
  if(m_fileReader)
  {
    // Save the performance counter frequency for later use.
    if (!QueryPerformanceFrequency(&liFrequency))
      XBMC->Log(LOG_ERROR, "QPF() failed with error %d\n", GetLastError());

    if (!QueryPerformanceCounter(&liCurrent))
		  XBMC->Log(LOG_ERROR, "QPC() failed with error %d\n", GetLastError());
    liLast = liCurrent;

    long rc = m_fileReader->Read(pbData, lDataLength, dwReadBytes);

    if (!QueryPerformanceCounter(&liCurrent))
      XBMC->Log(LOG_ERROR, "QPC() failed with error %d\n", GetLastError());
    
    // Convert difference in performance counter values to nanoseconds.
    liDelta.QuadPart += (((liCurrent.QuadPart - liLast.QuadPart) * 1000000) / liFrequency.QuadPart);
    liCount.QuadPart++;
    return rc;
  }

  dwReadBytes = 0;
  return 1;
}

void CTsReader::Close()
{
  if(m_fileReader)
  {
    m_fileReader->CloseFile();
    SAFE_DELETE(m_fileReader);
  }
}

unsigned long CTsReader::SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod)
{
  return m_fileReader->SetFilePointer(llDistanceToMove, dwMoveMethod);
}

int64_t CTsReader::GetFileSize()
{
  return m_fileReader->GetFileSize();
}

int64_t CTsReader::GetFilePointer()
{
  return m_fileReader->GetFilePointer();
}

void CTsReader::OnZap(void)
{
  m_fileReader->OnZap();
}

long long CTsReader::sigmaTime()
{
  return liDelta.QuadPart;
}
long long CTsReader::sigmaCount()
{
  return liCount.QuadPart;
}

#endif //TSREADER
