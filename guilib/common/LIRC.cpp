#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "LIRC.h"
#include "ButtonTranslator.h"
#include "log.h"
#include "Settings.h"

#define LIRC_DEVICE "/dev/lircd"

CRemoteControl g_RemoteControl;

CRemoteControl::CRemoteControl()
{
  m_fd = -1;
  m_file = NULL;  
  m_bInitialized = false;
  m_skipHold = false;
  Reset();
}

CRemoteControl::~CRemoteControl()
{
  if (m_file != NULL)
    fclose(m_file);
}

void CRemoteControl::Reset()
{
  m_isHolding = false;
  m_button = 0;
}

void CRemoteControl::Initialize()
{
  struct sockaddr_un addr;
  
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, LIRC_DEVICE);
 
  // Open the socket from which we will receive the remote commands 
  m_fd = socket(AF_UNIX, SOCK_STREAM,0);
  if (m_fd == -1)  
  {
    CLog::Log(LOGERROR, "LIRC %s: socket failed: %s", __FUNCTION__, strerror(errno));
    return;
  }
  
  // Connect to the socket
  if (connect(m_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)  
  {
    CLog::Log(LOGERROR, "LIRC %s: connect failed: %s", __FUNCTION__, strerror(errno));
    return;
  }
  
  // Set the socket to non-blocking
  int opts = fcntl(m_fd,F_GETFL);
  if (opts == -1) 
  {
    CLog::Log(LOGERROR, "LIRC %s: fcntl(F_GETFL) failed: %s", __FUNCTION__, strerror(errno));
    return;
  }
	
  opts = (opts | O_NONBLOCK);
  if (fcntl(m_fd,F_SETFL,opts) == -1) 
  {
    CLog::Log(LOGERROR, "LIRC %s: fcntl(F_SETFL) failed: %s", __FUNCTION__, strerror(errno));
    return;
  }
	  
  m_file = fdopen(m_fd, "r");
  if (m_file == NULL)
  {
    CLog::Log(LOGERROR, "LIRC %s: fdopen failed: %s", __FUNCTION__, strerror(errno));
    return;
  }
  
  m_bInitialized = true;
}

void CRemoteControl::Update()
{
  if (!m_bInitialized)
    return;

  Uint32 now = SDL_GetTicks(); 

  // Read a line from the socket
  while (fgets(m_buf, sizeof(m_buf), m_file) != NULL)
  {
    // Remove the \n
    m_buf[strlen(m_buf)-1] = '\0';

    // Parse the result. Sample line:
    // 000000037ff07bdd 00 OK mceusb
    char scanCode[128];  
    char buttonName[128];
    char repeatStr[4];
    char deviceName[128];
    sscanf(m_buf, "%s %s %s %s", &scanCode[0], &repeatStr[0], &buttonName[0], &deviceName[0]);

    m_button = g_buttonTranslator.TranslateLircRemoteString(deviceName, buttonName);

    if (strcmp(repeatStr, "00") == 0)
    {
      CLog::Log(LOGDEBUG, "%s - NEW at %d:%s", __FUNCTION__, now, m_buf);
      m_firstClickTime = now;
      m_isHolding = false;
      m_skipHold = true;
      return;
    }
    else if (now - m_firstClickTime >= g_advancedSettings.m_remoteRepeat && !m_skipHold)
    {
      m_isHolding = true;
    }
    else
    {
      m_isHolding = false;
      m_button = 0;
    }
  }
  m_skipHold = false;
}

WORD CRemoteControl::GetButton()
{
  return m_button;
}

bool CRemoteControl::IsHolding()
{
  return m_isHolding;
}
