#include "fribidi.h"

#define	mk_wcwidth	FRIBIDI_API fribidi_wcwidth
#define	mk_wcswidth	FRIBIDI_API fribidi_wcswidth
#define	mk_wcwidth_cjk	FRIBIDI_API fribidi_wcwidth_cjk
#define	mk_wcswidth_cjk	FRIBIDI_API fribidi_wcswidth_cjk

#define wchar_t		FriBidiChar
#define size_t		FriBidiStrIndex
