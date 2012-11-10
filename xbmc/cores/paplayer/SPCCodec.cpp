/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "SPCCodec.h"
#include "cores/DllLoader/DllLoader.h"
#include "cores/DllLoader/SoLoader.h"
#include "music/tags/MusicInfoTagLoaderSPC.h"
#include "music/tags/MusicInfoTag.h"
#include "filesystem/File.h"
#include "DynamicDll.h"
#include "Util.h"
#include "utils/log.h"
#ifdef _WIN32
#include "cores/DllLoader/Win32DllLoader.h"
#endif

using namespace MUSIC_INFO;
using namespace XFILE;

SPCCodec::SPCCodec()
{
  m_CodecName = "spc";
  m_szBuffer = NULL;
  m_pApuRAM = NULL;
  m_iDataPos = 0;
  m_loader = NULL;
  m_dll.EmuAPU = NULL;
  m_dll.LoadSPCFile = NULL;
  m_dll.SeekAPU = NULL;
#ifdef _LINUX
  m_dll.ResetAPU = NULL;
  m_dll.InitAPU = NULL;
#endif
}

SPCCodec::~SPCCodec()
{
  DeInit();
}

bool SPCCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  // SNESAPU can ONLY be opened and used by one instance (lot's of statics).
  // So to work around this problem with SNESAPU, we need to make sure that
  // each instance of SPCCodec has it's own instance of SNESAPU. Do this by
  // coping DLL_PATH_SPC_CODEC into special://temp and using a unique name. Then
  // loading this unique named SNESAPU as the library.
  // This forces the shared lib loader to load a per-instance copy of SNESAPU.
#ifdef _LINUX
  m_loader_name = CUtil::GetNextFilename("special://temp/SNESAPU-%03d.so", 999);
  XFILE::CFile::Cache(DLL_PATH_SPC_CODEC, m_loader_name);

  m_loader = new SoLoader(m_loader_name);
#else
  m_loader_name = CUtil::GetNextFilename("special://temp/SNESAPU-%03d.dll", 999);
  XFILE::CFile::Cache(DLL_PATH_SPC_CODEC, m_loader_name);

  m_loader = new Win32DllLoader(m_loader_name);
#endif
  if (!m_loader)
  {
    XFILE::CFile::Delete(m_loader_name);
    return false;
  }

  if (!m_loader->Load())
  {
    delete m_loader;
    m_loader = NULL;
    XFILE::CFile::Delete(m_loader_name);
    return false;
  }

  m_loader->ResolveExport("LoadSPCFile",(void**)&m_dll.LoadSPCFile);
  m_loader->ResolveExport("EmuAPU",(void**)&m_dll.EmuAPU);
  m_loader->ResolveExport("SeekAPU",(void**)&m_dll.SeekAPU);
#ifdef _LINUX
  m_loader->ResolveExport("InitAPU",(void**)&m_dll.InitAPU);
  m_loader->ResolveExport("ResetAPU",(void**)&m_dll.ResetAPU);
#endif

  CFile file;
  if (!file.Open(strFile))
  {
    CLog::Log(LOGERROR,"SPCCodec: error opening file %s!",strFile.c_str());
    return false;
  }
  m_szBuffer = new char[0x10200];
  if (!file.Read(m_szBuffer,0x10200))
  {
    delete[] m_szBuffer;
    m_szBuffer = NULL;
    file.Close();
    CLog::Log(LOGERROR,"SPCCodec: error reading file %s!",strFile.c_str());
    return false;
  }
  file.Close();

  m_pApuRAM = new u8[65536];
#ifdef _LINUX
  m_dll.InitAPU();
  m_dll.ResetAPU();
#endif

  m_dll.LoadSPCFile(m_szBuffer);

  m_SampleRate = 32000;
  m_Channels = 2;
  m_BitsPerSample = 16;
  m_DataFormat = AE_FMT_S16NE;
  CMusicInfoTagLoaderSPC tagLoader;
  CMusicInfoTag tag;
  tagLoader.Load(strFile,tag);
  if (tag.Loaded())
    m_TotalTime = tag.GetDuration()*1000;
  else
    m_TotalTime = 4*60*1000; // default
  m_iDataPos = 0;

  return true;
}

void SPCCodec::DeInit()
{
  if (m_loader) {
    delete m_loader;
    m_loader = NULL;
    XFILE::CFile::Delete(m_loader_name);
  }
  if (m_szBuffer)
    delete[] m_szBuffer;
  m_szBuffer = NULL;

  if (m_pApuRAM)
    delete[] m_pApuRAM;
  m_pApuRAM = NULL;
}

int64_t SPCCodec::Seek(int64_t iSeekTime)
{
  if (m_iDataPos > iSeekTime/1000*m_SampleRate*4)
  {
    m_dll.LoadSPCFile(m_szBuffer);
    m_iDataPos = iSeekTime/1000*m_SampleRate*4;
  }
  else
  {
    int64_t iDataPos2 = m_iDataPos;
    m_iDataPos = iSeekTime/1000*m_SampleRate*4;
    iSeekTime -= (iDataPos2*1000)/(m_SampleRate*4);
  }

  m_dll.SeekAPU((u32)iSeekTime*64,0);
  return (m_iDataPos*1000)/(m_SampleRate*4);
}

int SPCCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos >= m_TotalTime/1000*m_SampleRate*4)
    return READ_EOF;

  *actualsize = (int)((BYTE*)m_dll.EmuAPU(pBuffer,0,size/4)-pBuffer);
  m_iDataPos += *actualsize;

  if (*actualsize)
    return READ_SUCCESS;
  else
    return READ_ERROR;
}

bool SPCCodec::CanInit()
{
  return true;
}

