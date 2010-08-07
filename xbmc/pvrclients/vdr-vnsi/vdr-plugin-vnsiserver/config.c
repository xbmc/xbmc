/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <vdr/plugin.h>

#include "config.h"

cVNSIServerConfig::cVNSIServerConfig()
{
  memset(this, 0, sizeof(cVNSIServerConfig));

  listen_port         = LISTEN_PORT;
  SuspendMode         = smAlways;
  ConfigDirectory     = NULL;
}

void cVNSIServerConfig::readNoSignalStream()
{
  m_noSignalStreamSize = 0;

  cString noSignalFileName = cString::sprintf("%s/"NO_SIGNAL_FILE, *ConfigDirectory);

  FILE *const f = fopen(*noSignalFileName, "rb");
  if (f)
  {
    m_noSignalStreamSize = fread(&m_noSignalStreamData[0] + 9, 1, sizeof (m_noSignalStreamData) - 9 - 9 - 4, f);
    if (m_noSignalStreamSize == sizeof (m_noSignalStreamData) - 9 - 9 - 4)
    {
      esyslog("VNSI-Error: '%s' exeeds limit of %ld bytes!", *noSignalFileName, (long)(sizeof (m_noSignalStreamData) - 9 - 9 - 4 - 1));
    }
    else if (m_noSignalStreamSize > 0)
    {
      m_noSignalStreamData[ 0 ] = 0x00;
      m_noSignalStreamData[ 1 ] = 0x00;
      m_noSignalStreamData[ 2 ] = 0x01;
      m_noSignalStreamData[ 3 ] = 0xe0;
      m_noSignalStreamData[ 4 ] = (m_noSignalStreamSize + 3) >> 8;
      m_noSignalStreamData[ 5 ] = (m_noSignalStreamSize + 3) & 0xff;
      m_noSignalStreamData[ 6 ] = 0x80;
      m_noSignalStreamData[ 7 ] = 0x00;
      m_noSignalStreamData[ 8 ] = 0x00;
      m_noSignalStreamSize += 9;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x01;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0xe0;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x07;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x80;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x01;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0xb7;
    }
    fclose(f);
    return;
  }
  else
  {
    esyslog("VNSI-Error: couldn't open '%s'!", *noSignalFileName);
  }

  return;
}

/* Global instance */
cVNSIServerConfig VNSIServerConfig;
