#pragma once
#include "thread.h"

class CInfoLoader;

class CBackgroundLoader : public CThread
{
public:
  CBackgroundLoader(CInfoLoader *callback);
  ~CBackgroundLoader();

protected:
  void Process();
  virtual void GetInformation()=0;
  CInfoLoader *m_callback;
};

class CInfoLoader
{
public:
  CInfoLoader(const char *type);
  ~CInfoLoader();
  const char *GetInfo(DWORD dwInfo);
  void Refresh();
  void LoaderFinished();
protected:
  virtual const char *TranslateInfo(DWORD dwInfo);
  virtual const char *BusyInfo(DWORD dwInfo);
  virtual DWORD TimeToNextRefreshInMs() { return 300000; }; // default to 5 minutes
private:
  DWORD m_refreshTime;
  CBackgroundLoader *m_backgroundLoader;
  bool m_busy;
  CStdString m_busyText;
  CStdString m_type;
};
