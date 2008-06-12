/*****************************************************************
|
|   Neptune - Toplevel Include
|
|   (c) 2001-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NEPTUNE_H_
#define _NEPTUNE_H_

/*----------------------------------------------------------------------
|   flags
+---------------------------------------------------------------------*/
#define NPT_EXTERNAL_USE /* do not expose internal definitions */

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptCommon.h"
#include "NptResults.h"
#include "NptTypes.h"
#include "NptConstants.h"
#include "NptReferences.h"
#include "NptStreams.h"
#include "NptBufferedStreams.h"
#include "NptFile.h"
#include "NptNetwork.h"
#include "NptSockets.h"
#include "NptTime.h"
#include "NptThreads.h"
#include "NptSystem.h"
#include "NptMessaging.h"
#include "NptQueue.h"
#include "NptSimpleMessageQueue.h"
#include "NptSelectableMessageQueue.h"
#include "NptXml.h"
#include "NptStrings.h"
#include "NptArray.h"
#include "NptList.h"
#include "NptMap.h"
#include "NptStack.h"
#include "NptUri.h"
#include "NptHttp.h"
#include "NptDataBuffer.h"
#include "NptUtils.h"
#include "NptRingBuffer.h"
#include "NptBase64.h"
#include "NptConsole.h"
#include "NptLogging.h"
#include "NptSerialPort.h"
#include "NptVersion.h"
#include "NptDirectory.h"

// optional modules
#include "NptZip.h"

#endif // _NEPTUNE_H_
