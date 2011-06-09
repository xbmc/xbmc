/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "KaraokeSettings.h"
#include "utils/XMLUtils.h"

CKaraokeSettings::CKaraokeSettings()
{
  m_keepDelay = true;
  m_alwaysEmptyOnCdgs = true;
  m_syncDelayCDG = 0.0f;
  m_syncDelayLRC = 0.0f;
  m_changeGenreForSongs = false;
  m_startIndex = 1;
  m_useSongSpecificBackground = 0;
}

CKaraokeSettings::CKaraokeSettings(TiXmlElement *pRootElement)
{
  TiXmlElement *pElement = pRootElement->FirstChildElement("karaoke");
  if (pElement)
  {
    XMLUtils::GetFloat(pElement, "syncdelaycdg", m_syncDelayCDG, -3.0f, 3.0f); // keep the old name for comp
    XMLUtils::GetFloat(pElement, "syncdelaylrc", m_syncDelayLRC, -3.0f, 3.0f);
    XMLUtils::GetBoolean(pElement, "alwaysreplacegenre", m_changeGenreForSongs );
    XMLUtils::GetBoolean(pElement, "storedelay", m_keepDelay );
    XMLUtils::GetInt(pElement, "autoassignstartfrom", m_startIndex, 1, 2000000000);
    XMLUtils::GetBoolean(pElement, "nocdgbackground", m_alwaysEmptyOnCdgs );
    XMLUtils::GetBoolean(pElement, "lookupsongbackground", m_useSongSpecificBackground );

    TiXmlElement* pKaraokeBackground = pElement->FirstChildElement("defaultbackground");
    if (pKaraokeBackground)
    {
      const char* attr = pKaraokeBackground->Attribute("type");
      if (attr)
        m_defaultBackgroundType = attr;

      attr = pKaraokeBackground->Attribute("path");
      if (attr)
        m_defaultBackgroundFilePath = attr;
    }
  }
}

bool CKaraokeSettings::AlwaysEmptyOnCDGs()
{ 
  return m_alwaysEmptyOnCdgs; 
}

bool CKaraokeSettings::KeepDelay()
{
  return m_keepDelay;
}

bool CKaraokeSettings::ChangeGenreForSongs()
{
  return m_changeGenreForSongs;
}

bool CKaraokeSettings::UseSongSpecificBackground()
{ 
  return m_useSongSpecificBackground;
}

float CKaraokeSettings::SyncDelayCDG()
{ 
  return m_syncDelayCDG; 
}

float CKaraokeSettings::SyncDelayLRC()
{
  return m_syncDelayLRC;
}

int CKaraokeSettings::StartIndex()
{
  return m_startIndex;
}

CStdString CKaraokeSettings::DefaultBackgroundFilePath()
{
  return m_defaultBackgroundType;
}

CStdString CKaraokeSettings::DefaultBackgroundType()
{ 
  return m_defaultBackgroundFilePath;
}