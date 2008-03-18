#pragma once

class IAudioDeviceChangedCallback
{
public:
  virtual void  Initialize(int iDevice)=0;
  virtual void  DeInitialize(int iDevice)=0;
  virtual ~IAudioDeviceChangedCallback() {}
};

