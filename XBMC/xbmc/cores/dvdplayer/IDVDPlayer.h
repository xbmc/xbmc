
#pragma once

class DVDNavResult;

class IDVDPlayer
{
public:
  virtual int OnDVDNavResult(void* pData, int iMessage) = 0;
  virtual ~IDVDPlayer() { }
};
