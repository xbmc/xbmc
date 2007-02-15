#include "..\python\python.h"
#include "window.h"
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{

	typedef struct {
    PyObject_HEAD_XBMC_WINDOW
	} WindowXML;


	extern PyTypeObject WindowXML_Type;
	
	void initWindowXML_Type();
}

#ifdef __cplusplus
}
#endif
