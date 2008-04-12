/***************************************************************************/
/*                                                                         */
/*  winfnt.h                                                               */
/*                                                                         */
/*    FreeType font driver for Windows FNT/FON files                       */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004 by                               */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __WINFNT_H__
#define __WINFNT_H__


#include <ft2build.h>
#include FT_WINFONTS_H
#include FT_INTERNAL_DRIVER_H


FT_BEGIN_HEADER

  typedef struct  WinMZ_HeaderRec_
  {
    FT_UShort  magic;
    /* skipped content */
    FT_UShort  lfanew;

  } WinMZ_HeaderRec;


  typedef struct  WinNE_HeaderRec_
  {
    FT_UShort  magic;
    /* skipped content */
    FT_UShort  resource_tab_offset;
    FT_UShort  rname_tab_offset;

  } WinNE_HeaderRec;


  typedef struct  WinNameInfoRec_
  {
    FT_UShort  offset;
    FT_UShort  length;
    FT_UShort  flags;
    FT_UShort  id;
    FT_UShort  handle;
    FT_UShort  usage;

  } WinNameInfoRec;


  typedef struct  WinResourceInfoRec_
  {
    FT_UShort  type_id;
    FT_UShort  count;

  } WinResourceInfoRec;


#define WINFNT_MZ_MAGIC  0x5A4D
#define WINFNT_NE_MAGIC  0x454E


  typedef struct  FNT_FontRec_
  {
    FT_ULong             offset;
    FT_Int               size_shift;

    FT_WinFNT_HeaderRec  header;

    FT_Byte*             fnt_frame;
    FT_ULong             fnt_size;
    FT_String*           family_name;

  } FNT_FontRec, *FNT_Font;


  typedef struct  FNT_FaceRec_
  {
    FT_FaceRec     root;
    FNT_Font       font;

    FT_CharMap     charmap_handle;
    FT_CharMapRec  charmap;  /* a single charmap per face */

  } FNT_FaceRec, *FNT_Face;


  FT_EXPORT_VAR( const FT_Driver_ClassRec )  winfnt_driver_class;


FT_END_HEADER


#endif /* __WINFNT_H__ */


/* END */
