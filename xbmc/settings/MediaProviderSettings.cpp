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

#include "MediaProviderSettings.h"
#include "utils/XMLUtils.h"

CMediaProviderSettings::CMediaProviderSettings()
{
  Initialise();
}

CMediaProviderSettings::CMediaProviderSettings(TiXmlElement *pRootElement)
{
  Initialise();

  TiXmlElement *pElement = pRootElement->FirstChildElement("tuxbox");
  if (pElement)
  {
    XMLUtils::GetInt(pElement, "streamtsport", m_iTuxBoxStreamtsPort, 0, 65535);
    XMLUtils::GetBoolean(pElement, "audiochannelselection", m_bTuxBoxAudioChannelSelection);
    XMLUtils::GetBoolean(pElement, "submenuselection", m_bTuxBoxSubMenuSelection);
    XMLUtils::GetBoolean(pElement, "pictureicon", m_bTuxBoxPictureIcon);
    XMLUtils::GetBoolean(pElement, "sendallaudiopids", m_bTuxBoxSendAllAPids);
    XMLUtils::GetInt(pElement, "epgrequesttime", m_iTuxBoxEpgRequestTime, 0, 3600);
    XMLUtils::GetInt(pElement, "defaultsubmenu", m_iTuxBoxDefaultSubMenu, 1, 4);
    XMLUtils::GetInt(pElement, "defaultrootmenu", m_iTuxBoxDefaultRootMenu, 0, 4);
    XMLUtils::GetInt(pElement, "zapwaittime", m_iTuxBoxZapWaitTime, 0, 120);
    XMLUtils::GetBoolean(pElement, "zapstream", m_bTuxBoxZapstream);
    XMLUtils::GetInt(pElement, "zapstreamport", m_iTuxBoxZapstreamPort, 0, 65535);
  }
  
  pElement = pRootElement->FirstChildElement("myth");
  if (pElement)
  {
    XMLUtils::GetInt(pElement, "movielength", m_iMythMovieLength);
  }

  pElement = pRootElement->FirstChildElement("edl");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "mergeshortcommbreaks", m_bEdlMergeShortCommBreaks);
    XMLUtils::GetInt(pElement, "maxcommbreaklength", m_iEdlMaxCommBreakLength, 0, 10 * 60); // Between 0 and 10 minutes
    XMLUtils::GetInt(pElement, "mincommbreaklength", m_iEdlMinCommBreakLength, 0, 5 * 60);  // Between 0 and 5 minutes
    XMLUtils::GetInt(pElement, "maxcommbreakgap", m_iEdlMaxCommBreakGap, 0, 5 * 60);        // Between 0 and 5 minutes.
    XMLUtils::GetInt(pElement, "maxstartgap", m_iEdlMaxStartGap, 0, 10 * 60);               // Between 0 and 10 minutes
    XMLUtils::GetInt(pElement, "commbreakautowait", m_iEdlCommBreakAutowait, 0, 10);        // Between 0 and 10 seconds
    XMLUtils::GetInt(pElement, "commbreakautowind", m_iEdlCommBreakAutowind, 0, 10);        // Between 0 and 10 seconds
  }
}

int CMediaProviderSettings::TuxBoxStreamtsPort()
{
  return m_iTuxBoxStreamtsPort;
}

bool CMediaProviderSettings::TuxBoxSubMenuSelection()
{
  return m_bTuxBoxSubMenuSelection;
}

int CMediaProviderSettings::TuxBoxDefaultSubMenu()
{
  return m_iTuxBoxDefaultSubMenu;
}

void CMediaProviderSettings::SetTuxBoxDefaultRootMenu(int value)
{
  m_iTuxBoxDefaultSubMenu = value;
}

int CMediaProviderSettings::TuxBoxDefaultRootMenu()
{
  return m_iTuxBoxDefaultRootMenu;
}

bool CMediaProviderSettings::TuxBoxAudioChannelSelection()
{
  return m_bTuxBoxAudioChannelSelection;
}

bool CMediaProviderSettings::TuxBoxPictureIcon()
{
  return m_bTuxBoxPictureIcon;
}

int CMediaProviderSettings::TuxBoxEpgRequestTime()
{
  return m_iTuxBoxEpgRequestTime;
}

int CMediaProviderSettings::TuxBoxZapWaitTime()
{
  return m_iTuxBoxZapWaitTime;
}

bool CMediaProviderSettings::TuxBoxSendAllAPids()
{
  return m_bTuxBoxSendAllAPids;
}

bool CMediaProviderSettings::TuxBoxZapstream()
{
  return m_bTuxBoxZapstream;
}

int CMediaProviderSettings::TuxBoxZapstreamPort()
{
  return m_iTuxBoxZapstreamPort;
}

int CMediaProviderSettings::MythMovieLength()
{
  return m_iMythMovieLength;
}

bool CMediaProviderSettings::EdlMergeShortCommBreaks()
{
  return m_bEdlMergeShortCommBreaks;
}

int CMediaProviderSettings::EdlMaxCommBreakLength()
{
  return m_iEdlMaxCommBreakLength;
}

int CMediaProviderSettings::EdlMinCommBreakLength()
{
  return m_iEdlMinCommBreakLength;
}

int CMediaProviderSettings::EdlMaxCommBreakGap()
{
  return m_iEdlMaxCommBreakGap;
}

int CMediaProviderSettings::EdlMaxStartGap()
{
  return m_iEdlMaxStartGap;
}

int CMediaProviderSettings::EdlCommBreakAutowait()
{
  return m_iEdlCommBreakAutowait;
}

int CMediaProviderSettings::EdlCommBreakAutowind()
{
  return m_iEdlCommBreakAutowind;
}

void CMediaProviderSettings::Initialise()
{
  m_iTuxBoxStreamtsPort = 31339;
  m_bTuxBoxAudioChannelSelection = false;
  m_bTuxBoxSubMenuSelection = false;
  m_bTuxBoxPictureIcon= true;
  m_bTuxBoxSendAllAPids= false;
  m_iTuxBoxEpgRequestTime = 10; //seconds
  m_iTuxBoxDefaultSubMenu = 4;
  m_iTuxBoxDefaultRootMenu = 0; //default TV Mode
  m_iTuxBoxZapWaitTime = 0; // Time in sec. Default 0:OFF
  m_bTuxBoxZapstream = true;
  m_iTuxBoxZapstreamPort = 31344;
  m_iMythMovieLength = 0; // 0 == Off
  m_bEdlMergeShortCommBreaks = false;      // Off by default
  m_iEdlMaxCommBreakLength = 8 * 30 + 10;  // Just over 8 * 30 second commercial break.
  m_iEdlMinCommBreakLength = 3 * 30;       // 3 * 30 second commercial breaks.
  m_iEdlMaxCommBreakGap = 4 * 30;          // 4 * 30 second commercial breaks.
  m_iEdlMaxStartGap = 5 * 60;              // 5 minutes.
  m_iEdlCommBreakAutowait = 0;             // Off by default
  m_iEdlCommBreakAutowind = 0;             // Off by default
}