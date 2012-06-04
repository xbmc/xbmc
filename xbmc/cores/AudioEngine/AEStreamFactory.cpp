/*
 *      Copyright (C) 2012 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING. If not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 */

#include "AEStreamFactory.h"
#include "AEFactory.h"
#include "utils/log.h"

IAEStream *CAEStreamFactory::MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options)
{
  if (sampleRate == 0)
  {
    CLog::Log(LOGERROR, "MakeStream: Cannot create an audio stream with zero sample rate");
    return NULL;
  }

  if (channelLayout.Count() == 0)
  {
    CLog::Log(LOGERROR, "MakeStream: Cannot create an audio stream with no channels");
    return NULL;
  }
  
  if (AE_IS_RAW(dataFormat) && encodedSampleRate == 0)
  {
    CLog::Log(LOGERROR, "MakeStream: Cannot create a passthrough audio stream without encoded sample rate");
  }
  
  return CAEFactory::AE->MakeStream(dataFormat, sampleRate, encodedSampleRate, channelLayout, options);
}

void CAEStreamFactory::FreeStream(IAEStream *stream)
{
  CAEFactory::AE->FreeStream(stream);
}
