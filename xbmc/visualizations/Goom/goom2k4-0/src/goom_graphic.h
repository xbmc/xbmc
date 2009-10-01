#ifndef GRAPHIC_H
#define GRAPHIC_H

typedef unsigned int Uint;

typedef struct
{
  unsigned short r, v, b;
}
Color;

extern const Color BLACK;
extern const Color WHITE;
extern const Color RED;
extern const Color BLUE;
extern const Color GREEN;
extern const Color YELLOW;
extern const Color ORANGE;
extern const Color VIOLET;


#ifdef COLOR_BGRA

#define R_CHANNEL 0xFF000000
#define G_CHANNEL 0x00FF0000
#define B_CHANNEL 0x0000FF00
#define A_CHANNEL 0x000000FF
#define R_OFFSET  24
#define G_OFFSET  16
#define B_OFFSET  8
#define A_OFFSET  0

typedef union _PIXEL {
  struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
  } channels;
  unsigned int val;
  unsigned char cop[4];
} Pixel;

#else

#define A_CHANNEL 0xFF000000
#define R_CHANNEL 0x00FF0000
#define G_CHANNEL 0x0000FF00
#define B_CHANNEL 0x000000FF
#define A_OFFSET  24
#define R_OFFSET  16
#define G_OFFSET  8
#define B_OFFSET  0

typedef union _PIXEL {
  struct {
    unsigned char a;
    unsigned char r;
    unsigned char g;
    unsigned char b;
  } channels;
  unsigned int val;
  unsigned char cop[4];
} Pixel;

#endif /* COLOR_BGRA */

/*
inline void setPixelRGB (Pixel * buffer, Uint x, Uint y, Color c);
inline void getPixelRGB (Pixel * buffer, Uint x, Uint y, Color * c);
*/


#endif /* GRAPHIC_H */
