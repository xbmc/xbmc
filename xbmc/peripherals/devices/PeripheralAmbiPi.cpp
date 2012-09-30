/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "PeripheralAmbiPi.h"
#include "utils/log.h"

#include "cores/VideoRenderers/RenderManager.h"
#include "cores/VideoRenderers/WinRenderer.h"

#define AMBIPI_DEFAULT_PORT 20434

extern CXBMCRenderManager g_renderManager;

using namespace PERIPHERALS;
using namespace AUTOPTR;

CPeripheralAmbiPi::CPeripheralAmbiPi(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId) :
  CPeripheral(type, busType, strLocation, strDeviceName, iVendorId, iProductId),
  CThread("AmbiPi"),
  m_bStarted(false),
  m_bIsRunning(false),
  m_pGrid(NULL),
  m_previousImageWidth(0),
  m_previousImageHeight(0)
{  
  m_features.push_back(FEATURE_AMBIPI);
}

CPeripheralAmbiPi::~CPeripheralAmbiPi(void)
{
  CLog::Log(LOGDEBUG, "%s - Removing AmbiPi on %s", __FUNCTION__, m_strLocation.c_str());

  DisconnectFromDevice();

  delete m_pGrid;
}

bool CPeripheralAmbiPi::InitialiseFeature(const PeripheralFeature feature)
{
  if (feature != FEATURE_AMBIPI || m_bStarted || !GetSettingBool("enabled")) {
    return CPeripheral::InitialiseFeature(feature);
  }

  CLog::Log(LOGDEBUG, "%s - Initialising AmbiPi on %s", __FUNCTION__, m_strLocation.c_str());

  LoadAddressFromConfiguration();
  UpdateGridFromConfiguration();

  ConnectToDevice();

  m_bStarted = true;
  Create();

  return CPeripheral::InitialiseFeature(feature);
}

void CPeripheralAmbiPi::LoadAddressFromConfiguration()
{
  
  CStdString portFromConfiguration = GetSettingString("port");

  int port;

  if (sscanf(portFromConfiguration.c_str(), "%d", &port) &&
      port >= 0 &&
      port <= 65535)
    m_port = port;
  else
    m_port = AMBIPI_DEFAULT_PORT;

  m_address = GetSettingString("address");
}

void CPeripheralAmbiPi::UpdateGridFromConfiguration()
{

  unsigned int width = 16;
  unsigned int height = 11;

  if (m_pGrid)
    delete m_pGrid;

  m_pGrid = new CAmbiPiGrid(width, height);

}

bool CPeripheralAmbiPi::ConfigureRenderCallback()
{
  if (!g_renderManager.IsStarted() || !g_renderManager.IsConfigured()) {
    return false;
  }

  g_renderManager.RegisterRenderUpdateCallBack((const void*)this, RenderUpdateCallBack);
  return true;
}

void CPeripheralAmbiPi::RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect)
{
  CPeripheralAmbiPi *peripheralAmbiPi = (CPeripheralAmbiPi*)ctx;

  peripheralAmbiPi->ProcessImage();
}

void CPeripheralAmbiPi::ProcessImage()
{
  if (!m_connection.IsConnected())
  {
    return;
  }

  UpdateImage();  
  GenerateDataStreamFromImage();
  SendData();
}

void CPeripheralAmbiPi::UpdateImage()
{
  CWinRenderer *pRenderer = g_renderManager.m_pRenderer;
  
  int index = pRenderer->GetImage(&m_image);
}

void CPeripheralAmbiPi::GenerateDataStreamFromImage()
{
  UpdateSampleRectangles(m_image.width, m_image.height);

  m_pGrid->UpdateTilesFromImage(&m_image);
}

void CPeripheralAmbiPi::UpdateSampleRectangles(unsigned int imageWidth, unsigned int imageHeight)
{
  if (imageWidth == m_previousImageWidth || imageHeight == m_previousImageHeight)
    return;

  m_previousImageWidth = imageWidth;
  m_previousImageHeight = imageHeight;

  m_pGrid->UpdateSampleRectangles(imageWidth, imageHeight);
}

void CPeripheralAmbiPi::SendData(void) {
  TileData *pTileData = m_pGrid->GetTileData();
  try {
    m_connection.Send(pTileData->stream, pTileData->streamLength);
  } catch (std::exception) {
    CLog::Log(LOGINFO, "%s - Send exception", __FUNCTION__);  
    // TODO reconnect?
  }
}

void CPeripheralAmbiPi::ConnectToDevice()
{
  CLog::Log(LOGINFO, "%s - Connecting to AmbiPi on %s:%d", __FUNCTION__, m_address.c_str(), m_port);  

  m_connection.Disconnect();
  m_connection.Connect(m_address, m_port);
}

#define RETRY_DELAY_WHEN_UNCONFIGURED 1
#define RETRY_DELAY_WHEN_CONFIGURED 5

void CPeripheralAmbiPi::Process(void)
{

  {
    CSingleLock lock(m_critSection);
    m_bIsRunning = true;
  }

  int retryDelay = RETRY_DELAY_WHEN_UNCONFIGURED;

  while (!m_bStop)
  {
    if (!m_bStop) {
      bool configured = ConfigureRenderCallback();
      
      retryDelay = configured ? RETRY_DELAY_WHEN_CONFIGURED : RETRY_DELAY_WHEN_UNCONFIGURED;
    }

    if (!m_bStop)
      Sleep(retryDelay * 1000);
  }

  {
    CSingleLock lock(m_critSection);
    m_bStarted = false;
    m_bIsRunning = false;
  }

}

void CPeripheralAmbiPi::OnSettingChanged(const CStdString &strChangedSetting)
{
  CLog::Log(LOGDEBUG, "%s - handling configuration change, setting: '%s'", __FUNCTION__, strChangedSetting.c_str());

  DisconnectFromDevice();
  InitialiseFeature(FEATURE_AMBIPI);
}

void CPeripheralAmbiPi::DisconnectFromDevice(void)
{
  CLog::Log(LOGDEBUG, "%s - disconnecting from the AmbiPi", __FUNCTION__);
  if (IsRunning())
  {
    StopThread(true);
  }
  m_connection.Disconnect();
}

bool CPeripheralAmbiPi::IsRunning(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsRunning;
}

