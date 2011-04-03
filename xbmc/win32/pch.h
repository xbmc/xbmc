#pragma once
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#if !(defined(_WINSOCKAPI_) || defined(_WINSOCK_H))
#include <winsock2.h>
#endif
#include <windows.h>
#include <mmsystem.h>
#include <TCHAR.H>
#include <locale>
#include <comdef.h>
#define DIRECTINPUT_VERSION 0x0800
#include "DInput.h"
#include "DSound.h"
#include "D3D9.h"
#include "D3DX9.h"
#include "boost/shared_ptr.hpp"
#include "SDL\SDL.h"
// anything below here should be headers that very rarely (hopefully never)
// change yet are included almost everywhere.
#include "utils/StdString.h"
