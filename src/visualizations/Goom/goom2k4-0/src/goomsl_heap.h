#ifndef GOOMSL_HEAP
#define GOOMSL_HEAP

/**
 * Resizable Array that guarranty that resizes don't change address of
 * the stored datas.
 *
 * This is implemented as an array of arrays... granularity is the size
 * of each arrays.
 */

typedef struct _GOOM_HEAP GoomHeap;

/* Constructors / Destructor */
GoomHeap *goom_heap_new(void);
GoomHeap *goom_heap_new_with_granularity(int granularity);
void      goom_heap_delete(GoomHeap *_this);

/* This method behaves like malloc. */
void     *goom_heap_malloc(GoomHeap *_this, int nb_bytes);
/* This adds an alignment constraint. */
void     *goom_heap_malloc_with_alignment(GoomHeap *_this, int nb_bytes, int alignment);

/* Returns a pointeur on the bytes... prefix is before */
void     *goom_heap_malloc_with_alignment_prefixed(GoomHeap *_this, int nb_bytes,
                                                   int alignment, int prefix_bytes);

#endif

