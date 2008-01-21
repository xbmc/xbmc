#pragma once

#include "Thread.h"

class CSplash : public CThread
{
public:
  CSplash(const CStdString& imageName);
  virtual ~CSplash();

  bool IsRunning();
  bool Start();
  void Stop();
private:
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  CStdString m_ImageName;
};
