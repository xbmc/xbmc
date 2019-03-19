/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDCodecUtils.h"
#include "cores/VideoPlayer/Interface/Addon/TimingConstants.h"
#include "cores/FFmpeg.h"
#include "Util.h"

extern "C" {
#include <libswscale/swscale.h>
}

bool CDVDCodecUtils::IsVP3CompatibleWidth(int width)
{
  // known hardware limitation of purevideo 3 (VP3). (the Nvidia 9400 is a purevideo 3 chip)
  // from nvidia's linux vdpau README: All current third generation PureVideo hardware
  // (G98, MCP77, MCP78, MCP79, MCP7A) cannot decode H.264 for the following horizontal resolutions:
  // 769-784, 849-864, 929-944, 1009–1024, 1793–1808, 1873–1888, 1953–1968 and 2033-2048 pixel.
  // This relates to the following macroblock sizes.
  int unsupported[] = {49, 54, 59, 64, 113, 118, 123, 128};
  for (int u : unsupported)
  {
    if (u == (width + 15) / 16)
      return false;
  }
  return true;
}

double CDVDCodecUtils::NormalizeFrameduration(double frameduration, bool *match)
{
  //if the duration is within 20 microseconds of a common duration, use that
  const double durations[] = {DVD_TIME_BASE * 1.001 / 24.0, DVD_TIME_BASE / 24.0, DVD_TIME_BASE / 25.0,
                              DVD_TIME_BASE * 1.001 / 30.0, DVD_TIME_BASE / 30.0, DVD_TIME_BASE / 50.0,
                              DVD_TIME_BASE * 1.001 / 60.0, DVD_TIME_BASE / 60.0};

  double lowestdiff = DVD_TIME_BASE;
  int    selected   = -1;
  for (size_t i = 0; i < ARRAY_SIZE(durations); i++)
  {
    double diff = fabs(frameduration - durations[i]);
    if (diff < DVD_MSEC_TO_TIME(0.02) && diff < lowestdiff)
    {
      selected = i;
      lowestdiff = diff;
    }
  }

  if (selected != -1)
  {
    if (match)
      *match = true;
    return durations[selected];
  }
  else
  {
    if (match)
      *match = false;
    return frameduration;
  }
}

