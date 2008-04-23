#ifndef _LINUX
#include "lib/libPython/python/Python.h"
#else
#include <python2.4/Python.h>
#endif

#include "MusicInfoTag.h"

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

  void initInfoTagMusic_Type();
}

#ifdef __cplusplus
}
#endif
