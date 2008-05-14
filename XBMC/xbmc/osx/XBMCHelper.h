#ifndef __XBMCHELPER_H__
#define __XBMCHELPER_H__

class XBMCHelper
{
 public:
  
  XBMCHelper();
   
  void Start();
  void Stop();
  
  void Configure();
  
  void Install();
  void Uninstall();
  
  bool IsRemoteBuddyInstalled();
  bool IsSofaControlRunning();
 
  bool IsAlwaysOn() const { return m_alwaysOn; }
  int  GetMode() const { return m_mode; }
  
  bool ErrorStarting() { return m_errorStarting; }
  
 private:
   
  int GetProcessPid(const char* processName);
  
  std::string ReadFile(const char* fileName);
  void WriteFile(const char* fileName, const std::string& data);
   
  bool m_alwaysOn;
  int  m_mode;
  int  m_sequenceDelay;
  bool m_errorStarting;
  
  std::string m_configFile;
  std::string m_launchAgentLocalFile;
  std::string m_launchAgentInstallFile;
  std::string m_helperFile;
};

extern XBMCHelper g_xbmcHelper;

#endif
