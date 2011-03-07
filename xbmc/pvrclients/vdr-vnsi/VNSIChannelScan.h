#pragma once
/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#include "client.h"
#include "VNSISession.h"
#include "thread.h"

typedef enum scantype
{
  DVB_TERR    = 0,
  DVB_CABLE   = 1,
  DVB_SAT     = 2,
  PVRINPUT    = 3,
  PVRINPUT_FM = 4,
  DVB_ATSC    = 5,
} scantype_t;


class cVNSIChannelScan : public cThread
{
public:
  cVNSIChannelScan();
  ~cVNSIChannelScan();

  bool Open();

  bool OnClick(int controlId);
  bool OnFocus(int controlId);
  bool OnInit();
  bool OnAction(int actionId);

  static bool OnClickCB(GUIHANDLE cbhdl, int controlId);
  static bool OnFocusCB(GUIHANDLE cbhdl, int controlId);
  static bool OnInitCB(GUIHANDLE cbhdl);
  static bool OnActionCB(GUIHANDLE cbhdl, int actionId);

protected:
  virtual void Action(void);

private:
  bool ReadCountries();
  bool ReadSatellites();
  void SetControlsVisible(scantype_t type);
  void StartScan();
  void StopScan();
  void ReturnFromProcessView();
  void SetProgress(int procent);
  void SetSignal(int procent, bool locked);

  cResponsePacket* ReadResult(cRequestPacket* vrp);
  bool readData(uint8_t* buffer, int totalBytes);

  struct SMessage
  {
    cCondWait       *event;
    cResponsePacket *pkt;
  };
  typedef std::map<int, SMessage> SMessages;
  SMessages       m_queue;
  cMutex          m_Mutex;
  cVNSISession    m_session;
  CStdString      m_header;
  CStdString      m_Signal;
  bool            m_running;
  bool            m_stopped;
  bool            m_Canceled;

  cGUIWindow      *m_window;
  cGUISpinControl *m_spinSourceType;
  cGUISpinControl *m_spinCountries;
  cGUISpinControl *m_spinSatellites;
  cGUISpinControl *m_spinDVBCInversion;
  cGUISpinControl *m_spinDVBCSymbolrates;
  cGUISpinControl *m_spinDVBCqam;
  cGUISpinControl *m_spinDVBTInversion;
  cGUISpinControl *m_spinATSCType;
  cGUIRadioButton *m_radioButtonTV;
  cGUIRadioButton *m_radioButtonRadio;
  cGUIRadioButton *m_radioButtonFTA;
  cGUIRadioButton *m_radioButtonScrambled;
  cGUIRadioButton *m_radioButtonHD;
  cGUIProgressControl *m_progressDone;
  cGUIProgressControl *m_progressSignal;

};
