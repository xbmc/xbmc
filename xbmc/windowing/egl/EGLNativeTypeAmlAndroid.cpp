/*
 *      Copyright (C) 2011-2014 Team XBMC
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

#include "system.h"
#include <EGL/egl.h>
#include "EGLNativeTypeAmlAndroid.h"
#include "utils/log.h"
#include "guilib/gui3d.h"
#include "platform/android/activity/XBMCApp.h"
#include "utils/StringUtils.h"
#include "utils/SysfsUtils.h"
#include "utils/AMLUtils.h"

bool CEGLNativeTypeAmlAndroid::m_isWritable = false;

bool CEGLNativeTypeAmlAndroid::CheckCompatibility()
{
  if (aml_present())
  {
    m_isWritable = false;
    if (SysfsUtils::HasRW("/sys/class/display/mode"))
      m_isWritable = true;
    else
      CLog::Log(LOGINFO, "AMLEGL: no rw on /sys/class/display/mode");
    return true;
  }
  return false;
}

bool CEGLNativeTypeAmlAndroid::GetNativeResolution(RESOLUTION_INFO *res) const
{
  CEGLNativeTypeAndroid::GetNativeResolution(&m_fb_res);

  std::string mode;
  RESOLUTION_INFO hdmi_res;
  if (SysfsUtils::GetString("/sys/class/display/mode", mode) == 0 && aml_mode_to_resolution(mode.c_str(), &hdmi_res))
  {
    m_curHdmiResolution = mode;
    *res = hdmi_res;
    res->iWidth = m_fb_res.iWidth;
    res->iHeight = m_fb_res.iHeight;
    res->iSubtitles = (int)(0.965 * res->iHeight);
  }
  else
    *res = m_fb_res;

  return true;
}

bool CEGLNativeTypeAmlAndroid::SetNativeResolution(const RESOLUTION_INFO &res)
{
  if (!m_isWritable)
    return false;

  if (res.iScreenWidth == 720 && !aml_IsHdmiConnected())
  {
    if (res.iScreenHeight == 480)
      return SetDisplayResolution("480cvbs");
    else
      return SetDisplayResolution("576cvbs");
  }

  // Don't set the same mode as current
  std::string mode;
  SysfsUtils::GetString("/sys/class/display/mode", mode);
  if (res.strId == mode)
    return false;

  return SetDisplayResolution(res.strId.c_str());
}

bool CEGLNativeTypeAmlAndroid::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  CEGLNativeTypeAndroid::GetNativeResolution(&m_fb_res);

  std::string valstr;
  if (SysfsUtils::GetString("/sys/class/amhdmitx/amhdmitx0/disp_cap", valstr) < 0)
    return false;
  std::vector<std::string> probe_str = StringUtils::Split(valstr, "\n");

  resolutions.clear();
  RESOLUTION_INFO res;
  for (size_t i = 0; i < probe_str.size(); i++)
  {
    if(aml_mode_to_resolution(probe_str[i].c_str(), &res))
    {
      res.iWidth = m_fb_res.iWidth;
      res.iHeight = m_fb_res.iHeight;
      res.iSubtitles = (int)(0.965 * res.iHeight);
      resolutions.push_back(res);
    }
  }
  return resolutions.size() > 0;

}

bool CEGLNativeTypeAmlAndroid::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return GetNativeResolution(res);
}

bool CEGLNativeTypeAmlAndroid::SetDisplayResolution(const char *resolution)
{
  if (m_curHdmiResolution == resolution)
    return true;

  // switch display resolution
  if (SysfsUtils::SetString("/sys/class/display/mode", resolution) < 0)
    return false;

  m_curHdmiResolution = resolution;

  return true;
}

