#ifndef __RectUtils__
#define __RectUtils__

#include "..\Eg Common.h"

#if EG_WIN
extern short 		PtInRect( const Point& inPt, const Rect* inRect );
extern void			SetRect( Rect* inR, long left, long top, long right, long bot );
extern void			InsetRect( Rect* inR, int inDelX, int inDelY );
extern void			UnionRect( const Rect* inR1, const Rect* inR2, Rect* outRect );
extern void			OffsetRect( Rect* inRect, int inDelX, int inDelY );
extern void			SectRect( const Rect* inR1, const Rect* inR2, Rect* outRect );
#endif

#if EG_MAC
#include <QuickDraw.h>
#endif

extern void			UnionPt( long x, long y, Rect* ioRect );
extern void			SetRect( Rect* ioRect, const LongRect* inRect );
extern void			SetRect( LongRect* ioRect, const Rect* inRect );
extern void			InsetRect( LongRect* inR, int inDelX, int inDelY );

#endif