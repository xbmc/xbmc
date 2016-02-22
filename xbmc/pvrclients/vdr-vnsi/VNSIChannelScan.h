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

#include "VNSIData.h"
#include "client.h"
#include <string>
#include <map>

typedef enum scantype
{
  DVB_TERR    = 0,
  DVB_CABLE   = 1,
  DVB_SAT     = 2,
  PVRINPUT    = 3,
  PVRINPUT_FM = 4,
  DVB_ATSC    = 5,
} scantype_t;


class cVNSIChannelScan : public cVNSIData
{
public:

  cVNSIChannelScan();
  ~cVNSIChannelScan();

  bool Open(const std::string& hostname, int port, const char* name = "XBMC channel scanner");

  bool OnClick(int controlId);
  bool OnFocus(int controlId);
  bool OnInit();
  bool OnAction(int actionId);

  static bool OnClickCB(GUIHANDLE cbhdl, int controlId);
  static bool OnFocusCB(GUIHANDLE cbhdl, int controlId);
  static bool OnInitCB(GUIHANDLE cbhdl);
  static bool OnActionCB(GUIHANDLE cbhdl, int actionId);

protected:

  bool OnResponsePacket(cResponsePacket* resp);

private:

  bool ReadCountries();
  bool ReadSatellites();
  void SetControlsVisible(scantype_t type);
  void StartScan();
  void StopScan();
  void ReturnFromProcessView();
  void SetProgress(int procent);
  void SetSignal(int procent, bool locked);

  std::string     m_header;
  std::string     m_Signal;
  bool            m_running;
  bool            m_stopped;
  bool            m_Canceled;

  CAddonGUIWindow      *m_window;
  CAddonGUISpinControl *m_spinSourceType;
  CAddonGUISpinControl *m_spinCountries;
  CAddonGUISpinControl *m_spinSatellites;
  CAddonGUISpinControl *m_spinDVBCInversion;
  CAddonGUISpinControl *m_spinDVBCSymbolrates;
  CAddonGUISpinControl *m_spinDVBCqam;
  CAddonGUISpinControl *m_spinDVBTInversion;
  CAddonGUISpinControl *m_spinATSCType;
  CAddonGUIRadioButton *m_radioButtonTV;
  CAddonGUIRadioButton *m_radioButtonRadio;
  CAddonGUIRadioButton *m_radioButtonFTA;
  CAddonGUIRadioButton *m_radioButtonScrambled;
  CAddonGUIRadioButton *m_radioButtonHD;
  CAddonGUIProgressControl *m_progressDone;
  CAddonGUIProgressControl *m_progressSignal;
};
