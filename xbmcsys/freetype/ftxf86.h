/***************************************************************************/
/*                                                                         */
/*  ftxf86.h                                                               */
/*                                                                         */
/*    Support functions for X11.                                           */
/*                                                                         */
/*  Copyright 2002, 2003, 2004 by                                          */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __FTXF86_H__
#define __FTXF86_H__

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef FREETYPE_H
#error "freetype.h of FreeType 1 has been loaded!"
#error "Please fix the directory search order for header files"
#error "so that freetype.h of FreeType 2 is found first."
#endif


FT_BEGIN_HEADER

  /* this comment is intentionally disabled for now, to prevent this       */
  /* function from appearing in the API Reference.                         */

  /*@***********************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_X11_Font_Format                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Return a string describing the format of a given face as an X11    */
  /*    FONT_PROPERTY.  It should only be used by the FreeType 2 font      */
  /*    backend of the XFree86 font server.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: Input face handle.                                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Font format string.  NULL in case of error.                        */
  /*                                                                       */
  FT_EXPORT( const char* )
  FT_Get_X11_Font_Format( FT_Face  face );

 /* */

FT_END_HEADER

#endif /* __FTXF86_H__ */
