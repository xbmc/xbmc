// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define XBMC_MAX_PATH 1024 // normal max path is 260, but smb shares and the like can be longer

#define DEBUG_MOUSE
#define DEBUG_KEYBOARD
#include <xtl.h>
#include <xvoice.h>
#include <xonline.h>
#include <XGraphics.h>
#include <xbutil.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <queue>
#include "stdstring.h"
#include "StringUtils.h"
#include "memutil.h"
using namespace std;

#if defined(_DEBUG) && defined(_MEMTRACKING)
#define _CRTDBG_MAP_ALLOC
#include <FStream>
#include <stdlib.h>
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif

// guilib internal
#include "tinyxml/tinyxml.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"

#include "utils/Thread.h"
#include "utils/CriticalSection.h"
#include "utils/SingleLock.h"
#include "utils/Event.h"
#include "utils/Mutex.h"
#include "utils/archive.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"

#include "MusicInfotag.h"
using namespace MUSIC_INFO;
#include "Song.h"
#include "Url.h"
#include "FileSystem/Directory.h"
using namespace DIRECTORY;
#include "FileSystem/File.h"
using namespace XFILE;
#include "SectionLoader.h"
#include "ApplicationMessenger.h"
#include "crc32.h"
#include "AutoPtrHandle.h"
using namespace AUTOPTR;

// Often used
#include "GUIDialogOK.h"
#include "GUIDialogProgress.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogSelect.h"
#include "GUIDialogKeyboard.h"
#include "Settings.h"
#include "GuiUserMessages.h"

#ifdef QueryPerformanceFrequency
#undef QueryPerformanceFrequency
#endif
WINBASEAPI BOOL WINAPI QueryPerformanceFrequencyXbox(LARGE_INTEGER *lpFrequency);
#define QueryPerformanceFrequency(a) QueryPerformanceFrequencyXbox(a)





// TODO: reference additional headers your program requires here
