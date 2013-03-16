/*
 *      Copyright (C) 2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "DisplaySettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

using namespace std;

static RESOLUTION_INFO EmptyResolution;
static RESOLUTION_INFO EmptyModifiableResolution;

CDisplaySettings::CDisplaySettings()
{
  m_resolutions.insert(m_resolutions.begin(), RES_CUSTOM, RESOLUTION_INFO());
}

CDisplaySettings::~CDisplaySettings()
{ }

CDisplaySettings& CDisplaySettings::Get()
{
  static CDisplaySettings sDisplaySettings;
  return sDisplaySettings;
}

bool CDisplaySettings::Load(const TiXmlNode *settings)
{
  CSingleLock lock(m_critical);
  m_calibrations.clear();

  if (settings == NULL)
    return false;

  const TiXmlElement *pElement = settings->FirstChildElement("resolutions");
  if (!pElement)
  {
    CLog::Log(LOGERROR, "CDisplaySettings: settings file doesn't contain <resolutions>");
    return false;
  }

  const TiXmlElement *pResolution = pElement->FirstChildElement("resolution");
  while (pResolution)
  {
    // get the data for this calibration
    RESOLUTION_INFO cal;

    XMLUtils::GetString(pResolution, "description", cal.strMode);
    XMLUtils::GetInt(pResolution, "subtitles", cal.iSubtitles);
    XMLUtils::GetFloat(pResolution, "pixelratio", cal.fPixelRatio);
#ifdef HAS_XRANDR
    XMLUtils::GetFloat(pResolution, "refreshrate", cal.fRefreshRate);
    XMLUtils::GetString(pResolution, "output", cal.strOutput);
    XMLUtils::GetString(pResolution, "xrandrid", cal.strId);
#endif

    const TiXmlElement *pOverscan = pResolution->FirstChildElement("overscan");
    if (pOverscan)
    {
      XMLUtils::GetInt(pOverscan, "left", cal.Overscan.left);
      XMLUtils::GetInt(pOverscan, "top", cal.Overscan.top);
      XMLUtils::GetInt(pOverscan, "right", cal.Overscan.right);
      XMLUtils::GetInt(pOverscan, "bottom", cal.Overscan.bottom);
    }

    // mark calibration as not updated
    // we must not delete those, resolution just might not be available
    cal.iWidth = cal.iHeight = 0;

    // store calibration, avoid adding duplicates
    bool found = false;
    for (ResolutionInfos::const_iterator  it = m_calibrations.begin(); it != m_calibrations.end(); ++it)
    {
      if (it->strMode.Equals(cal.strMode))
      {
        found = true;
        break;
      }
    }
    if (!found)
      m_calibrations.push_back(cal);

    // iterate around
    pResolution = pResolution->NextSiblingElement("resolution");
  }

  ApplyCalibrations();
  return true;
}

bool CDisplaySettings::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  CSingleLock lock(m_critical);
  TiXmlElement xmlRootElement("resolutions");
  TiXmlNode *pRoot = settings->InsertEndChild(xmlRootElement);
  if (pRoot == NULL)
    return false;

  // save calibrations
  for (ResolutionInfos::const_iterator it = m_calibrations.begin(); it != m_calibrations.end(); ++it)
  {
    // Write the resolution tag
    TiXmlElement resElement("resolution");
    TiXmlNode *pNode = pRoot->InsertEndChild(resElement);
    if (pNode == NULL)
      return false;

    // Now write each of the pieces of information we need...
    XMLUtils::SetString(pNode, "description", it->strMode);
    XMLUtils::SetInt(pNode, "subtitles", it->iSubtitles);
    XMLUtils::SetFloat(pNode, "pixelratio", it->fPixelRatio);
#ifdef HAS_XRANDR
    XMLUtils::SetFloat(pNode, "refreshrate", it->fRefreshRate);
    XMLUtils::SetString(pNode, "output", it->strOutput);
    XMLUtils::SetString(pNode, "xrandrid", it->strId);
#endif

    // create the overscan child
    TiXmlElement overscanElement("overscan");
    TiXmlNode *pOverscanNode = pNode->InsertEndChild(overscanElement);
    if (pOverscanNode == NULL)
      return false;

    XMLUtils::SetInt(pOverscanNode, "left", it->Overscan.left);
    XMLUtils::SetInt(pOverscanNode, "top", it->Overscan.top);
    XMLUtils::SetInt(pOverscanNode, "right", it->Overscan.right);
    XMLUtils::SetInt(pOverscanNode, "bottom", it->Overscan.bottom);
  }

  return true;
}

void CDisplaySettings::Clear()
{
  CSingleLock lock(m_critical);
  m_calibrations.clear();
  m_resolutions.clear();
}

const RESOLUTION_INFO& CDisplaySettings::GetResolutionInfo(size_t index) const
{
  CSingleLock lock(m_critical);
  if (index >= m_resolutions.size())
    return EmptyResolution;

  return m_resolutions[index];
}

const RESOLUTION_INFO& CDisplaySettings::GetResolutionInfo(RESOLUTION resolution) const
{
  if (resolution <= RES_INVALID)
    return EmptyResolution;

  return GetResolutionInfo((size_t)resolution);
}

RESOLUTION_INFO& CDisplaySettings::GetResolutionInfo(size_t index)
{
  CSingleLock lock(m_critical);
  if (index >= m_resolutions.size())
  {
    EmptyModifiableResolution = RESOLUTION_INFO();
    return EmptyModifiableResolution;
  }

  return m_resolutions[index];
}

RESOLUTION_INFO& CDisplaySettings::GetResolutionInfo(RESOLUTION resolution)
{
  if (resolution <= RES_INVALID)
  {
    EmptyModifiableResolution = RESOLUTION_INFO();
    return EmptyModifiableResolution;
  }

  return GetResolutionInfo((size_t)resolution);
}

void CDisplaySettings::AddResolutionInfo(const RESOLUTION_INFO &resolution)
{
  CSingleLock lock(m_critical);
  m_resolutions.push_back(resolution);
}

void CDisplaySettings::ApplyCalibrations()
{
  CSingleLock lock(m_critical);
  // apply all calibrations to the resolutions
  for (ResolutionInfos::const_iterator itCal = m_calibrations.begin(); itCal != m_calibrations.end(); ++itCal)
  {
    // find resolutions
    for (size_t res = 0; res < m_resolutions.size(); ++res)
    {
      if (res == RES_WINDOW)
        continue;
      if (itCal->strMode.Equals(m_resolutions[res].strMode))
      {
        // overscan
        m_resolutions[res].Overscan.left = itCal->Overscan.left;
        if (m_resolutions[res].Overscan.left < -m_resolutions[res].iWidth/4)
          m_resolutions[res].Overscan.left = -m_resolutions[res].iWidth/4;
        if (m_resolutions[res].Overscan.left > m_resolutions[res].iWidth/4)
          m_resolutions[res].Overscan.left = m_resolutions[res].iWidth/4;

        m_resolutions[res].Overscan.top = itCal->Overscan.top;
        if (m_resolutions[res].Overscan.top < -m_resolutions[res].iHeight/4)
          m_resolutions[res].Overscan.top = -m_resolutions[res].iHeight/4;
        if (m_resolutions[res].Overscan.top > m_resolutions[res].iHeight/4)
          m_resolutions[res].Overscan.top = m_resolutions[res].iHeight/4;

        m_resolutions[res].Overscan.right = itCal->Overscan.right;
        if (m_resolutions[res].Overscan.right < m_resolutions[res].iWidth / 2)
          m_resolutions[res].Overscan.right = m_resolutions[res].iWidth / 2;
        if (m_resolutions[res].Overscan.right > m_resolutions[res].iWidth * 3/2)
          m_resolutions[res].Overscan.right = m_resolutions[res].iWidth *3/2;

        m_resolutions[res].Overscan.bottom = itCal->Overscan.bottom;
        if (m_resolutions[res].Overscan.bottom < m_resolutions[res].iHeight / 2)
          m_resolutions[res].Overscan.bottom = m_resolutions[res].iHeight / 2;
        if (m_resolutions[res].Overscan.bottom > m_resolutions[res].iHeight * 3/2)
          m_resolutions[res].Overscan.bottom = m_resolutions[res].iHeight * 3/2;

        m_resolutions[res].iSubtitles = itCal->iSubtitles;
        if (m_resolutions[res].iSubtitles < m_resolutions[res].iHeight / 2)
          m_resolutions[res].iSubtitles = m_resolutions[res].iHeight / 2;
        if (m_resolutions[res].iSubtitles > m_resolutions[res].iHeight* 5/4)
          m_resolutions[res].iSubtitles = m_resolutions[res].iHeight* 5/4;

        m_resolutions[res].fPixelRatio = itCal->fPixelRatio;
        if (m_resolutions[res].fPixelRatio < 0.5f)
          m_resolutions[res].fPixelRatio = 0.5f;
        if (m_resolutions[res].fPixelRatio > 2.0f)
          m_resolutions[res].fPixelRatio = 2.0f;
        break;
      }
    }
  }
}

void CDisplaySettings::UpdateCalibrations()
{
  CSingleLock lock(m_critical);
  for (size_t res = RES_DESKTOP; res < m_resolutions.size(); ++res)
  {
    // find calibration
    bool found = false;
    for (ResolutionInfos::iterator itCal = m_calibrations.begin(); itCal != m_calibrations.end(); ++itCal)
    {
      if (itCal->strMode.Equals(m_resolutions[res].strMode))
      {
        // TODO: erase calibrations with default values
        *itCal = m_resolutions[res];
        found = true;
        break;
      }
    }

    if (!found)
      m_calibrations.push_back(m_resolutions[res]);
  }
}
