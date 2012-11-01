/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#ifndef LASTFM_SCROBBLER_H__
#define LASTFM_SCROBBLER_H__

#include "scrobbler.h"

#define LASTFM_SCROBBLER_HANDSHAKE_URL  "post.audioscrobbler.com"
#define LASTFM_SCROBBLER_LOG_PREFIX     "CLastfmScrobbler"

class CLastfmScrobbler : public CScrobbler
{
private:
  static long m_instanceLock;
  static CLastfmScrobbler *m_pInstance;
  virtual void LoadCredentials();
  virtual void NotifyUser(int error);
  virtual bool CanScrobble();
  virtual CStdString GetJournalFileName();
public:
//  CLastfmScrobbler() : CScrobbler(LASTFM_SCROBBLER_HANDSHAKE_URL, LASTFM_SCROBBLER_LOG_PREFIX) {}
//  virtual ~CLastfmScrobbler() {}
  CLastfmScrobbler();
  virtual ~CLastfmScrobbler();
  static CLastfmScrobbler *GetInstance();
  static void RemoveInstance();
};

#endif // LASTFM_SCROBBLER_H__
