#pragma once

class IAudioDeviceChangedCallback
{
public:
  virtual void  Initialize()=0;
  virtual void  DeInitialize()=0;
};

