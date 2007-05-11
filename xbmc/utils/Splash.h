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

  // In case you don't want to use another thread
  void Show();
  void Hide();
  
private:
  virtual void Process();
  virtual void OnStartup();
  virtual void OnExit();

  float fade;
  CStdString m_ImageName;
#ifndef HAS_SDL
  D3DGAMMARAMP newRamp;
  D3DGAMMARAMP oldRamp;  
 
#endif  
};
