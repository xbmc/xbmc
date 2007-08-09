/*
==============================================================================
|  GLF Library
|  Version 1.0 (Release)
| 
|  Author: Roman Podobedov
|  Email: romka@ut.ee
|  WEB: www.ut.ee/~romka
|  Date: 17 August 2000
|  
|  Copyright (C) 2000, Romka Graphics
|  This library is freely distributable without any license or permissions.
|  You can use this library in any program (commercial, educational
|  or individual), but in each program, where You use this library, You 
|  should to keep this header (author name and coordinates)!
==============================================================================
*/
#ifndef GLF_H
#define GLF_H

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS */

#define GLF_ERROR           -1
#define GLF_OK               0
#define GLF_YES              1
#define GLF_NO               2

#define GLF_CONSOLE_MESSAGES 10
#define GLF_TEXTURING        11
#define GLF_CONTOURING       12

#define GLF_LEFT_UP          20
#define GLF_LEFT_CENTER      21
#define GLF_LEFT_DOWN        22
#define GLF_CENTER_UP        23
#define GLF_CENTER_CENTER    24
#define GLF_CENTER_DOWN      25
#define GLF_RIGHT_UP         26
#define GLF_RIGHT_CENTER     27
#define GLF_RIGHT_DOWN       28
#define GLF_CENTER           24

/* Console constants */
#define GLF_CONSOLE_CURSOR   30

 /* Library initialization (must be called before any usage of library) */
void glfInit(); 

/* Font loading/unloading functions */
int  glfLoadFont(char *font_name);        /* Load font to memory*/
int  glfUnloadFont();                     /* Unload font from memory */
int  glfUnloadFontD(int font_descriptor); /* Unload font by font_descriptor */

/* Text drawing functions */
void glfDrawWiredSymbol(char s);    /* Draw wired symbol    */
void glfDrawWiredString(char *s);   /* Draw wired string    */
void glfDrawSolidSymbol(char s);    /* Draw solid symbol    */
void glfDrawSolidString(char *s);   /* Draw wired string    */
void glfDraw3DWiredSymbol(char s);  /* Draw 3D wired symbol */
void glfDraw3DWiredString(char *s); /* Draw 3D wired string */
void glfDraw3DSolidSymbol(char s);  /* Draw 3D solid symbol */
void glfDraw3DSolidString(char *s); /* Draw 3D solid string */

/* Text drawing functions by specified font */
void glfDrawWiredSymbolF(int font_descriptor, char s);
void glfDrawWiredStringF(int font_descriptor, char *s);
void glfDrawSolidSymbolF(int font_descriptor, char s);
void glfDrawSolidStringF(int font_descriptor, char *s);
void glfDraw3DWiredSymbolF(int font_descriptor, char s);
void glfDraw3DWiredStringF(int font_descriptor, char *s);
void glfDraw3DSolidSymbolF(int font_descriptor, char s);
void glfDraw3DSolidStringF(int font_descriptor, char *s);

/* Text control functions */
void glfGetStringBoundsF(int fd, char *s, float *minx, float *miny, float *maxx, float *maxy);
void glfGetStringBounds(char *s, float *minx, float *miny, float *maxx, float *maxy);

void  glfSetSymbolSpace(float sp);   /* Set space between symbols */
float glfGetSymbolSpace();           /* Get space between symbols */
void  glfSetSpaceSize(float sp);     /* Set space size            */
float glfGetSpaceSize();             /* Get current space size    */

void  glfSetSymbolDepth(float dpth); /* Set depth of 3D symbol */
float glfGetSymbolDepth();           /* Get depth of 3D symbol */
int   glfSetCurrentFont(int Font_Descriptor); /* Set current font */
int   glfGetCurrentFont();                    /* Get current font descriptor */

void  glfSetAnchorPoint(int anchp); /* Set symbol anchor point */
void glfSetContourColor(float r, float g, float b, float a); /* Contour color */
/* Enable/Disable GLF features */

void glfEnable(int what); /* Enable GLF feature 'what' */
void glfDisable(int what); /* Disable GLF feature 'what' */

/* Console functions */
void glfSetConsoleParam(int width, int height);
int  glfSetConsoleFont(int Font_Descriptor);
void glfConsoleClear();
void glfPrint(char *s, int lenght);
void glfPrintString(char *s);
void glfPrintChar(char s);

void glfConsoleDraw();
void glfSetCursorBlinkRate(int Rate);

#ifdef __cplusplus
};
#endif

#endif
