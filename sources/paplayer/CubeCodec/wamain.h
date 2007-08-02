/*

  in_cube Gamecube Stream Player for Winamp
  by hcs

  includes work by Destop and bero

*/

// winamp and user interface

#include "cube.h"

#ifndef _WAMAIN_H

#define _WAMAIN_H
 
#ifdef _LINUX
#define __cdecl
#endif

void __cdecl DisplayError (char * Message, ...);

extern int looptimes;
extern int fadelength;
extern int fadedelay;
extern CUBEFILE cubefile;

#endif

