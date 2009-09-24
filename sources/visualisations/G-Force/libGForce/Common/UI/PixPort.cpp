#include "PixPort.h"

#include <math.h>
#include "..\general tools\RectUtils.h"

long		PixPort::sTempSize			= 0;
char*		PixPort::sTemp				= NULL;

PixPort::PixPort() {
	mX = 0;
	mY = 0;
	mLineWidth = 1;
	mBackColor	= 0;
	mBytesPerPix = 0;
	mCurFontID = 0;
	mDeviceLineHeight = 0;
	mBits = NULL;
}



PixPort::~PixPort() {
	PixTextStyle* font;
	int i;
	
	Un_Init();

	// Delete any info structures we may have created
	for ( i = 0; i < mFonts.Count(); i++ ) {
		font = (PixTextStyle*) mFonts[ i ];
		delete font;
	}
	
	if ( sTemp ) {
		delete []sTemp;
		sTemp = NULL;
		sTempSize = 0;
	}
	if(mBits)
		delete[] mBits;
}




void PixPort::Un_Init() {
	// Invalidate the selected text style
	mCurFontID = -1;
}



void PixPort::SetClipRect( const Rect* inRect ) {
	mClipRect.top = 0;
	mClipRect.left = 0;
	mClipRect.right = mX;
	mClipRect.bottom = mY;

	if ( inRect )
		SectRect( inRect, &mClipRect, &mClipRect );		
}


void PixPort::SetClipRect( long inSX, long inSY, long inEX, long inEY ) {
	Rect r;
	SetRect( &r, inSX, inSY, inEX, inEY );
	SetClipRect( &r );
}


void PixPort::Init( int inWidth, int inHeight, int inDepth ) {

	//inDepth ist IMMER 8 bei GForce...

	if ( inWidth < 0 ) inWidth = 0;
	if ( inHeight < 0 ) inHeight = 0;

	// Catch any invalid depth levels.
	if ( inDepth != 32 && inDepth != 16 && inDepth != 8 )
		inDepth = 16;
	
//>	if ( inDepth < ScreenDevice::sMinDepth )
//>		inDepth = ScreenDevice::sMinDepth;

	mX			= 4 * (( inWidth + 3 ) / 4 );
	mY			= inHeight;

	Un_Init();

	if(mBits)
		delete[] mBits;

	mBits = new char[inWidth*inHeight];	//da inDepth immer 8Bits ist...

	mBytesPerRow	= inWidth*(inDepth/8);
	mBytesPerPix	= inDepth / 8;

	SetClipRect();

	EraseRect();
}

#define __clipPt( x, y )	\
	if ( x < mClipRect.left )			\
		x = mClipRect.left;				\
	else if ( x > mClipRect.right )		\
		x = mClipRect.right;			\
	if ( y < mClipRect.top )			\
		y = mClipRect.top;				\
	else if ( y > mClipRect.bottom )	\
		y = mClipRect.bottom;

#define __clipRect( inRect )			\
	Rect r = inRect;					\
	__clipPt( r.left, r.top )			\
	__clipPt( r.right, r.bottom )		\
	long width 	= r.right - r.left;		\
	long height = r.bottom - r.top;


long PixPort::GetPortColor( long inR, long inG, long inB ) {
	int bitDepth = mBytesPerPix << 3;
	long c;
	if ( inR > 0xFFFF )	inR = 0xFFFF;
	if ( inG > 0xFFFF )	inG = 0xFFFF;
	if ( inB > 0xFFFF )	inB = 0xFFFF;
	if ( inR < 0 )		inR = 0;
	if ( inG < 0 )		inG = 0;
	if ( inB < 0 )		inB = 0;
	
	if ( bitDepth == 32 ) 
		c = __Clr32( inR, inG, inB );
	else if ( bitDepth == 16 )
		c = __Clr16( inR, inG, inB );
	else
		c = __Clr8( inR, inG, inB );

	return c;
}


long PixPort::SetBackColor( const RGBColor& RGB ) {
	mBackColor = GetPortColor( RGB );
	return mBackColor;
}


long PixPort::SetBackColor( long inR, long inG, long inB ) {
	mBackColor = GetPortColor( inR, inG, inB );
	return mBackColor;
}

void PixPort::SetPalette( PixPalEntry inPal[ 256 ] ) {
	int i;
	for(i=0; i<256; i++)
		mDataPalette[i] = inPal[i];
}


void PixPort::GaussBlur( int inBoxWidth, const Rect& inRect, void* inDestBits ) {	
	// Don't let us draw in random parts of memory -- clip inRect
	__clipRect( inRect )
	
	if ( inBoxWidth <= 1 )
		return;
	
	// 3 box convolutions, 3 colors per pixel, 4 bytes per color
	long 	boxTempSize	= 36 * inBoxWidth;
	char*	tempBits	= NULL;
	unsigned long*	boxTemp;
	long	imgOffset	= mBytesPerPix * r.left + r.top * mBytesPerRow;
	long	bytesNeeded	= mBytesPerRow * (mY + 2) + boxTempSize;
	
	// Resort to app's heap for temp mem if failed temp mem attempt or in win32
	tempBits = mBlurTemp.Dim( bytesNeeded );

	// Have the box temp and the pixel temp rgns use the same handle
	boxTemp = (unsigned long*) tempBits;
	tempBits += boxTempSize;
	
	if ( ! inDestBits )
		inDestBits = mBits;
		
	// Do a box blur on the x axis, transposing the source rgn to the dest rgn
	// Then o a box blur on the transposed image, effectively blurring the y cords, transposing it to the dest
	if ( mBytesPerPix == 2 )  {	
		BoxBlur16( ( mBits + imgOffset), tempBits, inBoxWidth, width, height, mBytesPerRow, mBytesPerPix*height, boxTemp, mBackColor );
		BoxBlur16( tempBits, ((char*) inDestBits + imgOffset), inBoxWidth, height, width, mBytesPerPix*height, mBytesPerRow, boxTemp, mBackColor );  }
	else if ( mBytesPerPix == 4 ) {
		BoxBlur32( ( mBits + imgOffset), tempBits, inBoxWidth, width, height, mBytesPerRow, mBytesPerPix*height, boxTemp, mBackColor );
		BoxBlur32( tempBits, ((char*) inDestBits + imgOffset), inBoxWidth, height, width, mBytesPerPix*height, mBytesPerRow, boxTemp, mBackColor ); 
	}
}



void PixPort::CrossBlur( const Rect& inRect ) {	
	// Don't let us draw in random parts of memory -- clip inRect
	__clipRect( inRect )
	
	// 3 box convolutions, 3 colors per pixel, 4 bytes per color
	long	imgOffset	= mBytesPerPix * r.left + r.top * mBytesPerRow;
	
	unsigned char* tempBits = (unsigned char*) mBlurTemp.Dim( mX * 3 );
		
	if ( mBytesPerPix == 2 ) 
		CrossBlur16( ( mBits + imgOffset), width, height, mBytesPerRow, tempBits ); 
	else if ( mBytesPerPix == 4 )
		CrossBlur32( ( mBits + imgOffset), width, height, mBytesPerRow, tempBits );
}



void PixPort::CopyBits( unsigned long *frameBuffer, int pitch ) {

	//Oki, gleich auf 32bit konvertieren...

	long palette[256];
	for(int i=0; i<256; i++) {
		palette[i] = (mDataPalette[i].rgbRed<<16) | (mDataPalette[i].rgbGreen<<8)
			| mDataPalette[i].rgbBlue;
	}

	int x,y;
	unsigned long *work = frameBuffer;
	unsigned char *srcBuffer = (unsigned char*)mBits;
	for(y = 0; y < mY; y++) {
		unsigned long *pixel = work;
		for(x = 0; x < mX; x++) {
			*(pixel++) = palette[*(srcBuffer++)];
		}
		work += pitch/4;
	}

//>		::BitBlt( inPort, inDest -> left, inDest -> top, inDest -> right - inDest -> left, inDest -> bottom - inDest -> top, mWorld, inSrce -> left, inSrce -> top, SRCCOPY );

}


void PixPort::Line( int sx, int sy, int ex, int ey, long inColor ) {
	
	if ( mBytesPerPix == 2 ) 
		Line16( sx, sy, ex, ey, inColor );
	else if ( mBytesPerPix == 1 )
		Line8 ( sx, sy, ex, ey, inColor );
	else if ( mBytesPerPix == 4 ) 
		Line32( sx, sy, ex, ey, inColor );
}


#define __ABS( n )  ( ( n > 0 ) ? (n) : (-n) )
#define CLR_LINE_THR	520

void PixPort::Line( int sx, int sy, int ex, int ey, const RGBColor& inS, const RGBColor& inE ) {
	long R, G, B, dR, dG, dB;
	
	R = inS.red;
	G = inS.green;
	B = inS.blue;
	dR = inE.red - R;
	dG = inE.green - G;
	dB = inE.blue - B;
	
	// If the endpoints have the same color, run the faster line procs (that just use one color)
	if (	dR > - CLR_LINE_THR && dR < CLR_LINE_THR &&
			dG > - CLR_LINE_THR && dG < CLR_LINE_THR &&
			dB > - CLR_LINE_THR && dB < CLR_LINE_THR ) {
		long color;

		if ( mBytesPerPix == 2 ) {
			color = __Clr16( R, G, B );
			Line16( sx, sy, ex, ey, color ); }
		else if ( mBytesPerPix == 4 ) {
			color = __Clr32( R, G, B );
			Line32( sx, sy, ex, ey, color ); }
		else if ( mBytesPerPix == 1 ) {
			color = __Clr8( R, G, B );
			Line8 ( sx, sy, ex, ey, color );
		} }
	else {
		if ( mBytesPerPix == 2 ) 
			Line16( sx, sy, ex, ey, inS, dR, dG, dB );
		else if ( mBytesPerPix == 4 ) 
			Line32( sx, sy, ex, ey, inS, dR, dG, dB );
		else if ( mBytesPerPix == 1 )
			Line8 ( sx, sy, ex, ey, inS.red, dR );
	}
}



long PixPort::CreateFont() {
	PixTextStyle* newFont = new PixTextStyle;
	mFonts.Add( newFont );
	newFont -> mOSFontID = 0;
	return (long) newFont;
}



void PixPort::AssignFont( long inPixFontID, const char* inFontName, long inSize, long inStyleFlags ) {
	PixTextStyle* font = (PixTextStyle*) inPixFontID;
	
	font -> mFontName.Assign( inFontName );
	font -> mPointSize			= inSize;
	font -> mStyle				= inStyleFlags;
	font -> mOSStyle			= 0;
	font -> mDeviceLineHeight	= inSize;

//>	long height = - MulDiv( inSize, ::GetDeviceCaps( mWorld, LOGPIXELSY ), 72 );
	font -> mFontName.Keep( 31 );
//>	font -> mOSFontID = (long) ::CreateFont( height, 0, 0, 0, 
//>				( inStyleFlags & PP_BOLD ) ? FW_BOLD : FW_NORMAL,
//>				( inStyleFlags & PP_ITALIC ) ? true : false,
//>				( inStyleFlags & PP_UNDERLINE ) ? true : false, 
//>				 0, DEFAULT_CHARSET, 
//>					OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
//>					DEFAULT_PITCH | FF_DONTCARE, font -> mFontName.getCStr() );
}






void PixPort::SelectFont( long inPixFontID ) {
	
	// Exit if we're already in in this text face
	if ( inPixFontID == mCurFontID )
		return;
		
	mCurFontID = inPixFontID;
	PixTextStyle* font = (PixTextStyle*) inPixFontID;
	mDeviceLineHeight = font -> mDeviceLineHeight;
	
//>	::SelectObject( mWorld, (HFONT) font -> mOSFontID );
}



void PixPort::SetTextMode( PixDrawMode inMode ) {
//>	long mode = R2_COPYPEN;
//>	if ( inMode == SRC_BIC )
//>		mode = R2_WHITE;
//>	else if ( inMode == SRC_XOR )
//>		mode = R2_NOT;
//>	::SetROP2( mWorld, mode );
}


void PixPort::SetTextColor( RGBColor& inColor ) {
//>	::SetTextColor( mWorld, RGB( inColor.red >> 8, inColor.green >> 8, inColor.blue >> 8 ) );
}


void PixPort::SetTextColor( PixPalEntry& inColor ) {
//>	::SetTextColor( mWorld, RGB( inColor.rgbRed, inColor.rgbGreen, inColor.rgbBlue ) );
}


void PixPort::TextRect( const char* inStr, long& outWidth, long& outHeight ) {
	long width, pos;
	char c;
	
	outWidth  = 0;
	outHeight = 0;
	
	while ( *inStr ) {
		c = inStr[ 0 ];
		pos = 0;
		
		while ( c != '\r' && c ) {
			pos++;
			c = inStr[ pos ];
		}
//>		SIZE dim;
//>		::GetTextExtentPoint( mWorld, inStr, pos, &dim );
//>		width  = dim.cx;
		
		width = 5;	//DebugCode :)

		if ( width > outWidth )
			outWidth = width;
			
		outHeight += mDeviceLineHeight;

		if ( c == 0 )
			break;

		inStr += pos + 1;
	}
}



void PixPort::DrawText( long inX, long inY, const char* inStr ) {
	long pos;
	char c;
	
	while ( *inStr ) {
		c = inStr[ 0 ];
		pos = 0;
		
		while ( c != '\r' && c ) {
			pos++;
			c = inStr[ pos ];
		}
//>		::TextOut( mWorld, inX, inY, inStr, pos );
			
		if ( c == 0 )
			break;
			
		inY += mDeviceLineHeight;
		inStr += pos + 1;
	}
}


void PixPort::SetLineWidth( long inLineWidth ) {
	if ( inLineWidth <= 0 )
		mLineWidth = 1;
	else if ( inLineWidth > MAX_LINE_WIDTH )
		mLineWidth = MAX_LINE_WIDTH;
	else
		mLineWidth = inLineWidth;
}

void PixPort::EraseRect( const Rect* inRect ) {
	if ( mBytesPerPix == 2 )
		EraseRect16( inRect );
	else if ( mBytesPerPix == 1 )
		EraseRect8 ( inRect );
	else if ( mBytesPerPix == 4 )
		EraseRect32( inRect );
}




#define P_SZ	1
#include "DrawXX.cpp"

#undef P_SZ
#define P_SZ	2
#include "DrawXX.cpp"

#undef P_SZ
#define P_SZ	4
#include "DrawXX.cpp"

#define HALFCORD	0x007F  /* 16 bits per cord, 8 bits for fixed decimal, 8 bits for whole number */
#define FIXED_BITS	8


// Assembly note w/ branch prediction:  the first block is chosen to be more probable

void PixPort::Fade( const char* inSrce, char* inDest, long inBytesPerRow, long inX, long inY, unsigned long* grad ) {
	unsigned long x, y, u, v, u1, v1, P1, P2, P3, P4, p;
	const char* srceMap;
	const char* srce;

	// Setup the source row base address and offset to allow for negative grad components
	srce = inSrce - HALFCORD * inBytesPerRow - HALFCORD;
		
	// Start writing to the image...
	for ( y = 0; y < inY; y++ ) {

		for ( x = 0; x < inX; x++ ) {
		
			// Format of each long:
			// High byte: x (whole part), High-low byte: x (frac part)
			// Low-high byte: y (whole part), Low byte: y (frac part)
			u1 = *grad;		
			grad ++;

			p = 0;
			
			// 0xFFFFFFFF is a signal that this pixel is black.  
			if ( u1 != 0xFFFFFFFF )	{

				// Note that we use casting 3 times as an unsigned char to (smartly) get the compiler to do masking for us
				srceMap = srce + ( u1 >> 14 );
				v = ( u1 >> 7 ) & 0x7F;		// frac part of x
				u = ( u1      ) & 0x7F;		// frac part of y

				// In the end, the pixel intensity will be 31/32 of its current (interpolated) value
				v *= 31;
							
				/* Bilinear interpolation to approximate the source pixel value... */
				/* P1 - P2  */
				/* |     |  */
				/* P3 - P4  */
				P1  = ( (unsigned char*) srceMap )[0];
				P2  = ( (unsigned char*) srceMap )[1];
				u1	= 0x80 - u;
				P1  *= u1;
				P2  *= u1;
				v1 	=  3968 - v;  //  3968 == 31 * 0x80
				P3  = ( (unsigned char*) srceMap )[ inBytesPerRow ];
				P4  = ( (unsigned char*) srceMap )[ inBytesPerRow + 1 ];
				P3 *= u;
				P4 *= u;

				/* We can now calc the intensity of the pixel (truncating the fraction part of the pix value)  */
				/* We divide by (7+7+5) decimal places because p is units squared (7 places per decimal) and 5 more dec places cuz of the mult by 31 */
				p  = ( v * ( P2 + P4 ) + v1 * ( P1 + P3 ) ) >> 19;
			}
			( (unsigned char*) inDest )[ x ] = p;	
		}
		
		inDest	+= inBytesPerRow;
		srce	+= inBytesPerRow;
	}
}


