#pragma once

class IPowerEventsCallback
{
public:
  virtual ~IPowerEventsCallback() { }

  virtual void OnSleep() = 0;
  virtual void OnWake() = 0;
};

class IPowerSyscall
{
public:
  virtual ~IPowerSyscall() {};
  virtual bool Powerdown()    = 0;
  virtual bool Suspend()      = 0;
  virtual bool Hibernate()    = 0;
  virtual bool Reboot()       = 0;

// Might need to be membervariables instead for speed
  virtual bool CanPowerdown() = 0;
  virtual bool CanSuspend()   = 0;
  virtual bool CanHibernate() = 0;
  virtual bool CanReboot()    = 0;

  /*!
   \brief Pump power related events back to xbmc.

   PumpPowerEvents is called from Application Thread and the platform implementation may signal
   power related events back to xbmc through the callback.

   return true if an event occured and false if not.
   
   \param callback the callback to signal to
   */
  virtual bool PumpPowerEvents(IPowerEventsCallback *callback) = 0;
};

class CPowerSyscallWithoutEvents : public IPowerSyscall
{
public:
  CPowerSyscallWithoutEvents() { m_OnResume = false; m_OnSuspend = false; }

  virtual bool Suspend() { m_OnSuspend = true; return false; }
  virtual bool Hibernate() { m_OnSuspend = true; return false; }

  virtual bool PumpPowerEvents(IPowerEventsCallback *callback)
  {
    if (m_OnSuspend)
    {
      callback->OnSleep();
      m_OnSuspend = false;
      m_OnResume = true;
      return true;
    }
    else if (m_OnResume)
    {
      callback->OnWake();
      m_OnResume = false;
      return true;
    }
    else
      return false;
  }
private:
  bool m_OnResume;
  bool m_OnSuspend;
};
