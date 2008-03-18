// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define XBMC_MAX_PATH 1024 // normal max path is 260, but smb shares and the like can be longer

#define DEBUG_MOUSE
#define DEBUG_KEYBOARD
#include "system.h"
#include "gui3d.h"

#include <vector>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <queue>
#include "StdString.h"
#include "StringUtils.h"

#ifdef _XBOX
#if defined(_DEBUG) && defined(_MEMTRACKING)
#define _CRTDBG_MAP_ALLOC
#include <FStream>
#include <stdlib.h>
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif

// guilib internal
#include "tinyXML/tinyxml.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"

#include "utils/Thread.h"
#include "utils/CriticalSection.h"
#include "utils/SingleLock.h"
#include "utils/Event.h"
#include "utils/Mutex.h"
#include "utils/Archive.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "LangInfo.h"

#include "Settings.h"
#include "URL.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#include "SectionLoader.h"
#include "ApplicationMessenger.h"
#include "Crc32.h"
#include "AutoPtrHandle.h"

// Often used
#include "GUIDialogOK.h"
#include "GUIDialogProgress.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogSelect.h"
#include "GUIDialogKeyboard.h"
#include "GUIUserMessages.h"

#ifdef _XBOX
#ifdef QueryPerformanceFrequency
#undef QueryPerformanceFrequency
#endif
WINBASEAPI BOOL WINAPI QueryPerformanceFrequencyXbox(LARGE_INTEGER *lpFrequency);
#define QueryPerformanceFrequency(a) QueryPerformanceFrequencyXbox(a)
#else
#undef GetFreeSpace
#endif

#define SAFE_DELETE(p)       { delete (p);     (p)=NULL; }
#define SAFE_DELETE_ARRAY(p) { delete[] (p);   (p)=NULL; }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
