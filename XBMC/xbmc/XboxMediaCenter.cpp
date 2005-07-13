
#include "stdafx.h" 
// XboxMediaCenter
//
// libraries:
//   - CDRipX   : doesnt support section loading yet
//   - xbfilezilla : doesnt support section loading yet
//

#include "stdafx.h"
#include "application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CApplication g_application;
void main()
{
#ifdef _DEBUG
  // Get current flag
  int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

  // Turn on leak-checking bit
  tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

  // Turn off CRT block checking bit
  tmpFlag &= ~_CRTDBG_CHECK_CRT_DF;

  // Set flag to the new value
  _CrtSetDbgFlag( tmpFlag );
#endif
  g_application.Create();
  while (1)
  {
    g_application.Run();
  }
}

extern "C"
{

  void mp_msg( int x, int lev, const char *format, ... )
  {
    va_list va;
    static char tmp[2048];
    va_start(va, format);
    _vsnprintf(tmp, 2048, format, va);
    va_end(va);
    tmp[2048 - 1] = 0;

    OutputDebugString(tmp);
  }
}
