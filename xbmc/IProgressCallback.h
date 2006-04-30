#pragma once

class IProgressCallback
{
public:
  virtual void SetProgressMax(int max)=0;
  virtual void SetProgressAdvance(int nSteps=1)=0;
  virtual bool Abort()=0;
};
