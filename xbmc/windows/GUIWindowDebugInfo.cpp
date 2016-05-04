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

#include "GUIWindowDebugInfo.h"
#include "settings/AdvancedSettings.h"
#include "addons/Skin.h"
#include "utils/CPUInfo.h"
#include "utils/log.h"
#include "CompileInfo.h"
#include "filesystem/SpecialProtocol.h"
#include "input/ButtonTranslator.h"
#include "guilib/GUIControlFactory.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUITextLayout.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIControlProfiler.h"
#include "GUIInfoManager.h"
#include "utils/Variant.h"
#include "utils/StringUtils.h"

#ifdef TARGET_POSIX
#include "linux/XMemUtils.h"
#endif

CGUIWindowDebugInfo::CGUIWindowDebugInfo(void)
  : CGUIDialog(WINDOW_DEBUG_INFO, "", DialogModalityType::MODELESS)
{
  m_needsScaling = false;
  m_layout = nullptr;
  m_renderOrder = RENDER_ORDER_WINDOW_DEBUG;
}

CGUIWindowDebugInfo::~CGUIWindowDebugInfo(void)
{
}

void CGUIWindowDebugInfo::UpdateVisibility()
{
  if (LOG_LEVEL_DEBUG_FREEMEM <= g_advancedSettings.m_logLevel || g_SkinInfo->IsDebugging())
    Open();
  else
    Close();
}

bool CGUIWindowDebugInfo::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    delete m_layout;
    m_layout = nullptr;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIWindowDebugInfo::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  g_graphicsContext.SetRenderingResolution(g_graphicsContext.GetResInfo(), false);

  g_cpuInfo.getUsedPercentage(); // must call it to recalculate pct values

  static int yShift = 20;
  static int xShift = 40;
  static unsigned int lastShift = time(nullptr);
  time_t now = time(nullptr);
  if (now - lastShift > 10)
  {
    yShift *= -1;
    if (now % 5 == 0)
      xShift *= -1;
    lastShift = now;
    MarkDirtyRegion();
  }

  if (!m_layout)
  {
    CGUIFont *font13 = g_fontManager.GetDefaultFont();
    CGUIFont *font13border = g_fontManager.GetDefaultFont(true);
    if (font13)
      m_layout = new CGUITextLayout(font13, true, 0, font13border);
  }
  if (!m_layout)
    return;

  std::string info;
  if (LOG_LEVEL_DEBUG_FREEMEM <= g_advancedSettings.m_logLevel)
  {
    MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&stat);
    std::string profiling = CGUIControlProfiler::IsRunning() ? " (profiling)" : "";
    std::string strCores = g_cpuInfo.GetCoresUsageString();
    std::string lcAppName = CCompileInfo::GetAppName();
    StringUtils::ToLower(lcAppName);
#if !defined(TARGET_POSIX)
    info = StringUtils::Format("LOG: %s%s.log\nMEM: %" PRIu64"/%" PRIu64" KB - FPS: %2.1f fps\nCPU: %s%s", CSpecialProtocol::TranslatePath("special://logpath").c_str(), lcAppName.c_str(),
                               stat.ullAvailPhys/1024, stat.ullTotalPhys/1024, g_infoManager.GetFPS(), strCores.c_str(), profiling.c_str());
#else
    double dCPU = m_resourceCounter.GetCPUUsage();
    std::string ucAppName = lcAppName;
    StringUtils::ToUpper(ucAppName);
    info = StringUtils::Format("LOG: %s%s.log\n" 
                                "MEM: %" PRIu64"/%" PRIu64" KB - FPS: %2.1f fps\n"
                                "CPU: %s (CPU-%s %4.2f%%%s)",
                                CSpecialProtocol::TranslatePath("special://logpath").c_str(), lcAppName.c_str(),
                                stat.ullAvailPhys/1024, stat.ullTotalPhys/1024, g_infoManager.GetFPS(),
                                strCores.c_str(), ucAppName.c_str(), dCPU, profiling.c_str());
#endif
  }

  // render the skin debug info
  if (g_SkinInfo->IsDebugging())
  {
    if (!info.empty())
      info += "\n";
    CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());
    CGUIWindow *pointer = g_windowManager.GetWindow(WINDOW_DIALOG_POINTER);
    CPoint point;
    if (pointer)
      point = CPoint(pointer->GetXPosition(), pointer->GetYPosition());
    if (window)
    {
      std::string windowName = CButtonTranslator::TranslateWindow(window->GetID());
      if (!windowName.empty())
        windowName += " (" + std::string(window->GetProperty("xmlfile").asString()) + ")";
      else
        windowName = window->GetProperty("xmlfile").asString();
      info += "Window: " + windowName + "\n";
      // transform the mouse coordinates to this window's coordinates
      g_graphicsContext.SetScalingResolution(window->GetCoordsRes(), true);
      point.x *= g_graphicsContext.GetGUIScaleX();
      point.y *= g_graphicsContext.GetGUIScaleY();
      g_graphicsContext.SetRenderingResolution(g_graphicsContext.GetResInfo(), false);
    }
    info += StringUtils::Format("Mouse: (%d,%d)  ", static_cast<int>(point.x), static_cast<int>(point.y));
    if (window)
    {
      CGUIControl *control = window->GetFocusedControl();
      if (control)
        info += StringUtils::Format("Focused: %i (%s)", control->GetID(), CGUIControlFactory::TranslateControlType(control->GetControlType()).c_str());
    }
  }

  float w, h;
  if (m_layout->Update(info))
    MarkDirtyRegion();
  m_layout->GetTextExtent(w, h);

  float x = xShift + 0.04f * g_graphicsContext.GetWidth();
  float y = yShift + 0.04f * g_graphicsContext.GetHeight();
  m_renderRegion.SetRect(x, y, x+w, y+h);
}

void CGUIWindowDebugInfo::Render()
{
  g_graphicsContext.SetRenderingResolution(g_graphicsContext.GetResInfo(), false);
  if (m_layout)
    m_layout->RenderOutline(m_renderRegion.x1, m_renderRegion.y1, 0xffffffff, 0xff000000, 0, 0);
}
