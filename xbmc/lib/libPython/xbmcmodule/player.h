#include "..\python.h"
#include "PythonPlayer.h"
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	typedef struct {
    PyObject_HEAD
		int iPlayList;
		CPythonPlayer* pPlayer;
	} Player;

	extern PyTypeObject Player_Type;
}

#ifdef __cplusplus
}
#endif
