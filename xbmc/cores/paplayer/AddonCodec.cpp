/*
 *      Copyright (C) 2009 Team XBMC
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

#include "AddonCodec.h"
#include "utils/log.h"

#include <cstdlib>
#include <cstdio>

using namespace ADDON;
using namespace XFILE;

AddonCodec::AddonCodec(AudioCodecPtr codec) :
  m_codec(codec)
{
  m_CodecName = "XXX";
  m_info = NULL;
}

AddonCodec::~AddonCodec()
{
  //DeInit();
}

bool AddonCodec::Init(const CStdString &strFile, unsigned int filecache)
{
//  DeInit();
  if ((m_info=m_codec->Init(strFile.c_str())))
  {
    m_Channels = m_info->channels;
    m_SampleRate = m_info->samplerate;
    m_BitsPerSample = m_info->bitrate;
    m_TotalTime = m_info->totaltime;
    m_CodecName = m_info->name;
  }
  return (m_info!=NULL);
}

void AddonCodec::DeInit()
{
  m_codec->DeInit(m_info);
}

__int64 AddonCodec::Seek(__int64 iSeekTime)
{
  return m_codec->Seek(m_info,iSeekTime);
}

int AddonCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  return m_codec->ReadPCM(m_info,pBuffer,(unsigned int)size,(unsigned int*)actualsize);
}

bool AddonCodec::CanInit()
{
  return true;
}
