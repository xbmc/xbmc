#ifndef MEM_H
# define MEM_H

/* Use MHD__asn1_afree() when calling alloca, or
 * memory leaks may occur in systems which do not
 * support alloca.
 */
#ifdef HAVE_ALLOCA
# ifdef HAVE_ALLOCA_H
#  include <alloca.h>
# endif
# define MHD__asn1_alloca alloca
# define MHD__asn1_afree(x)
#else
# define MHD__asn1_alloca MHD__asn1_malloc
# define MHD__asn1_afree MHD__asn1_free
#endif /* HAVE_ALLOCA */

#define MHD__asn1_malloc malloc
#define MHD__asn1_free free
#define MHD__asn1_calloc calloc
#define MHD__asn1_realloc realloc
#define MHD__asn1_strdup strdup

#endif /* MEM_H */
