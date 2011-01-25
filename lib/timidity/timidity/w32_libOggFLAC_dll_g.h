

#ifndef __libOggFLAC_dll_g_h__
#define __libOggFLAC_dll_g_h__

#include "w32_libOggFLAC_dll_i.h"

/***************************************************************
   for header file of global definition
 ***************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif


extern int g_load_libOggFLAC_dll ( char *path );
extern void g_free_libOggFLAC_dll ( void );
#ifndef IGNORE_libOggFLAC_OggFLAC__StreamEncoderStateString
  extern OggFLAC_API const char * const*  g_libOggFLAC_OggFLAC__StreamEncoderStateString;
#define OggFLAC__StreamEncoderStateString (g_libOggFLAC_OggFLAC__StreamEncoderStateString)
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__StreamDecoderStateString
  extern OggFLAC_API const char * const*  g_libOggFLAC_OggFLAC__StreamDecoderStateString;
#define OggFLAC__StreamDecoderStateString (g_libOggFLAC_OggFLAC__StreamDecoderStateString)
#endif

#if defined(__cplusplus)
}  /* extern "C" { */
#endif
/***************************************************************/

#endif  /* __libOggFLAC_dll_g_h__ */

