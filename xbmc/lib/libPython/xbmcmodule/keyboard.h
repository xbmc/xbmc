#include "..\python.h"
#pragma once

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	typedef struct {
    PyObject_HEAD
		string strDefault;
	} Keyboard;

	extern PyTypeObject Keyboard_Type;
}

#ifdef __cplusplus
}
#endif
