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

#include "TimidityCodec.h"
#include "../DllLoader/LibraryLoader.h"
#include "../DllLoader/SoLoader.h"
#include "../DllLoader/DllLoader.h"
#include "Util.h"
#include "utils/log.h"
#include "filesystem/SpecialProtocol.h"
#ifdef _WIN32
#include "../DllLoader/Win32DllLoader.h"
#endif

static const char * DEFAULT_SOUNDFONT_FILE = "special://masterprofile/timidity/soundfont.sf2";

TimidityCodec::TimidityCodec()
{
  m_CodecName = "MID";
  m_mid = 0;
  m_iTrack = -1;
  m_iDataPos = -1;
  m_loader = NULL;
}

TimidityCodec::~TimidityCodec()
{
  DeInit();
}

bool TimidityCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  // We do not need to init/load Timidity more than once
  //
  // Note the above comment makes no sense as TimidityCodec is not a singleton.
  // In fact, MID_CODEC can ONLY be opened and used by one instance (lot's of statics).
  // So to work around this problem with MID_CODEC, we need to make sure that
  // each instance of TimidityCodec has it's own instance of MID_CODEC. Do this by
  // coping DLL_PATH_MID_CODEC into special://temp and using a unique name. Then
  // loading this unique named MID_CODEC as the library.
  // This forces the shared lib loader to load a per-instance copy of MID_CODEC.
  if ( !m_loader )
  {
#ifdef _LINUX
    m_loader_name = CUtil::GetNextFilename("special://temp/libtimidity-%03d.so", 999);
    XFILE::CFile::Cache(DLL_PATH_MID_CODEC, m_loader_name);

    m_loader = new SoLoader(m_loader_name.c_str());
#else
    m_loader_name = CUtil::GetNextFilename("special://temp/libtimidity-%03d.dll", 999);
    XFILE::CFile::Cache(DLL_PATH_MID_CODEC, m_loader_name);

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

    m_loader->ResolveExport("DLL_Init",(void**)&m_dll.Init);
    m_loader->ResolveExport("DLL_LoadMID",(void**)&m_dll.LoadMID);
    m_loader->ResolveExport("DLL_FreeMID",(void**)&m_dll.FreeMID);
    m_loader->ResolveExport("DLL_FillBuffer",(void**)&m_dll.FillBuffer);
    m_loader->ResolveExport("DLL_GetLength",(void**)&m_dll.GetLength);
    m_loader->ResolveExport("DLL_Cleanup",(void**)&m_dll.Cleanup);
    m_loader->ResolveExport("DLL_ErrorMsg",(void**)&m_dll.ErrorMsg);
    m_loader->ResolveExport("DLL_Seek",(void**)&m_dll.Seek);

    if ( m_dll.Init( DEFAULT_SOUNDFONT_FILE ) == 0 )
    {
      CLog::Log(LOGERROR,"TimidityCodec: cannot init codec: %s", m_dll.ErrorMsg() );
      CLog::Log(LOGERROR,"Failed to initialize MIDI codec. Please make sure you configured MIDI playback according to http://wiki.xbmc.org/?title=HOW-TO:_Setup_XBMC_for_karaoke" );
      return false;
    }
  }

  // Free the song if already loaded
  if ( m_mid )
    m_dll.FreeMID( m_mid );

  CStdString file = strFile;
  CURL url(strFile);
  if (!url.IsLocal())
  {
    CStdString file = CUtil::GetNextFilename("special://temp/midi%03d.mid",999);
    XFILE::CFile::Cache(strFile,file);
    url.Parse(file);
  }

  m_mid = m_dll.LoadMID(_P(url.Get()).c_str());
  if (!m_mid)
  {
    CLog::Log(LOGERROR,"TimidityCodec: error opening file %s: %s",strFile.c_str(), m_dll.ErrorMsg());
    return false;
  }

  m_Channels = 2;
  m_SampleRate = 48000;
  m_BitsPerSample = 16;
  m_TotalTime = (__int64)m_dll.GetLength(m_mid);

  return true;
}

void TimidityCodec::DeInit()
{
  if ( m_mid )
    m_dll.FreeMID(m_mid);

  if ( m_loader )
  {
    m_dll.Cleanup();
    delete m_loader;
    XFILE::CFile::Delete(m_loader_name);
  }

  m_mid = 0;
  m_loader = 0;
}

__int64 TimidityCodec::Seek(__int64 iSeekTime)
{
  __int64 result = (__int64)m_dll.Seek(m_mid,(unsigned long)iSeekTime);
  m_iDataPos = result/1000*48000*4;

  return result;
}

int TimidityCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos == -1)
  {
    m_iDataPos = 0;
  }

  if (m_iDataPos >= m_TotalTime/1000*48000*4)
    return READ_EOF;

  if ((*actualsize=m_dll.FillBuffer(m_mid,(char*)pBuffer,size))> 0)
  {
    m_iDataPos += *actualsize;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool TimidityCodec::CanInit()
{
  return XFILE::CFile::Exists("special://xbmc/system/players/paplayer/timidity/timidity.cfg")
      || XFILE::CFile::Exists( DEFAULT_SOUNDFONT_FILE );
}

bool TimidityCodec::IsSupportedFormat(const CStdString& strExt)
{
  if (strExt == "mid" || strExt == "kar")
    return true;

  return false;
}

