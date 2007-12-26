#ifndef CXRANDR
#define CXRANDR

#ifdef HAS_XRANDR

#include "StdString.h"
#include <vector>
#include <map>

class XMode
{
public:
  XMode()
    {
      id="";
      name="";
      hz=0.0f;
      isPreferred=false;
      isCurrent=false;
      w=h=0;
    }
  bool operator==(XMode& mode) const
    {
      if (id!=mode.id)
        return false;
      if (name!=mode.name)
        return false;
      if (hz!=mode.hz)
        return false;
      if (isPreferred!=mode.isPreferred)
        return false;
      if (isCurrent!=mode.isCurrent)
        return false;
      if (w!=mode.w)
        return false;
      if (h!=mode.h)
        return false;
      return true;
    }
  CStdString id;
  CStdString name;
  float hz;
  bool isPreferred;
  bool isCurrent;
  unsigned int w;
  unsigned int h;
};

class XOutput
{
public:
  XOutput()
    {
      name="";
      isConnected=false;
      w=h=x=y=wmm=hmm=0;
    }
  CStdString name;
  bool isConnected;
  int w;
  int h;
  int x;
  int y;
  int wmm;
  int hmm;
  vector<XMode> modes;
};

class CXRandR
{
public:
  CXRandR(bool query=true);
  std::vector<XOutput> GetModes(void);
  XMode GetCurrentMode(CStdString outputName);
  bool SetMode(XOutput output, XMode mode);
  void LoadCustomModeLinesToAllOutputs(void);
  void SaveState();
  void RestoreState();
  //bool Has1080i();
  //bool Has1080p();
  //bool Has720p();
  //bool Has480p();

private:
  bool m_bInit;
  std::vector<XOutput> m_current;
  void Query(bool force=false);
  std::vector<XOutput> m_outputs;
  CStdString m_currentOutput;
  CStdString m_currentMode;
};

extern CXRandR g_xrandr;

#endif

#endif
