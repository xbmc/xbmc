#ifndef LIRC_H
#define LIRC_H

#include "../system.h"

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

private:
  int   m_fd;
  FILE* m_file;
  bool  m_isHolding;
  WORD  m_button;
  char  m_buf[128];
  bool  m_bInitialized;
  bool  m_skipHold;
  bool  m_used;
  Uint32 m_firstClickTime;
  CStdString m_deviceName;
};

extern CRemoteControl g_RemoteControl;

#endif
