/*
 *      Copyright (C) 2016 Canonical Ltd.
 *      brandon.schaefer@canonical.com
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


#include "WinSystemMir.h"

#include <string.h>

#include "guilib/GraphicContext.h"
#include "settings/DisplaySettings.h"
#include "utils/log.h"
#include "WinEventsMir.h"

CWinSystemMir::CWinSystemMir() :
  m_connection(nullptr),
  m_surface(nullptr),
  m_pixel_format(mir_pixel_format_invalid)
{
  m_eWindowSystem = WINDOW_SYSTEM_MIR;
}

bool CWinSystemMir::InitWindowSystem()
{
  m_connection = mir_connect_sync(nullptr, __PRETTY_FUNCTION__);
  if (!mir_connection_is_valid(m_connection))
  {
    CLog::Log(LOGERROR, "WinSystemMir::InitWindowSystem - %s",
      mir_connection_get_error_message(m_connection));

    return false;
  }

  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemMir::DestroyWindowSystem()
{
  mir_connection_release(m_connection);

  return true;
}

bool CWinSystemMir::CreateNewWindow(const std::string& name,
                                    bool fullScreen,
                                    RESOLUTION_INFO& res,
                                    PHANDLE_EVENT_FUNC userFunction)
{
  if (m_pixel_format == mir_pixel_format_invalid)
  {
    CLog::Log(LOGERROR, "WinSystemMir::CreateNewWindow - invalid pixel format");
    return false;
  }

  auto spec = mir_connection_create_spec_for_normal_surface(m_connection,
                                                            res.iWidth,
                                                            res.iHeight,
                                                            m_pixel_format);

  mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
  mir_surface_spec_set_name(spec, name.c_str());

  m_surface = mir_surface_create_sync(spec);
  mir_surface_spec_release(spec);

  if (!mir_surface_is_valid(m_surface))
  {
    CLog::Log(LOGERROR, "WinSystemMir::CreateNewWindow - %s",
      mir_surface_get_error_message(m_surface));

    return false;
  }

  mir_surface_set_event_handler(m_surface, MirHandleEvent, NULL);

  // Hide the cursor
  mir_surface_configure_cursor(m_surface, NULL);

  return true;
}

bool CWinSystemMir::DestroyWindow()
{
  mir_surface_release_sync(m_surface);

  return true;
}

void CWinSystemMir::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  auto display_config = mir_connection_create_display_configuration(m_connection);
  uint32_t num_outputs     = mir_display_config_get_num_outputs(display_config);

  for (uint32_t d = 0; d < num_outputs; d++)
  {
    auto output  = mir_display_config_get_output(display_config, d);
    auto state   = mir_output_get_connection_state(output);
    bool enabled = mir_output_is_enabled(output);

    if (enabled && state == mir_output_connection_state_connected)
    {
      auto mode   = mir_output_get_mode(output, d);
      auto width  = mir_output_mode_get_width(mode);
      auto height = mir_output_mode_get_height(mode);
      auto refresh_rate = mir_output_mode_get_refresh_rate(mode);

      UpdateDesktopResolution(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP), 0, width, height, refresh_rate);
      CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP).strId = std::to_string(d);
      SetWindowResolution(width, height);
    }
  }
  CDisplaySettings::GetInstance().ClearCustomResolutions();
  CDisplaySettings::GetInstance().ApplyCalibrations();
}

bool CWinSystemMir::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  auto spec = mir_connection_create_spec_for_changes(m_connection);
  mir_surface_spec_set_width (spec, newWidth);
  mir_surface_spec_set_height(spec, newHeight);

  mir_surface_apply_spec(m_surface, spec);
  mir_surface_spec_release(spec);

  return true;
}

bool CWinSystemMir::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  auto state = mir_surface_state_fullscreen;
  if (!fullScreen)
  {
    state = mir_surface_state_restored;
  }

  auto spec = mir_connection_create_spec_for_changes(m_connection);
  mir_surface_spec_set_state(spec, state);

  mir_surface_apply_spec(m_surface, spec);
  mir_surface_spec_release(spec);

  m_nWidth      = res.iWidth;
  m_nHeight     = res.iHeight;
  m_bFullScreen = fullScreen;

  ResizeWindow(res.iWidth, res.iHeight, 0, 0);

  return true;
}

bool CWinSystemMir::Hide()
{
  return false;
}

bool CWinSystemMir::Show(bool raise)
{
  return true;
}

void CWinSystemMir::Register(IDispResource * /*resource*/)
{
}

void CWinSystemMir::Unregister(IDispResource * /*resource*/)
{
}
