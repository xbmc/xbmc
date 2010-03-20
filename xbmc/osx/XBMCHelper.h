#ifndef __XBMCHELPER_H__
#define __XBMCHELPER_H__

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

class XBMCHelper
{
 public:

  static XBMCHelper& GetInstance();

  void Start();
  void Stop();

  void Configure();

  bool IsRunning();

  bool IsAlwaysOn() const { return m_alwaysOn; }
  int  GetMode() const { return m_mode; }

  bool ErrorStarting() { return m_errorStarting; }

  //global helpers
  static void CaptureAllInput();
  static void ReleaseAllInput();

private:
  XBMCHelper();
  XBMCHelper(XBMCHelper const& );
  XBMCHelper& operator=(XBMCHelper const&);

  void Install();
  void Uninstall();

  bool IsRemoteBuddyInstalled();
  bool IsSofaControlRunning();

  int GetProcessPid(const char* processName);

  std::string ReadFile(const char* fileName);
  void WriteFile(const char* fileName, const std::string& data);

  bool m_alwaysOn;
  int  m_mode;
  int  m_sequenceDelay;
  int  m_port;
  bool m_errorStarting;

  std::string m_configFile;
  std::string m_launchAgentLocalFile;
  std::string m_launchAgentInstallFile;
  std::string m_homepath;
  std::string m_helperFile;

  static XBMCHelper* smp_instance;
};

#endif
