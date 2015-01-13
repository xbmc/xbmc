/*
 *      Copyright (C) 2012-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "DVDOverlayCodec.h"
#include "cores/dvdplayer/DVDClock.h"

void CDVDOverlayCodec::GetAbsoluteTimes(double &starttime, double &stoptime, DemuxPacket *pkt, bool &replace, double offset/* = 0.0*/)
{
  if (!pkt)
    return;
  
  double duration = 0.0;
  double pts = starttime;
  
  // we assume pts from packet is better than what
  // decoder gives us, only take duration
  // from decoder if available
  if(stoptime > starttime)
    duration = stoptime - starttime;
  else if(pkt->duration != DVD_NOPTS_VALUE)
    duration = pkt->duration;
  
  if     (pkt->pts != DVD_NOPTS_VALUE)
    pts = pkt->pts;
  else if(pkt->dts != DVD_NOPTS_VALUE)
    pts = pkt->dts;
  
  starttime = pts + offset;
  if(duration)
  {
    stoptime = pts + duration + offset;
  }
  else
  {
    stoptime = 0;
    replace = true;
  }
}
