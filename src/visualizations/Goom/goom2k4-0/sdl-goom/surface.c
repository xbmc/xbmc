#include "surface.h"
#include <stdlib.h>

Surface * surface_new (int w, int h) {
  Surface * s = (Surface*)malloc(sizeof(Surface));
  s->realstart = (int*)malloc(w*h*4 + 128);
  s->buf = (int*)((int)s->realstart + 128 - (((int)s->realstart) % 128));
  s->size = w*h;
  s->width = w;
  s->height = h;
  return s;
}

void surface_delete (Surface **s) {
  free ((*s)->realstart);
  free (*s);
  *s = NULL;
}
