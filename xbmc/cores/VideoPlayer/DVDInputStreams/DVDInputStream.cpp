/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "DVDInputStream.h"
#include "URL.h"
#include "DVDStreamInfo.h"

CDVDInputStream::CDVDInputStream(DVDStreamType streamType, const CFileItem& fileitem)
{
  m_streamType = streamType;
  m_contentLookup = true;
  m_realtime = false;
  m_item = fileitem;
}

CDVDInputStream::~CDVDInputStream() = default;

bool CDVDInputStream::Open()
{
  m_content = m_item.GetMimeType();
  m_contentLookup = m_item.ContentLookup();
  return true;
}

void CDVDInputStream::Close()
{

}

std::string CDVDInputStream::GetFileName()
{
  CURL url(m_item.GetPath());

  url.SetProtocolOptions("");
  return url.Get();
}

CURL CDVDInputStream::GetURL()
{
  return m_item.GetURL();
}
