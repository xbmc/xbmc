#ifndef CXRANDR
#define CXRANDR

#include "StdString.h"
#include <vector>
#include <map>
 
struct XMode
{
   CStdString id;
   CStdString name;
   float hz;
   bool isPreferred;
   bool isCurrent;
   unsigned int w;
   unsigned int h;
};

struct XOutput
{
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
   CXRandR();
   std::vector<XOutput> GetModes(void);
   XMode GetCurrentMode(CStdString outputName);
   bool SetMode(XOutput output, XMode mode);
   void LoadCustomModeLinesToAllOutputs(void);
   
private:
   void Query();   
   std::vector<XOutput> m_outputs;
};

#endif
