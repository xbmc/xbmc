#include "../python/Python.h"
#include "PythonPlayer.h"
#include "../../../cores/PlayerCoreFactory.h"
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
    EPLAYERCORES playerCore;
  } Player;

  extern PyTypeObject Player_Type;
  void initPlayer_Type();
}

#ifdef __cplusplus
}
#endif
