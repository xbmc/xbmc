#ifndef XBMC_PC_H_
#define XBMC_PC_H_

class CXBMC_PC
{
public:
  CXBMC_PC();
  ~CXBMC_PC();

  HRESULT Create( HINSTANCE hInstance, LPSTR commandLine );
  INT Run();
  HINSTANCE m_hInstance;
};

#endif
