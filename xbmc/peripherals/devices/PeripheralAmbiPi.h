#pragma once
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

#include "system.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"

#include "Peripheral.h"
#include "peripherals/devices/ambipi/AmbiPiConnection.h"
#include "peripherals/devices/ambipi/AmbiPiGrid.h"

namespace PERIPHERALS
{

  class CPeripheralAmbiPi : public CPeripheral, private CThread
  {
  public:
    CPeripheralAmbiPi(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId);
    virtual ~CPeripheralAmbiPi(void);

    void OnSettingChanged(const CStdString &strChangedSetting);

  protected:
	  bool InitialiseFeature(const PeripheralFeature feature);

    void ConnectToDevice(void);
    void DisconnectFromDevice(void);

    void LoadAddressFromConfiguration(void);
    bool ConfigureRenderCallback(void);

    void Process(void);
    bool IsRunning(void) const;

    void ProcessImage(void);
    void UpdateImage(void);
    void GenerateDataStreamFromImage(void);
    void UpdateGridFromConfiguration(void);
    void SendData(void);

    bool                              m_bStarted;
    bool                              m_bIsRunning;
    int                               m_port;
    CStdString                        m_address;

  private:
    CCriticalSection                  m_critSection;
    YV12Image                         m_image;    
    CAmbiPiGrid*                      m_pGrid;
    unsigned int                      m_previousImageWidth;
    unsigned int                      m_previousImageHeight;

    static void RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect);

    void UpdateSampleRectangles(unsigned int imageWidth, unsigned int imageHeight);
    CAmbiPiConnection                 m_connection;
  };
}
