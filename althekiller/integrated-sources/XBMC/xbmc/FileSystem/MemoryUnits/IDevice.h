#pragma once

class IDevice
{
public:
  IDevice(unsigned long port, unsigned long slot, void *device)
  {
    m_port = port;
    m_slot = slot;
    m_device = device;
  };

  const CStdString &GetVolumeName() { return m_volumeName; };
  bool IsInPort(unsigned long port, unsigned long slot)
  {
    return (m_port == port && m_slot == slot);
  }

  virtual void LogInfo()=0;
  virtual const char *GetFileSystem()=0;
  virtual void UnMount()=0;
  virtual bool Mount(const char *device)=0;

protected:
  void*          m_device;
  CStdString     m_volumeName;
  unsigned long  m_port;
  unsigned long  m_slot;
};
