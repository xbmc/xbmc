 
#pragma once

#include "..\EG Common.h"

#include "..\general tools\UtilStr.h"
#include "..\general tools\TempMem.h"
#include "..\general tools\XPtrList.h"

enum {
	REL_8_ROW = 5,
	FIXED_16_ROW = 6
};

class DeltaFieldData {
public:
	const char*		mField;
};



class PixTextStyle {
	public:
	long					mPointSize;
	UtilStr					mFontName;
	long					mStyle;
	long					mDeviceLineHeight;
	long					mOSFontID;
	long					mOSStyle;	
};

enum PixDrawMode {
	SRC_OR,
	SRC_BIC,
	SRC_XOR
};

#define __Clr8(r,g,b)	( r >> 8 )
#define __Clr16(r,g,b)	((( ((unsigned long) r) & 0xF800) >> 1) | ((((unsigned long) g) & 0xF800) >> 6) | (((unsigned long) b) >> 11))
typedef RGBQUAD PixPalEntry;
#define	__Clr32(r,g,b)	__winRGB( b, g, r )

#define __ClrREF( rgb ) __Clr32( rgb.red, rgb.green, rgb.blue )



/*  PixPort is a platform indep drawing world/environment.  You set the size of one using
Init() and then execute whatever drawing commands. When the drawing is complete, CopyBits() 
copies pixel data from this world to the given destination OS graphics world.  */
	
#define		PP_PLAIN		0
#define 	PP_BOLD			0x1
#define		PP_ITALIC		0x2
#define		PP_UNDERLINE	0x4




class PixPort {
	public:
		//friend class PixPort;
								 PixPort();
		virtual					~PixPort();

		// One or the other must be called before a PixPort is used.  If inDepth is 0 or invalid,
		// the current OS depth is used.  Call Deactivate() or Init() to exit fullscreen mode.
		// inWin is passed because PixPort may need the window that's going fullscreen
		void					Init(  );
		void					Init( int inWidth, int inHeight, int inDepth = 0 );
		

		// Returns the current bit color depth from Init(). (8, 16, or 32)
		long					GetDepth()											{ return mBytesPerPix * 8; 	}
	
		// See how many bytes there are per row
		long					GetRowSize() 										{	return mBytesPerRow;	}
		
		
		//	Sets the background colors (for erase rect and Blur() )
		//  Returns the new pixel entry for the given color and this port
		//  Note:  For 8 bit ports, the red 0-2^16 component maps directly to 0-255 pixel value
		long					SetBackColor( const RGBColor& inColor );
		long					SetBackColor( long inR, long inG, long inB );

		//	Blurs the rect given in this image, with a given box filter of size (1=no blur)
		//	If the dest is NULL, the blur is applied to itself
		void					GaussBlur( int inBoxWidth, const Rect& inRect, void* inDestBits = NULL );	
		
		//	A different, more primitive blur that doesn't look as good as GaussBlur,
		void					CrossBlur( const Rect& inRect );

		// 	Sets the width of the pen
		void					SetLineWidth( long inWidth );
		
		//	Draw a line.  Use GetPortColor() to get a device color for a RGB
		void					Line( int sx, int sy, int ex, int ey, long inColor );
		void					Line( int sx, int sy, int ex, int ey, const RGBColor& inS, const RGBColor& inE );

		//	Sets the clip rgn for all drawing operations.  NULL for the cliprect means to remove all clipping.
		void					SetClipRect( const Rect* inRect = NULL );
		void					SetClipRect( long inSX, long inSY, long inEX, long inEY );
		
		//  Note:  For 8 bit ports, the red 0-2^16 component maps directly to 0-255 pixel value
		inline long				GetPortColor_inline( long inR, long inG, long inB ) {
			if ( mBytesPerPix == 2 )
				return __Clr16( inR, inG, inB );
			else if ( mBytesPerPix == 4 ) 
				return __Clr32( inB, inG, inR );
			else
				return __Clr8( inR, inG, inB );
		}
		
		void					SetTextMode( PixDrawMode inMode );
		void					SetTextColor( PixPalEntry& inColor );
		void					SetTextColor( RGBColor& inColor );
		void					DrawText( long inX, long inY, UtilStr& inStr )									{ DrawText( inX, inY, inStr.getCStr() ); 	}
		void					DrawText( long inX, long inY, const char* inStr );

		// See how big some text is going to be...
		void					TextRect( const char* inStr, long& outWidth, long& outHeight );
		
		//	Set the given rect to the current background color.  If no rect is specified, the entire port rect is cleared.
		void					EraseRect( const Rect* inRect = NULL );
		
		//	Copies the pixels in a rectangle from this pixel rect to the destination.  If the port is in fullscreen, inDestWin and inDest are ignored.
		void					CopyBits( unsigned long *frameBuffer, int pitch );
				
		//	Gets a port/device color for a given RGB		
		//  Note:  For 8 bit ports, the red 0-2^16 component maps directly to 0-255 pixel value
		long					GetPortColor( long inR, long inG, long inB );
		inline long				GetPortColor( const RGBColor& inColor )  		{ return GetPortColor( inColor.red, inColor.green, inColor.blue );  }
				
		//	The guts for G-Force...  This PixPort must be in 8-bit mode to do anything.
		void					Fade( DeltaFieldData* inGrad )									{ Fade( mBits, mBytesPerRow, mX, mY, inGrad ); } 
		void					Fade( PixPort& inDest, DeltaFieldData* inGrad )					{ Fade( mBits, inDest.mBits, mBytesPerRow, mX, mY, (unsigned long*) inGrad -> mField ); } 

		//	To draw in a new font, you:
		//		1) Call CreateFont() and PixPort will return you a fontID
		//		2) Call AssignFont() to set various font characteristic about it
		//		3) Call SelectFont() to set it as the current font for subsequent text drawing
		long					CreateFont();
		void					AssignFont( long inPixFontID, const char* inFontName, long inSize, long inStyleFlags = PP_PLAIN );
		void					SelectFont( long inPixFontID );

		void	SetPalette( PixPalEntry inPal[ 256 ] );


		long					GetX()			{ return mX; }
		long					GetY()			{ return mY; }
				
		
		static char*			sTemp;
		static long				sTempSize;


		#define MAX_LINE_WIDTH		32
	
		
	protected:
	
		Rect					mClipRect;
		long					mBytesPerPix;
		long					mBytesPerRow;
		long					mX, mY;
		long					mBackColor;
		long					mLineWidth;
		
		char*					mBits;
				
		TempMem					mBlurTemp;
		
		XPtrList				mFonts;
		long					mCurFontID;
		long					mDeviceLineHeight;
					
		void					Un_Init();
		
		void					EraseRect8 ( const Rect* inRect );
		void					EraseRect16( const Rect* inRect );
		void					EraseRect32( const Rect* inRect );

		static void				BoxBlur8 ( char* inSrce, char* inDest, int inBoxWidth, int inWidth, int inHeight, int inSrceRowSize, int inDestRowSize, unsigned long* temp, unsigned long inBackColor ); 
		static void				BoxBlur16( char* inSrce, char* inDest, int inBoxWidth, int inWidth, int inHeight, int inSrceRowSize, int inDestRowSize, unsigned long* temp, unsigned long inBackColor ); 
		static void				BoxBlur32( char* inSrce, char* inDest, int inBoxWidth, int inWidth, int inHeight, int inSrceRowSize, int inDestRowSize, unsigned long* temp, unsigned long inBackColor ); 

		static void				CrossBlur8 ( char* inSrce, int inWidth, int inHeight, int inBytesPerRow, unsigned char* inRowBuf );
		static void				CrossBlur16( char* inSrce, int inWidth, int inHeight, int inBytesPerRow, unsigned char* inRowBuf );
		static void				CrossBlur32( char* inSrce, int inWidth, int inHeight, int inBytesPerRow, unsigned char* inRowBuf );

		void					Line8 ( int sx, int sy, int ex, int ey, long inColor );
		void					Line16( int sx, int sy, int ex, int ey, long inColor );
		void					Line32( int sx, int sy, int ex, int ey, long inColor );

		void					Line8 ( int sx, int sy, int ex, int ey, long inR, long dR );
		void					Line16( int sx, int sy, int ex, int ey, const RGBColor& inS, long dR, long dG, long dB );
		void					Line32( int sx, int sy, int ex, int ey, const RGBColor& inS, long dR, long dG, long dB );

		static void				Fade( const char* inSrce, char* inDest, long inBytesPerRow, long inX, long inY, unsigned long* inGrad );
		static void				Fade( char* ioPix, long inBytesPerRow, long inX, long inY, DeltaFieldData* inGrad );

		PixPalEntry mDataPalette[ 256 ];
};