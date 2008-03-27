#ifndef EG_COMMON_H
#define EG_COMMON_H


#define EG_WIN		1
#define EG_WIN32	1

#ifndef true
#define true -1
#define false 0
#endif

#ifndef NULL
#define NULL 0L
#endif

#ifndef Rect_Defined
struct Rect {
	short left, top, right, bottom;
};
#define Rect_Defined
#endif

typedef long KeyMap[4];

struct Point {
	short v, h;
};
	
struct RGBColor {
	unsigned short red, green, blue;
};

#define __winRGB( r, g, b )  (( r >> 8 ) | ( g & 0xFF00 ) | (( b & 0xFF00 ) << 8 ))


typedef struct tagRGBQUAD {
	unsigned char    rgbBlue;
	unsigned char    rgbGreen;
	unsigned char    rgbRed;
	unsigned char    rgbReserved;
} RGBQUAD;


struct ColorSpec {
	short		value;
	RGBColor	rgb;
};


struct LongRect {
	long left, top, right, bottom;
};


#endif