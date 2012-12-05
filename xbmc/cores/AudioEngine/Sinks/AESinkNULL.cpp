/*
 *      Copyright (C) 2010-2012 Team XBMC
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

#include "system.h"

#include "AESinkNULL.h"
#include <stdint.h>
#include <limits.h>

#include "guilib/LocalizeStrings.h"
#include "dialogs/GUIDialogKaiToast.h"

#include "Utils/AEUtil.h"
#include "utils/StdString.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "settings/GUISettings.h"

CAESinkNULL::CAESinkNULL() {
}

CAESinkNULL::~CAESinkNULL()
{
}

bool CAESinkNULL::Initialize(AEAudioFormat &format, std::string &device)
{
  m_msPerFrame           = 1000.0f / format.m_sampleRate;
  m_ts                   = 0;

  format.m_dataFormat    = AE_IS_RAW(format.m_dataFormat) ? AE_FMT_S16NE : AE_FMT_FLOAT;
  format.m_frames        = format.m_sampleRate / 1000 * 500; /* 500ms */
  format.m_frameSamples  = format.m_channelLayout.Count();
  format.m_frameSize     = format.m_frameSamples * (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);

#if 0
  /* FIXME, CAUSES A DEADLOCK */
  /* display failure notification */
  CGUIDialogKaiToast::QueueNotification(
    CGUIDialogKaiToast::Error,
    g_localizeStrings.Get(34402),
    g_localizeStrings.Get(34403),
    TOAST_DISPLAY_TIME,
    false
  );
#endif

  return true;
}

void CAESinkNULL::Deinitialize()
{
}

bool CAESinkNULL::IsCompatible(const AEAudioFormat format, const std::string device)
{
  return false;
}

double CAESinkNULL::GetDelay()
{
  return std::max(0.0, (double)(m_ts - CurrentHostCounter()) / 1000000.0f);
}

unsigned int CAESinkNULL::AddPackets(uint8_t *data, unsigned int frames, bool hasAudio)
{
  float timeout = m_msPerFrame * frames;
  m_ts = CurrentHostCounter() + MathUtils::round_int(timeout * 1000000.0f);
  Sleep(MathUtils::round_int(timeout));
  return frames;
}

void CAESinkNULL::Drain()
{
}

void CAESinkNULL::EnumerateDevices (AEDeviceList &devices, bool passthrough)
{
  /* we never return any devices */
}
