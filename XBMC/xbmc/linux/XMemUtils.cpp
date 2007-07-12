#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "XMemUtils.h"

#undef ALIGN
#define ALIGN(value, alignment) (((value)+(alignment-1))&~(alignment-1))

// aligned memory allocation.
// in order to do so - we alloc extra space and store the original allocation in it (so that we can free later on).
// the returned address will be the nearest alligned address within the space allocated.
void *_aligned_malloc(size_t s, size_t alignTo) {

  char *pFull = (char*)malloc(s + alignTo + sizeof(char *));
  char *pAlligned = (char *)ALIGN(((unsigned long)pFull + sizeof(char *)), alignTo);

  *(char **)(pAlligned - sizeof(char*)) = pFull;

  return(pAlligned);
}

void _aligned_free(void *p) {
  if (!p)
    return;

  char *pFull = *(char **)(((char *)p) - sizeof(char *)); 
  free(pFull);
}


