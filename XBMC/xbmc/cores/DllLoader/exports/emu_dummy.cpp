
#include "stdafx.h"
#include "emu_dummy.h"

extern "C" void not_implement( LPCSTR debuginfo)
{
  if (debuginfo)
  {
    CLog::Log(LOGDEBUG, debuginfo);
  }
}

