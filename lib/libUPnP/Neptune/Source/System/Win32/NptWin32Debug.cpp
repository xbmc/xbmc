/*****************************************************************
|
|   Neptune - Debug Support: Win32 Implementation
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <windows.h>

#include "NptConfig.h"
#include "NptDefs.h"
#include "NptTypes.h"
#include "NptDebug.h"
#include <memory>

/*----------------------------------------------------------------------
|   NPT_DebugOutput
+---------------------------------------------------------------------*/
void
NPT_DebugOutput(const char* message)
{
  int result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, message, -1, nullptr, 0);
  if (result == 0)
    return;

  auto newStr = std::make_unique<wchar_t[]>(result + 1);
  result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, message, result, newStr.get(), result);

  if (result == 0)
    return;

  OutputDebugString(newStr.get());
}

