#include "../python/Python.h"
#include "../../../utils/IMDB.h"

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  typedef struct {
    PyObject_HEAD
    CVideoInfoTag infoTag;
  } InfoTagVideo;

  extern PyTypeObject InfoTagVideo_Type;
  extern InfoTagVideo* InfoTagVideo_FromCVideoInfoTag(const CVideoInfoTag& infoTag);
  void initInfoTagVideo_Type();
}

#ifdef __cplusplus
}
#endif
