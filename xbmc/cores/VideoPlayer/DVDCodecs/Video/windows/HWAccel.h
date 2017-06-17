/*
 *      Copyright (C) 2005-2016 Team XBMC
 *      http://xbmc.org
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#define VP_VIDEOCODEC_HW

#include "DXVA.h"

IHardwareDecoder* CDVDVideoCodecFFmpeg::CreateVideoDecoderHW(AVPixelFormat pixfmt, CProcessInfo &processInfo)
{
  if (DXVA::CDecoder::Supports(pixfmt) && CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEDXVA2))
    return new DXVA::CDecoder(m_processInfo);
  return nullptr;
}
