/***************************************************************************/
/*                                                                         */
/*  t1types.h                                                              */
/*                                                                         */
/*    Basic Type1/Type2 type definitions and interface (specification      */
/*    only).                                                               */
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


#ifndef __T1TYPES_H__
#define __T1TYPES_H__


#include <ft2build.h>
#include FT_TYPE1_TABLES_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H
#include FT_INTERNAL_SERVICE_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /***                                                                   ***/
  /***                                                                   ***/
  /***              REQUIRED TYPE1/TYPE2 TABLES DEFINITIONS              ***/
  /***                                                                   ***/
  /***                                                                   ***/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    T1_EncodingRec                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure modeling a custom encoding.                            */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    num_chars  :: The number of character codes in the encoding.       */
  /*                  Usually 256.                                         */
  /*                                                                       */
  /*    code_first :: The lowest valid character code in the encoding.     */
  /*                                                                       */
  /*    code_last  :: The highest valid character code in the encoding.    */
  /*                                                                       */
  /*    char_index :: An array of corresponding glyph indices.             */
  /*                                                                       */
  /*    char_name  :: An array of corresponding glyph names.               */
  /*                                                                       */
  typedef struct  T1_EncodingRecRec_
  {
    FT_Int       num_chars;
    FT_Int       code_first;
    FT_Int       code_last;

    FT_UShort*   char_index;
    FT_String**  char_name;

  } T1_EncodingRec, *T1_Encoding;


  typedef enum  T1_EncodingType_
  {
    T1_ENCODING_TYPE_NONE = 0,
    T1_ENCODING_TYPE_ARRAY,
    T1_ENCODING_TYPE_STANDARD,
    T1_ENCODING_TYPE_ISOLATIN1,
    T1_ENCODING_TYPE_EXPERT

  } T1_EncodingType;


  typedef struct  T1_FontRec_
  {
    PS_FontInfoRec   font_info;         /* font info dictionary */
    PS_PrivateRec    private_dict;      /* private dictionary   */
    FT_String*       font_name;         /* top-level dictionary */

    T1_EncodingType  encoding_type;
    T1_EncodingRec   encoding;

    FT_Byte*         subrs_block;
    FT_Byte*         charstrings_block;
    FT_Byte*         glyph_names_block;

    FT_Int           num_subrs;
    FT_Byte**        subrs;
    FT_PtrDist*      subrs_len;

    FT_Int           num_glyphs;
    FT_String**      glyph_names;       /* array of glyph names       */
    FT_Byte**        charstrings;       /* array of glyph charstrings */
    FT_PtrDist*      charstrings_len;

    FT_Byte          paint_type;
    FT_Byte          font_type;
    FT_Matrix        font_matrix;
    FT_Vector        font_offset;
    FT_BBox          font_bbox;
    FT_Long          font_id;

    FT_Fixed         stroke_width;

  } T1_FontRec, *T1_Font;


  typedef struct  CID_SubrsRec_
  {
    FT_UInt    num_subrs;
    FT_Byte**  code;

  } CID_SubrsRec, *CID_Subrs;


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /***                                                                   ***/
  /***                                                                   ***/
  /***                ORIGINAL T1_FACE CLASS DEFINITION                  ***/
  /***                                                                   ***/
  /***                                                                   ***/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  typedef struct T1_FaceRec_*   T1_Face;
  typedef struct CID_FaceRec_*  CID_Face;


  typedef struct  T1_FaceRec_
  {
    FT_FaceRec     root;
    T1_FontRec     type1;
    const void*    psnames;
    const void*    psaux;
    const void*    afm_data;
    FT_CharMapRec  charmaprecs[2];
    FT_CharMap     charmaps[2];
    PS_Unicodes    unicode_map;

    /* support for Multiple Masters fonts */
    PS_Blend       blend;

    /* since FT 2.1 - interface to PostScript hinter */
    const void*    pshinter;

  } T1_FaceRec;


  typedef struct  CID_FaceRec_
  {
    FT_FaceRec       root;
    void*            psnames;
    void*            psaux;
    CID_FaceInfoRec  cid;
    void*            afm_data;
    FT_Byte*         binary_data; /* used if hex data has been converted */
    FT_Stream        cid_stream;
    CID_Subrs        subrs;

    /* since FT 2.1 - interface to PostScript hinter */
    void*            pshinter;

  } CID_FaceRec;


FT_END_HEADER

#endif /* __T1TYPES_H__ */


/* END */
