#ifndef LIRC_H
#define LIRC_H

#include "../system.h"
#include "StdString.h"

class CRemoteControl
{
public:
  CRemoteControl();
  ~CRemoteControl();
  void Initialize();
  void Disconnect();
  void Reset();
  void Update();
  WORD GetButton();
  bool IsHolding();
  void setDeviceName(const CStdString& value);
  void setUsed(bool value);
  bool IsInUse() const { return m_used; }
  bool IsInitialized() const { return m_bInitialized; }

private:
  int     m_fd;
  int     m_inotify_fd;
  int     m_inotify_wd;
  int     m_lastInitAttempt;
  int     m_initRetryPeriod;
  FILE*   m_file;
  bool    m_isHolding;
  int32_t m_button;
  char    m_buf[128];
  bool    m_bInitialized;
  bool    m_skipHold;
  bool    m_used;
  bool    m_bLogConnectFailure;
  uint32_t    m_firstClickTime;
  CStdString  m_deviceName;
  bool        CheckDevice();
};

extern CRemoteControl g_RemoteControl;

#endif
