#include "RectUtils.h"


#if EG_WIN

void SetRect( Rect* inR, long left, long top, long right, long bot ) {

	inR -> top		= top;
	inR -> left		= left;
	inR -> bottom	= bot;
	inR -> right	= right;
}


void InsetRect( Rect* inR, int inDelX, int inDelY ) {

	inR -> left		+= inDelX;
	inR -> right	-= inDelX;
	inR -> bottom	-= inDelY;
	inR -> top		+= inDelY;
}



void OffsetRect( Rect* inR, int inDelX, int inDelY ) {

	inR -> left		+= inDelX;
	inR -> right	+= inDelX;
	inR -> bottom	+= inDelY;
	inR -> top		+= inDelY;
}



void UnionRect( const Rect* inR1, const Rect* inR2, Rect* outRect ) {
	long l, t, b;

	l					= ( inR1 -> left < inR2 -> left ) ? inR1 -> left : inR2 -> left;
	t					= ( inR1 -> top < inR2 -> top ) ? inR1 -> top : inR2 -> top;
	b					= ( inR1 -> bottom < inR2 -> bottom ) ? inR2 -> bottom : inR1 -> bottom;
	outRect -> right	= ( inR1 -> right < inR2 -> right ) ? inR2 -> right : inR1 -> right;

	outRect -> left		= l;
	outRect -> top		= t;
	outRect -> bottom	= b;
}




void SectRect( const Rect* inR1, const Rect* inR2, Rect* outRect ) {
	long l, t, b;

	l					= ( inR1 -> left > inR2 -> left ) ? inR1 -> left : inR2 -> left;
	t					= ( inR1 -> top > inR2 -> top ) ? inR1 -> top : inR2 -> top;
	b					= ( inR1 -> bottom > inR2 -> bottom ) ? inR2 -> bottom : inR1 -> bottom;
	outRect -> right	= ( inR1 -> right > inR2 -> right ) ? inR2 -> right : inR1 -> right;
	
	outRect -> left		= l;
	outRect -> top		= t;
	outRect -> bottom	= b;
}


short PtInRect( const Point& inPt, const Rect* inRect ) {
	if ( inPt.h > inRect -> left && inPt.h <= inRect -> right ) {
		if ( inPt.v > inRect -> top && inPt.v <= inRect -> bottom )
			return true;
	}
	
	return false;
}

#endif



void InsetRect( LongRect* inR, int inDelX, int inDelY ) {

	inR -> left		+= inDelX;
	inR -> right	-= inDelX;
	inR -> bottom	-= inDelY;
	inR -> top		+= inDelY;
}

#define __short( dest, n )	if ( n > 32000 )					\
								dest = 32000;					\
							else if ( n + 32000 <= 0 )			\
								dest = - 32000;					\
							else								\
								dest = n;

/*
#define __short( dest, n )	if ( n > 0x7FFF )					\
								dest = 0x7FFF;					\
							else if ( n + 0x7FFF <= 0 )			\
								dest = - 0x7FFF;				\
							else								\
								dest = n;

*/

void SetRect( Rect* ioRect, const LongRect* inRect ) {
	__short( ioRect -> left, inRect -> left )
	__short( ioRect -> top, inRect -> top )
	__short( ioRect -> right, inRect -> right )
	__short( ioRect -> bottom, inRect -> bottom )
}


void SetRect( LongRect* ioRect, const Rect* inRect ) {
	ioRect -> left = inRect -> left;
	ioRect -> top = inRect -> top;
	ioRect -> right = inRect -> right;
	ioRect -> bottom = inRect -> bottom;
}



void UnionPt( long x, long y, Rect* ioRect ) {

	if ( ioRect -> left > x )
		ioRect -> left = x;
	if ( ioRect -> right < x )
		ioRect -> right = x;
		
	if ( ioRect -> top > y )
		ioRect -> top = y;
	if ( ioRect -> bottom < y )
		ioRect -> bottom = y;
}
