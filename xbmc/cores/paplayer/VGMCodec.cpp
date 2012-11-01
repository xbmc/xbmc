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

#include "VGMCodec.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

VGMCodec::VGMCodec()
{
  m_CodecName = "VGM";
  m_vgm = 0;
  m_iDataPos = -1;
  m_DataFormat = AE_FMT_INVALID;
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

  //CStdString strFileToLoad = "filereader://"+strFile;
  XFILE::CFile::Cache(strFile,"special://temp/"+URIUtils::GetFileName(strFile));

  //m_vgm = m_dll.LoadVGM(strFileToLoad.c_str(),&m_SampleRate,&m_BitsPerSample,&m_Channels);
  m_vgm = m_dll.LoadVGM("special://temp/"+URIUtils::GetFileName(strFile),&m_SampleRate,&m_BitsPerSample,&m_Channels);
  if (!m_vgm)
  {
    CLog::Log(LOGERROR,"%s: error opening file %s!",__FUNCTION__,strFile.c_str());
    return false;
  }

  switch (m_BitsPerSample)
  {
    case  8: m_DataFormat = AE_FMT_U8   ; break;
    case 16: m_DataFormat = AE_FMT_S16NE; break;
    case 32: m_DataFormat = AE_FMT_FLOAT; break;
  }

  m_TotalTime = (int64_t)m_dll.GetLength(m_vgm);

  return true;
}

void VGMCodec::DeInit()
{
  if (m_vgm)
    m_dll.FreeVGM(m_vgm);
  m_vgm = 0;
}

int64_t VGMCodec::Seek(int64_t iSeekTime)
{
  int64_t result = (int64_t)m_dll.Seek(m_vgm,(unsigned long)iSeekTime);
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
  if (strExt == "aax" ||
      strExt == "acm" ||
      strExt == "adp" ||
      strExt == "ads" ||
      strExt == "adx" ||
      strExt == "afc" ||
      strExt == "agsc" ||
      strExt == "ahx" ||
      strExt == "aifc" ||
      strExt == "aix" ||
      strExt == "amts" ||
      strExt == "as4" ||
      strExt == "asd" ||
      strExt == "asf" ||
      strExt == "asr" ||
      strExt == "ass" ||
      strExt == "ast" ||
      strExt == "aud" ||
      strExt == "aus" ||
      strExt == "bg00" ||
      strExt == "bgw" ||
      strExt == "bh2pcm" ||
      strExt == "bmdx" ||
      strExt == "brstm" ||
      strExt == "capdsp" ||
      strExt == "ccc" ||
      strExt == "cfn" ||
      strExt == "cnk" ||
      strExt == "dcs" ||
      strExt == "de2" ||
      strExt == "dsp" ||
      strExt == "dvi" ||
      strExt == "dxh" ||
      strExt == "eam" ||
      strExt == "emff" ||
      strExt == "enth" ||
      strExt == "fag" ||
      strExt == "filp" ||
      strExt == "fsb" ||
      strExt == "gbts" ||
      strExt == "gca" ||
      strExt == "gcm" ||
      strExt == "gcw" ||
      strExt == "genh" ||
      strExt == "gms" ||
      strExt == "gsb" ||
      strExt == "hgc1" ||
      strExt == "hps" ||
      strExt == "idsp" ||
      strExt == "idvi" ||
      strExt == "ikm" ||
      strExt == "ild" ||
      strExt == "int" ||
      strExt == "isd" ||
      strExt == "ivb" ||
      strExt == "joe" ||
      strExt == "kces" ||
      strExt == "kcey" ||
      strExt == "kraw" ||
      strExt == "leg" ||
      strExt == "logg" ||
      strExt == "lwav" ||
      strExt == "matx" ||
      strExt == "mi4" ||
      strExt == "mib" ||
      strExt == "mic" ||
      strExt == "mihb" ||
      strExt == "mpdsp" ||
      strExt == "mss" ||
      strExt == "msvp" ||
      strExt == "mus" ||
      strExt == "musc" ||
      strExt == "musx" ||
      strExt == "mwv" ||
      strExt == "npsf" ||
      strExt == "nwa" ||
      strExt == "omu" ||
      strExt == "p2bt" ||
      strExt == "pcm" ||
      strExt == "pdt" ||
      strExt == "pnb" ||
      strExt == "pos" ||
      strExt == "psh" ||
      strExt == "psw" ||
      strExt == "raw" ||
      strExt == "rkv" ||
      strExt == "rnd" ||
      strExt == "rsd" ||
      strExt == "rsf" ||
      strExt == "rstm" ||
      strExt == "rwsd" ||
      strExt == "rwav" ||
      strExt == "rws" ||
      strExt == "rwsd" ||
      strExt == "rwx" ||
      strExt == "rxw" ||
      strExt == "sad" ||
      strExt == "sdt" ||
      strExt == "seg" ||
      strExt == "sfl" ||
      strExt == "sfs" ||
      strExt == "sl3" ||
      strExt == "sli" ||
      strExt == "smp" ||
      strExt == "sng" ||
      strExt == "spd" ||
      strExt == "spsd" ||
      strExt == "spw" ||
      strExt == "ss2" ||
      strExt == "ss7" ||
      strExt == "ssm" ||
      strExt == "stma" ||
      strExt == "str" ||
      strExt == "strm" ||
      strExt == "sts" ||
      strExt == "svag" ||
      strExt == "svs" ||
      strExt == "swd" ||
      strExt == "tec" ||
      strExt == "thp" ||
      strExt == "tydsp" ||
      strExt == "um3" ||
      strExt == "vag" ||
      strExt == "vas" ||
      strExt == "vgs" ||
      strExt == "vig" ||
      strExt == "vpk" ||
      strExt == "vs" ||
      strExt == "waa" ||
      strExt == "wac" ||
      strExt == "wad" ||
      strExt == "wam" ||
      strExt == "wavm" ||
      strExt == "wp2" ||
      strExt == "wsi" ||
      strExt == "wvs" ||
      strExt == "xa" ||
      strExt == "xa2" ||
      strExt == "xa30" ||
      strExt == "xmu" ||
      strExt == "xsf" ||
      strExt == "xss" ||
      strExt == "xvas" ||
      strExt == "xwav" ||
      strExt == "xwb" ||
      strExt == "ydsp" ||
      strExt == "ymf" ||
      strExt == "zwdsp"
)
    return true;

  return false;
}
