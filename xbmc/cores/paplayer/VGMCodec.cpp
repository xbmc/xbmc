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
#include "VGMCodec.h"

VGMCodec::VGMCodec()
{
  m_CodecName = "VGM";
  m_vgm = 0;
  m_iDataPos = -1; 
}

VGMCodec::~VGMCodec()
{
  DeInit();
}

bool VGMCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false; // error logged previously
  
  m_dll.Init();

  CStdString strFileToLoad = "filereader://"+strFile;

  m_vgm = m_dll.LoadVGM(strFileToLoad.c_str(),&m_SampleRate,&m_BitsPerSample,&m_Channels);
  if (!m_vgm)
  {
    CLog::Log(LOGERROR,"%s: error opening file %s!",__FUNCTION__,strFile.c_str());
    return false;
  }
  
  m_TotalTime = (__int64)m_dll.GetLength(m_vgm);

  return true;
}

void VGMCodec::DeInit()
{
  if (m_vgm)
    m_dll.FreeVGM(m_vgm);
  m_vgm = 0;
}

__int64 VGMCodec::Seek(__int64 iSeekTime)
{
  __int64 result = (__int64)m_dll.Seek(m_vgm,(unsigned long)iSeekTime);
  m_iDataPos = result/1000*m_SampleRate*m_BitsPerSample*m_Channels/8;
  
  return result;
}

int VGMCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos == -1)
  {
    m_iDataPos = 0;
  }

  if (m_iDataPos >= m_TotalTime/1000*m_SampleRate*m_BitsPerSample*m_Channels/8)
  {
    return READ_EOF;
  }
  
  if ((*actualsize=m_dll.FillBuffer(m_vgm,(char*)pBuffer,size))> 0)
  {
    m_iDataPos += *actualsize;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool VGMCodec::CanInit()
{
  return m_dll.CanLoad();
}

bool VGMCodec::IsSupportedFormat(const CStdString& strExt)
{
  if (strExt == "afc"   || strExt == "agsc" || strExt == "amts"  || strExt == "adp"  ||
      strExt == "cfn"   || strExt == "dsp"  || strExt == "gcm"   || strExt == "hps"  || 
      strExt == "mpdsp" || strExt == "mss"  || strExt == "sad"   || strExt == "stm"  || 
      strExt == "str"   || strExt == "ast"  || strExt == "brstm" || strExt == "wsi"  || 
      strExt == "rwsd"  || strExt == "strm" || strExt == "xa"    || strExt == "ads"  || 
      strExt == "ss2"   || strExt == "bmdx" || strExt == "gms"   || strExt == "ild"  || 
      strExt == "mib"   || strExt == "mi4"  || strExt == "mih"   || strExt == "mic"  || 
      strExt == "npsf"  || strExt == "pnb"  || strExt == "rxw"   || strExt == "sts"  || 
      strExt == "svag"  || strExt == "str+" || strExt == "sth"   || strExt == "sng"  || 
      strExt == "asf"   || strExt == "eam"  || strExt == "vag"   || strExt == "vpk"  || 
      strExt == "wp2"   || strExt == "aus"  || strExt == "bg00"  || strExt == "cnk"  || 
      strExt == "filp"  || strExt == "hgc1" || strExt == "ikm"   || strExt == "ivb"  || 
      strExt == "leg"   || strExt == "musc" || strExt == "musx"  || strExt == "psh"  || 
      strExt == "rstm"  || strExt == "rws"  || strExt == "sfs"   || strExt == "sl3"  || 
      strExt == "svs"   || strExt == "vig"  || strExt == "xa30"  || strExt == "wavm" || 
      strExt == "xwav"  || strExt == "xwb"  || strExt == "fsb"   || strExt == "genh" || 
      strExt == "rsd"   || strExt == "acm"  || strExt == "adx"   || strExt == "aiff" ||  
      strExt == "aifc"  || strExt == "ahx"  || strExt == "as4"   || strExt == "aud"  || 
      strExt == "dvi"   || strExt == "kcey" || strExt == "rsf"   || strExt == "gcsw" || 
      strExt == "int"   || strExt == "nwa"  || strExt == "raw"   || strExt == "rwx"  || 
      strExt == "xss"   || strExt == "lwav" || strExt == "pos"   || strExt == "logg" ||
      strExt == "sli"   || strExt == "sfl"   || strExt == "um3")
    return true;
  
  return false;
}

