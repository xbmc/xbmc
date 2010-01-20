#pragma once

class IPowerSyscall
{
public:
  virtual ~IPowerSyscall() {};
  virtual bool Powerdown()    = 0;
  virtual bool Suspend()      = 0;
  virtual bool Hibernate()    = 0;
  virtual bool Reboot()       = 0;

// Might need to be membervariables instead for speed
  virtual bool CanPowerdown()  = 0;
  virtual bool CanSuspend()   = 0;
  virtual bool CanHibernate() = 0;
  virtual bool CanReboot()    = 0;
};
