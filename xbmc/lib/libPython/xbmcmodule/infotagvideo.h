#include "..\python.h"
#include "..\..\..\utils\IMDB.h"

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	typedef struct {
    PyObject_HEAD
		CIMDBMovie infoTag;
	} InfoTagVideo;

	extern PyTypeObject InfoTagVideo_Type;
	extern InfoTagVideo* InfoTagVideo_FromCIMDBMovie(const CIMDBMovie& infoTag);
}

#ifdef __cplusplus
}
#endif
