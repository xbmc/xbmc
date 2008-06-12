/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "FileMMS.h"

using namespace XFILE;

#ifdef HAS_MMS

CFileMMS::CFileMMS()
{
}

CFileMMS::~CFileMMS()
{
}

__int64 CFileMMS::GetPosition()
{
  return mms_get_current_pos(m_mms);
}

__int64 CFileMMS::GetLength()
{
  return mms_get_length(m_mms);
}


bool CFileMMS::Open(const CURL& url, bool bBinary)
{
  CStdString strUrl;
  url.GetURL(strUrl);

  m_mms = mms_connect(NULL, NULL, strUrl.c_str(), 128*1024);
  if (!m_mms)
  {
     return false;
  }

  return true;
}

unsigned int CFileMMS::Read(void* lpBuf, __int64 uiBufSize)
{
  int s = mms_read(NULL, m_mms, (char*) lpBuf, uiBufSize);
  return s;
}

__int64 CFileMMS::Seek(__int64 iFilePosition, int iWhence)
{
   return mms_seek(NULL, m_mms, iFilePosition, 0);
}

void CFileMMS::Close()
{
   mms_close(m_mms);
}

CStdString CFileMMS::GetContent()
{
   return "audio/x-ms-wma";
}

#endif // HAS_MMS
