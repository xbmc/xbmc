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
  m_fileReader->SetFileName(m_fileName.c_str());
  m_fileReader->OpenFile();
  m_fileReader->SetFilePointer(0LL, FILE_BEGIN);

  return S_OK;
}

long CTsReader::Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes)
{
  if(m_fileReader)
  {
    return m_fileReader->Read(pbData, lDataLength, dwReadBytes);
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

void CTsReader::OnZap(void)
{
  m_fileReader->SetFilePointer(0LL, FILE_END);
}

#endif //TSREADER
