/*
*      Copyright (C) 2017 Team Kodi
*      https://kodi.tv
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

#ifndef ODBSTREAMDETAILS_H
#define ODBSTREAMDETAILS_H

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

#include <string>

#include "ODBFile.h"

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("stream_details"))
class CODBStreamDetails
{
public:
  CODBStreamDetails()
  {
    m_streamType = 0;
    m_videoCodec = "";
    m_videoAspect = 0;
    m_videoWidth = 0;
    m_videoHeight = 0;
    m_audioCodec = "";
    m_audioChannels = 0;
    m_audioLanguage = "";
    m_subtitleLanguage = "";
    m_videoDuration = 0;
    m_stereoMode = "";
    m_videoLanguage = "";
    m_synced = false;
  };
  
PRAGMA_DB (id auto)
  unsigned long m_idStreamDetail;
  odb::lazy_shared_ptr<CODBFile> m_file;
  int m_streamType;
  std::string m_videoCodec;
  float m_videoAspect;
  int m_videoWidth;
  int m_videoHeight;
  std::string m_audioCodec;
  int m_audioChannels;
  std::string m_audioLanguage;
  std::string m_subtitleLanguage;
  int m_videoDuration;
  std::string m_stereoMode;
  std::string m_videoLanguage;
  
  //Members not stored in the db, used for sync ...
PRAGMA_DB (transient)
  bool m_synced;
  
private:
  friend class odb::access;
  
PRAGMA_DB (index member(m_file))
PRAGMA_DB (index member(m_streamType))
  
};

#endif /* ODBSTREAMDETAILS_H */
