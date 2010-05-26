/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "RendererSettings.h"
#include "file.h"
#include "log.h"
#include "util.h"
#include "Settings.h"
#include "GuiSettings.h"
#include "XMLUtils.h"

using namespace XFILE;
CDsSettings::CDsSettings(void)
{
  m_hD3DX9Dll = NULL;
}

CDsSettings::~CDsSettings(void)
{
  if (m_hD3DX9Dll)
    FreeLibrary(m_hD3DX9Dll);
}

void CDsSettings::SetDefault()
{
  m_RenderSettings.SetDefault();

  iAPSurfaceUsage = VIDRNDT_AP_TEXTURE3D; //This one is required to work with xbmc gui
  fVMR9MixerMode = true;
  nSPCSize = 3; // Threading queue
  nSPCMaxRes = 0; // default
  fSPCAllowAnimationWhenBuffering = false; // No Anim
  iDX9Resizer = 1; //Dx9 Resizers
  m_fDisplayStats = 0;
}

void CDsSettings::LoadConfig()
{
  SetDefault();

  CStdString strDsConfigFile = g_settings.GetUserDataItem("dsconfig.xml");
  if (!CFile::Exists(strDsConfigFile))
  {
    CLog::Log(LOGNOTICE, "No dsconfig.xml to load (%s)", strDsConfigFile.c_str());
    return;
  }
  
  // load the xml file
  TiXmlDocument xmlDoc;

  if (!xmlDoc.LoadFile(strDsConfigFile))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", strDsConfigFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (pRootElement && (strcmpi(pRootElement->Value(), "dsplayersettings") != 0))
  {
    CLog::Log(LOGERROR, "Error loading %s, no <dsplayersettings> node", strDsConfigFile.c_str());
    return;
  }

  // video renderer settings
  TiXmlElement *pElement = pRootElement->FirstChildElement("videorenderers");
  if (! pElement)
    return;

  // TODO: Rename and document those options. Don't need the VMR9 prefix as these applied to EVR too

  // Default values are set by SetDefault. We don't need a default parameter in GetXXX.
  // If XMLUtils::GetXXX fails, the value is not modified

  // VSync
  XMLUtils::GetBoolean(pElement, "VMR9VSync", m_RenderSettings.iVMR9VSync);
  XMLUtils::GetBoolean(pElement, "VMR9AlterativeVSync", m_RenderSettings.fVMR9AlterativeVSync);
  XMLUtils::GetBoolean(pElement, "VMR9VSyncAccurate", m_RenderSettings.iVMR9VSyncAccurate);
  XMLUtils::GetBoolean(pElement, "VMRFlushGPUBeforeVSync", m_RenderSettings.iVMRFlushGPUBeforeVSync);
  XMLUtils::GetBoolean(pElement, "VMRFlushGPUWait", m_RenderSettings.iVMRFlushGPUWait);
  XMLUtils::GetBoolean(pElement, "VMRFlushGPUAfterPresent", m_RenderSettings.iVMRFlushGPUAfterPresent);
  XMLUtils::GetInt(pElement, "VMR9VSyncOffset", m_RenderSettings.iVMR9VSyncOffset, 0, 100);
  
  // Misc
  XMLUtils::GetBoolean(pElement, "VMRDisableDesktopComposition", m_RenderSettings.iVMRDisableDesktopComposition);  
  XMLUtils::GetBoolean(pElement, "SynchronizeVideo", m_RenderSettings.bSynchronizeVideo);
  XMLUtils::GetBoolean(pElement, "SynchronizeDisplay", m_RenderSettings.bSynchronizeDisplay);
  XMLUtils::GetBoolean(pElement, "SynchronizeNearest", m_RenderSettings.bSynchronizeNearest);
  XMLUtils::GetInt(pElement, "LineDelta", m_RenderSettings.iLineDelta, 0, 100);
  XMLUtils::GetInt(pElement, "ColumnDelta", m_RenderSettings.iColumnDelta, 0, 100);
  XMLUtils::GetFloat(pElement, "CycleDelta", m_RenderSettings.fCycleDelta, 0, 100);
  XMLUtils::GetFloat(pElement, "TargetSyncOffset", m_RenderSettings.fTargetSyncOffset, 0, 100);
  XMLUtils::GetFloat(pElement, "ControlLimit", m_RenderSettings.fControlLimit, 0, 100);

  // EVR
  XMLUtils::GetInt(pElement, "EVRBuffers", m_RenderSettings.iEvrBuffers, 4, 60);
}

void CDsSettings::CRendererSettingsShared::SetDefault()
{
  fVMR9AlterativeVSync = false; // Alternative VSync
  iVMR9VSyncOffset = 0; // Vsync Offset
  //The accurate vsync to 1 with dxva is making the video totaly out of sync
  iVMR9VSyncAccurate = true; // Accurate VSync
  iVMR9FullscreenGUISupport = true; //Needed for filters property page
  iVMR9VSync = true; //Vsync
  iVMRDisableDesktopComposition = false; //Disable desktop composition (Aero)

  iVMRFlushGPUBeforeVSync = true; //Flush GPU before VSync
  iVMRFlushGPUAfterPresent = false; //Flush GPU after Present
  iVMRFlushGPUWait = false; //Wait for flushes

  bSynchronizeVideo = false;
  bSynchronizeDisplay = false;
  bSynchronizeNearest = true;

  iLineDelta = 0;
  iColumnDelta = 0;
  fCycleDelta = 0.0012f; //Frequency adjustement
  fTargetSyncOffset = 12.0f; //Target sync offset
  fControlLimit = 2.0f; //control limits + - ms
}

void CDsSettings::CRendererSettingsShared::SetOptimal()
{
  fVMR9AlterativeVSync = true;
  iVMR9VSyncAccurate = true;
  iVMR9VSync = true;
  iVMRDisableDesktopComposition = true;
  iVMRFlushGPUBeforeVSync = true;
  iVMRFlushGPUAfterPresent = true;
  iVMRFlushGPUWait = false;
  bSynchronizeVideo = false;
  bSynchronizeDisplay = false;
  bSynchronizeNearest = true;
  iLineDelta = 0;
  iColumnDelta = 0;
  fCycleDelta = 0.0012f;
  fTargetSyncOffset = 12.0f;
  fControlLimit = 2.0f;
}
HINSTANCE CDsSettings::GetD3X9Dll()
{
if (m_hD3DX9Dll == NULL)
  {
    int min_ver = D3DX_SDK_VERSION;
    int max_ver = D3DX_SDK_VERSION;
    
    m_nDXSdkRelease = 0;

    if(D3DX_SDK_VERSION >= 42) {
      // August 2009 SDK (v42) is not compatible with older versions
      min_ver = 42;      
    } else {
      if(D3DX_SDK_VERSION > 33) {
        // versions between 34 and 41 have no known compatibility issues
        min_ver = 34;
      }  else {    
        // The minimum version that supports the functionality required by MPC is 24
        min_ver = 24;
  
        if(D3DX_SDK_VERSION == 33) {
          // The April 2007 SDK (v33) should not be used (crash sometimes during shader compilation)
          max_ver = 32;    
        }
      }
    }
    
    // load latest compatible version of the DLL that is available
    for (int i=max_ver; i>=min_ver; i--)
    {
      m_strD3DX9Version.Format(_T("d3dx9_%d.dll"), i);
      m_hD3DX9Dll = LoadLibrary (m_strD3DX9Version);
      if (m_hD3DX9Dll) 
      {
        m_nDXSdkRelease = i;
        break;
      }
    }
  }

  return m_hD3DX9Dll;
}

class CDsSettings g_dsSettings;
bool g_bNoDuration = false;
bool g_bExternalSubtitleTime = false;