/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2011 Alexander Pipelka
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

#ifndef VNSI_RECORDINGSCACHE_H
#define VNSI_RECORDINGSCACHE_H

#include <stdint.h>
#include <map>
#include <vdr/thread.h>
#include <vdr/tools.h>
#include <vdr/recording.h>

class cRecordingsCache
{
protected:

  cRecordingsCache();

  virtual ~cRecordingsCache();

public:

  static cRecordingsCache& GetInstance();

  uint32_t Register(cRecording* recording);

  cRecording* Lookup(uint32_t uid);

private:

  std::map<uint32_t, cString> m_recordings;

  cMutex m_mutex;
};


#endif // VNSI_RECORDINGSCACHE_H
