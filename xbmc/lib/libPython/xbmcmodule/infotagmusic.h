#include "..\python.h"
#include "..\..\..\musicInfoTag.h"

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	typedef struct {
    PyObject_HEAD
		MUSIC_INFO::CMusicInfoTag infoTag;
	} InfoTagMusic;

	extern PyTypeObject InfoTagMusic_Type;
	extern InfoTagMusic* InfoTagMusic_FromCMusicInfoTag(const MUSIC_INFO::CMusicInfoTag& infoTag);

}

#ifdef __cplusplus
}
#endif
