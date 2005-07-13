// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define DEBUG_MOUSE
#define DEBUG_KEYBOARD
#include <xtl.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <queue>
#include "stdstring.h"
#include "../xbmc/StringUtils.h"
#include "../xbmc/memutil.h"
using namespace std;

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#include "xbutil.h"
#include "../xbmc/utils/log.h"

// guilib internal
#include "gui3d.h"
#include "tinyxml/tinyxml.h"

// TODO: reference additional headers your program requires here
