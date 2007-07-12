
#ifndef __XMEMUTILS__H__
#define __XMEMUTILS__H__


// aligned memory allocation and free. memory returned will be aligned to "alignTo" bytes.
// this is a linux (actually platfom free) implementation of the win32 CRT methods _aligned_malloc and _aligned_free.
void *_aligned_malloc(size_t s, size_t alignTo);
void _aligned_free(void *p) ;

#endif

