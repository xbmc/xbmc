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

#include "config.h"
#include "recordingscache.h"
#include "hash.h"

cRecordingsCache::cRecordingsCache() {
}

cRecordingsCache::~cRecordingsCache() {
}

cRecordingsCache& cRecordingsCache::GetInstance() {
  static cRecordingsCache singleton;
  return singleton;
}

uint32_t cRecordingsCache::Register(cRecording* recording) {
  cString filename = recording->FileName();
  uint32_t uid = CreateStringHash(filename);

  m_mutex.Lock();
  if(m_recordings.find(uid) == m_recordings.end())
  {
    DEBUGLOG("%s - uid: %08x '%s'", __FUNCTION__, uid, (const char*)filename);
    m_recordings[uid] = filename;
  }
  m_mutex.Unlock();

  return uid;
}

cRecording* cRecordingsCache::Lookup(uint32_t uid) {
  DEBUGLOG("%s - lookup uid: %08x", __FUNCTION__, uid);

  if(m_recordings.find(uid) == m_recordings.end()) {
    DEBUGLOG("%s - not found !", __FUNCTION__);
    return NULL;
  }

  m_mutex.Lock();
  cString filename = m_recordings[uid];
  DEBUGLOG("%s - filename: %s", __FUNCTION__, (const char*)filename);

  cRecording* r = Recordings.GetByName(filename);
  DEBUGLOG("%s - recording %s", __FUNCTION__, (r == NULL) ? "not found !" : "found");
  m_mutex.Unlock();

  return r;
}
