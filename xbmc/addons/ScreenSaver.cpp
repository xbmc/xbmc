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

#include "ScreenSaver.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GraphicContext.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"

namespace ADDON
{

CScreenSaver::CScreenSaver(BinaryAddonBasePtr addonBase)
 : IAddonInstanceHandler(ADDON_INSTANCE_SCREENSAVER, addonBase)
{
  m_name = Name();
  m_presets = CSpecialProtocol::TranslatePath(Path());
  m_profile = CSpecialProtocol::TranslatePath(Profile());

  m_struct = {{0}};
#ifdef HAS_DX
  m_struct.props.device = g_Windowing.Get3D11Context();
#else
  m_struct.props.device = nullptr;
#endif
  m_struct.props.x = 0;
  m_struct.props.y = 0;
  m_struct.props.width = g_graphicsContext.GetWidth();
  m_struct.props.height = g_graphicsContext.GetHeight();
  m_struct.props.pixelRatio = g_graphicsContext.GetResInfo().fPixelRatio;
  m_struct.props.name = m_name.c_str();
  m_struct.props.presets = m_presets.c_str();
  m_struct.props.profile = m_profile.c_str();

  m_struct.toKodi.kodiInstance = this;

  /* Open the class "kodi::addon::CInstanceScreensaver" on add-on side */
  if (CreateInstance(&m_struct) != ADDON_STATUS_OK)
    CLog::Log(LOGFATAL, "Screensaver: failed to create instance for '%s' and not usable!", ID().c_str());
}

CScreenSaver::~CScreenSaver()
{
  /* Destroy the class "kodi::addon::CInstanceScreensaver" on add-on side */
  DestroyInstance();
}

bool CScreenSaver::Start()
{
  if (m_struct.toAddon.Start)
    return m_struct.toAddon.Start(&m_struct);
  return false;
}

void CScreenSaver::Stop()
{
  if (m_struct.toAddon.Stop)
    m_struct.toAddon.Stop(&m_struct);
}

void CScreenSaver::Render()
{
  if (m_struct.toAddon.Render)
    m_struct.toAddon.Render(&m_struct);
}

} /* namespace ADDON */
