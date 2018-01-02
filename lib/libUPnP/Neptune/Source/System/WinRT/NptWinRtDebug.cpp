/*****************************************************************
|
|   Neptune - Debug Support: WinRT Implementation
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
#include "NptDefs.h"
#include "NptTypes.h"
#include "NptDebug.h"
#include "NptWinRtUtils.h"

/*----------------------------------------------------------------------
|   NPT_DebugOutput
+---------------------------------------------------------------------*/
void
NPT_DebugOutput(const char* message)
{
    NPT_WINRT_USE_CHAR_CONVERSION;
    OutputDebugStringW(NPT_WINRT_A2W(message));
}

