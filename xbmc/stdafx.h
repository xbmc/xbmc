// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

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
using namespace std;

//	guilib internal
#include "tinyxml/tinyxml.h"
#include "GUIWindowManager.h"
#include "GUIDialog.h"
#include "guilistitem.h"
#include "LocalizeStrings.h"

#include "utils/Thread.h"
#include "utils/CriticalSection.h"
#include "utils/SingleLock.h"
#include "utils/Event.h"
#include "utils/Mutex.h"
#include "utils/archive.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"

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

//	Often used
#include "GUIDialogOK.h"
#include "GUIDialogProgress.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogSelect.h"
#include "GUIDialogKeyboard.h"
#include "Settings.h"
#include "GuiUserMessages.h"


// TODO: reference additional headers your program requires here
