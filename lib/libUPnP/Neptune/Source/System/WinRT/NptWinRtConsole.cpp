/*****************************************************************
|
|   Neptune - Console Support: WinRT Implementation
|
|   (c) 2002-2013 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptWinRtPch.h"

#include "NptConfig.h"
#include "NptConsole.h"
#include "NptWinRtUtils.h"

/*----------------------------------------------------------------------
|   NPT_Console::Output
+---------------------------------------------------------------------*/
void
NPT_Console::Output(const char* message)
{
    NPT_WINRT_USE_CHAR_CONVERSION;
    OutputDebugStringW(NPT_WINRT_A2W(message));
}

