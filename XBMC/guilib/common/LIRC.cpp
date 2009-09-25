#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/inotify.h>
#include <limits.h>
#include <unistd.h>
#include "LIRC.h"
#include "ButtonTranslator.h"
#include "log.h"
#include "AdvancedSettings.h"
#include "FileSystem/File.h"

#define LIRC_DEVICE "/dev/lircd"

CRemoteControl g_RemoteControl;

CRemoteControl::CRemoteControl()
{
  m_fd = -1;
  m_file = NULL;
  m_bInitialized = false;
  m_skipHold = false;
  m_button = 0;
  m_isHolding = false;
  m_used = true;
  m_deviceName = LIRC_DEVICE;
  m_inotify_fd = -1;
  m_inotify_wd = -1;
  m_bLogConnectFailure = true;
  m_lastInitAttempt = -5000;
  m_initRetryPeriod = 5000;
  Reset();
}

CRemoteControl::~CRemoteControl()
{
  if (m_file != NULL)
    fclose(m_file);
}

void CRemoteControl::setUsed(bool value)
{
  m_used=value;
  if (!value)
    CLog::Log(LOGINFO, "LIRC %s: disabled", __FUNCTION__);
}

void CRemoteControl::Reset()
{
  m_isHolding = false;
  m_button = 0;
}

void CRemoteControl::Disconnect()
{
  if (!m_used)
    return;

  if (m_fd != -1) 
  {
    m_bInitialized = false;
    if (m_file != NULL)
      fclose(m_file);
    m_fd = -1;
    m_file = NULL;
    if (m_inotify_wd >= 0) {
      inotify_rm_watch(m_inotify_fd, m_inotify_wd);
      m_inotify_wd = -1;
    }
    if (m_inotify_fd >= 0)
      close(m_inotify_fd);
  }
}

void CRemoteControl::setDeviceName(const CStdString& value)
{
  if (value.length()>0)
    m_deviceName=value;
  else
    m_deviceName=LIRC_DEVICE;
}

void CRemoteControl::Initialize()
{
  struct sockaddr_un addr;
  int now = timeGetTime();

  if (!m_used || now < m_lastInitAttempt + m_initRetryPeriod)
    return;
  m_lastInitAttempt = now;

  if (!XFILE::CFile::Exists(m_deviceName)) {
    m_initRetryPeriod *= 2;
    if (m_initRetryPeriod > 60000)
    {
      m_used = false;
      CLog::Log(LOGDEBUG, "LIRC device %s does not exist. Giving up.", m_deviceName.c_str());
    }
    else
      CLog::Log(LOGDEBUG, "LIRC device %s does not exist. Retry in %ds.", m_deviceName.c_str(), m_initRetryPeriod/1000);
    return;
  }

  m_initRetryPeriod = 5000;

  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, m_deviceName.c_str());

  // Open the socket from which we will receive the remote commands
  if ((m_fd = socket(AF_UNIX, SOCK_STREAM, 0)) != -1)
  {
    // Connect to the socket
    if (connect(m_fd, (struct sockaddr *)&addr, sizeof(addr)) != -1)
    {
      int opts;
      m_bLogConnectFailure = true;
      if ((opts = fcntl(m_fd,F_GETFL)) != -1)
      {
        // Set the socket to non-blocking
        opts = (opts | O_NONBLOCK);
        if (fcntl(m_fd,F_SETFL,opts) != -1)
        {
          if ((m_file = fdopen(m_fd, "r")) != NULL)
          {
            // Setup inotify so we can disconnect if lircd is restarted
            if ((m_inotify_fd = inotify_init()) >= 0)
            {
              // Set the fd non-blocking
              if ((opts = fcntl(m_inotify_fd, F_GETFL)) != -1)
              {
                opts |= O_NONBLOCK;
                if (fcntl(m_inotify_fd, F_SETFL, opts) != -1)
                {
                  // Set an inotify watch on the lirc device
                  if ((m_inotify_wd = inotify_add_watch(m_inotify_fd, m_deviceName.c_str(), IN_DELETE_SELF)) != -1)
                  {
                    m_bInitialized = true;
                    CLog::Log(LOGINFO, "LIRC %s: sucessfully started on: %s", __FUNCTION__, addr.sun_path);
                  }
                  else
                    CLog::Log(LOGDEBUG, "LIRC: Failed to initialize Inotify. LIRC device will not be monitored.");
                }
              }
            }
          }
          else
            CLog::Log(LOGERROR, "LIRC %s: fdopen failed: %s", __FUNCTION__, strerror(errno));
        }
        else
          CLog::Log(LOGERROR, "LIRC %s: fcntl(F_SETFL) failed: %s", __FUNCTION__, strerror(errno));
      }
      else
        CLog::Log(LOGERROR, "LIRC %s: fcntl(F_GETFL) failed: %s", __FUNCTION__, strerror(errno));
    }
    else
    {
      if (m_bLogConnectFailure)
      {
        CLog::Log(LOGINFO, "LIRC %s: connect failed: %s", __FUNCTION__, strerror(errno));
        m_bLogConnectFailure = false;
      }
    }
  }
  else
    CLog::Log(LOGINFO, "LIRC %s: socket failed: %s", __FUNCTION__, strerror(errno));
  if (!m_bInitialized)
    Disconnect();
}

bool CRemoteControl::CheckDevice() {
  if (m_inotify_fd < 0 || m_inotify_wd < 0)
    return true; // inotify wasn't setup for some reason, assume all is well
  int bufsize = sizeof(struct inotify_event) + PATH_MAX;
  char buf[bufsize];
  int ret = read(m_inotify_fd, buf, bufsize);
  for (int i = 0; i + (int)sizeof(struct inotify_event) <= ret;) {
    struct inotify_event* e = (struct inotify_event*)(buf+i);
    if (e->mask & IN_DELETE_SELF) {
      CLog::Log(LOGDEBUG, "LIRC device removed, disconnecting...");
      Disconnect();
      return false;
    }
    i += sizeof(struct inotify_event)+e->len;
  }
  return true;
}

void CRemoteControl::Update()
{
  if (!m_bInitialized || !m_used )
    return;

  if (!CheckDevice())
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

    // Some template LIRC configuration have button names in apostrophes or quotes.
    // If we got a quoted button name, strip 'em
    unsigned int buttonNameLen = strlen(buttonName);
    if ( buttonNameLen > 2
    && ( (buttonName[0] == '\'' && buttonName[buttonNameLen-1] == '\'')
         || ((buttonName[0] == '"' && buttonName[buttonNameLen-1] == '"') ) ) )
    {
      memmove( buttonName, buttonName + 1, buttonNameLen - 2 );
      buttonName[ buttonNameLen - 2 ] = '\0';
    }

    m_button = CButtonTranslator::GetInstance().TranslateLircRemoteString(deviceName, buttonName);

    if (strcmp(repeatStr, "00") == 0)
    {
      CLog::Log(LOGDEBUG, "LIRC: %s - NEW at %d:%s (%s)", __FUNCTION__, now, m_buf, buttonName);
      m_firstClickTime = now;
      m_isHolding = false;
      m_skipHold = true;
      return;
    }
    else if (now - m_firstClickTime >= (Uint32) g_advancedSettings.m_remoteRepeat && !m_skipHold)
    {
      m_isHolding = true;
    }
    else
    {
      m_isHolding = false;
      m_button = 0;
    }
  }
  if (feof(m_file) != 0)
    Disconnect();
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
