#include "goomsl_heap.h"
#include <stdlib.h>

struct _GOOM_HEAP {
  void **arrays;
  int number_of_arrays;
  int size_of_each_array;
  int consumed_in_last_array;
};

/* Constructors / Destructor */
GoomHeap *goom_heap_new(void)
{
  return goom_heap_new_with_granularity(4096);
}

GoomHeap *goom_heap_new_with_granularity(int granularity)
{
  GoomHeap *_this;
  _this = (GoomHeap*)malloc(sizeof(GoomHeap));
  _this->number_of_arrays   = 0;
  _this->size_of_each_array = granularity;
  _this->consumed_in_last_array = 0;
  _this->arrays = (void**)malloc(sizeof(void*));
  return _this;
}

void goom_heap_delete(GoomHeap *_this)
{
  int i;
  for (i=0;i<_this->number_of_arrays;++i) {
    free(_this->arrays[i]);
  }
  free(_this->arrays);
  free(_this);
}

static void align_it(GoomHeap *_this, int alignment)
{
  if ((alignment > 1) && (_this->number_of_arrays>0)) {
    void *last_array = _this->arrays[_this->number_of_arrays - 1];
    int   last_address = (int)last_array + _this->consumed_in_last_array;
    int   decal = (last_address % alignment);
    if (decal != 0) {
      _this->consumed_in_last_array += alignment - decal;
    }
  }
}

void     *goom_heap_malloc_with_alignment_prefixed(GoomHeap *_this, int nb_bytes,
                                                   int alignment, int prefix_bytes)
{
  void *retval = NULL;
  
  /* d'abord on gere les problemes d'alignement */
  _this->consumed_in_last_array += prefix_bytes;
  align_it(_this, alignment);

  /* ensuite on verifie que la quantite de memoire demandee tient dans le buffer */
  if ((_this->consumed_in_last_array + nb_bytes >= _this->size_of_each_array)
      || (_this->number_of_arrays == 0)) {

    if (prefix_bytes + nb_bytes + alignment >= _this->size_of_each_array) {

      /* Si la zone demandee est plus grosse que la granularitee */
      /* On alloue un buffer plus gros que les autres */
      _this->arrays = (void**)realloc(_this->arrays, sizeof(void*) * (_this->number_of_arrays+2));
      
      _this->number_of_arrays += 1;
      _this->consumed_in_last_array = prefix_bytes;
      
      _this->arrays[_this->number_of_arrays - 1] = malloc(prefix_bytes + nb_bytes + alignment);
      align_it(_this,alignment);
      retval = (void*)((char*)_this->arrays[_this->number_of_arrays - 1] + _this->consumed_in_last_array);

      /* puis on repart sur un nouveau buffer vide */
      _this->number_of_arrays += 1;
      _this->consumed_in_last_array = 0;
      _this->arrays[_this->number_of_arrays - 1] = malloc(_this->size_of_each_array);
      return retval;
    }
    else {
      _this->number_of_arrays += 1;
      _this->consumed_in_last_array = prefix_bytes;
      _this->arrays = (void**)realloc(_this->arrays, sizeof(void*) * _this->number_of_arrays);

      _this->arrays[_this->number_of_arrays - 1] = malloc(_this->size_of_each_array);
      align_it(_this,alignment);
    }
  }
  retval = (void*)((char*)_this->arrays[_this->number_of_arrays - 1] + _this->consumed_in_last_array);
  _this->consumed_in_last_array += nb_bytes;
  return retval;
}

void *goom_heap_malloc_with_alignment(GoomHeap *_this, int nb_bytes, int alignment)
{
  return goom_heap_malloc_with_alignment_prefixed(_this, nb_bytes, alignment, 0);
}

void *goom_heap_malloc(GoomHeap *_this, int nb_bytes)
{
  return goom_heap_malloc_with_alignment(_this,nb_bytes,1);
}

