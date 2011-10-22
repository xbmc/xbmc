#pragma once
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

#include "PeripheralHID.h"
#include "interfaces/AnnouncementManager.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include <queue>

class DllLibCEC;

namespace CEC
{
  class ICECAdapter;
};

namespace PERIPHERALS
{
  typedef struct
  {
    WORD         iButton;
    unsigned int iDuration;
  } CecButtonPress;


  class CPeripheralCecAdapter : public CPeripheralHID, public ANNOUNCEMENT::IAnnouncer, private CThread
  {
  public:
    CPeripheralCecAdapter(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId);
    virtual ~CPeripheralCecAdapter(void);

    virtual void Announce(ANNOUNCEMENT::EAnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
    virtual bool PowerOnCecDevices(void);
    virtual bool StandbyCecDevices(void);

    virtual bool SendPing(void);
    virtual bool StartBootloader(void);
    virtual bool SetHdmiPort(int iHdmiPort);

    virtual void OnSettingChanged(const CStdString &strChangedSetting);

    virtual WORD GetButton(void);
    virtual unsigned int GetHoldTime(void);
    virtual void ResetButton(void);

  protected:
    virtual void FlushLog(void);
    virtual bool GetNextKey(void);
    virtual bool InitialiseFeature(const PeripheralFeature feature);
    virtual void Process(void);
    virtual void ProcessNextCommand(void);
    virtual void SetMenuLanguage(const char *strLanguage);
    static bool FindConfigLocation(CStdString &strString);
    static bool TranslateComPort(CStdString &strPort);

    DllLibCEC*        m_dll;
    CEC::ICECAdapter* m_cecAdapter;
    bool              m_bStarted;
    bool              m_bHasButton;
    bool              m_bIsReady;
    CDateTime         m_screensaverLastActivated;
    CecButtonPress    m_button;
    CCriticalSection  m_critSection;
  };
}
