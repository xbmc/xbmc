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
#include "Util.h"
#include "ModuleCodec.h"
#include "FileSystem/File.h"

using namespace XFILE;

bool ModuleCodec::IsSupportedFormat(const CStdString& strExt)
{
  if (strExt == "mod" || strExt == "it" || strExt == "s3m" || strExt == "duh" || strExt == "xm")
    return true;
  
  return false;
}

ModuleCodec::ModuleCodec()
{
  m_CodecName = "MOD";
  m_module = 0;
  m_renderID = 0;
}

ModuleCodec::~ModuleCodec()
{
  DeInit();
}

bool ModuleCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  DeInit();

  if (!m_dll.Load())
    return false; // error logged previously

  // set correct codec name
  CUtil::GetExtension(strFile,m_CodecName);
  m_CodecName.erase(0,1);
  m_CodecName.ToUpper();

  CStdString strLoadFile = "special://temp/cachedmod";
  if (!CUtil::IsHD(strFile))
    CFile::Cache(strFile,"special://temp/cachedmod");
  else
    strLoadFile = strFile;
  
  m_module = m_dll.LoadModule(strLoadFile.c_str());
  if (!m_module)
  {
    CLog::Log(LOGERROR,"ModuleCodec: error opening file %s!",strFile.c_str());
    return false;
  }
  m_Channels = 2;
  m_SampleRate = 48000;
  m_BitsPerSample = 16;
  m_TotalTime = (__int64)(m_dll.GetModuleLength(m_module))/65536*1000;

  return true;
}

void ModuleCodec::DeInit()
{
  if (m_renderID)
    m_dll.StopPlayback(m_renderID);
  if (m_module)
    m_dll.FreeModule(m_module);

  m_renderID = 0;
  m_module = 0;
}

__int64 ModuleCodec::Seek(__int64 iSeekTime)
{
  if (m_renderID)
    m_dll.StopPlayback(m_renderID);
  
  m_renderID = m_dll.StartPlayback(m_module,long(iSeekTime/1000*65536));
  if (m_renderID)
    return iSeekTime;

  return -1;
}

int ModuleCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (!m_module)
    return READ_ERROR;
  
  if (m_dll.GetModulePosition(m_renderID) >= m_dll.GetModuleLength(m_module))
    return READ_EOF;

  if (!m_renderID)
    m_renderID = m_dll.StartPlayback(m_module,0);

  if ((*actualsize=m_dll.FillBuffer(m_module,m_renderID,(char*)pBuffer,size,1.f)*4) == size)
    return READ_SUCCESS;
    
  return READ_ERROR;
}

bool ModuleCodec::CanInit()
{
  return m_dll.CanLoad();
}

