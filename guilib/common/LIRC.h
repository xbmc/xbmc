#ifndef LIRC_H
#define LIRC_H

#include "../system.h"
 
class CRemoteControl
{
public:
   CRemoteControl();
   ~CRemoteControl();
   void Initialize();
   void Reset();
   void Update();
   WORD GetButton();
   bool IsHolding();
   
private:
   int   m_fd;
   FILE* m_file;
   bool  m_isHolding;
   WORD  m_button;
   char  m_buf[128];
   bool  m_bInitialized;
   Uint32 m_firstClickTime;
};

extern CRemoteControl g_RemoteControl;

#endif    
