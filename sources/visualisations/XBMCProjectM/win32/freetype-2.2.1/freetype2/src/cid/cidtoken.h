/***************************************************************************/
/*                                                                         */
/*  cidtoken.h                                                             */
/*                                                                         */
/*    CID token definitions (specification only).                          */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003 by                                     */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#undef  FT_STRUCTURE
#define FT_STRUCTURE  CID_FaceInfoRec
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_CID_INFO

  T1_FIELD_KEY   ( "CIDFontName", cid_font_name )
  T1_FIELD_NUM   ( "CIDFontVersion", cid_version )
  T1_FIELD_NUM   ( "CIDFontType", cid_font_type )
  T1_FIELD_STRING( "Registry", registry )
  T1_FIELD_STRING( "Ordering", ordering )
  T1_FIELD_NUM   ( "Supplement", supplement )
  T1_FIELD_NUM   ( "UIDBase", uid_base )
  T1_FIELD_NUM   ( "CIDMapOffset", cidmap_offset )
  T1_FIELD_NUM   ( "FDBytes", fd_bytes )
  T1_FIELD_NUM   ( "GDBytes", gd_bytes )
  T1_FIELD_NUM   ( "CIDCount", cid_count )


#undef  FT_STRUCTURE
#define FT_STRUCTURE  PS_FontInfoRec
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_FONT_INFO

  T1_FIELD_STRING( "version", version )
  T1_FIELD_STRING( "Notice", notice )
  T1_FIELD_STRING( "FullName", full_name )
  T1_FIELD_STRING( "FamilyName", family_name )
  T1_FIELD_STRING( "Weight", weight )
  T1_FIELD_NUM   ( "ItalicAngle", italic_angle )
  T1_FIELD_BOOL  ( "isFixedPitch", is_fixed_pitch )
  T1_FIELD_NUM   ( "UnderlinePosition", underline_position )
  T1_FIELD_NUM   ( "UnderlineThickness", underline_thickness )


#undef  FT_STRUCTURE
#define FT_STRUCTURE  CID_FaceDictRec
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_FONT_DICT

  T1_FIELD_NUM  ( "PaintType", paint_type )
  T1_FIELD_NUM  ( "FontType", font_type )
  T1_FIELD_NUM  ( "SubrMapOffset", subrmap_offset )
  T1_FIELD_NUM  ( "SDBytes", sd_bytes )
  T1_FIELD_NUM  ( "SubrCount", num_subrs )
  T1_FIELD_NUM  ( "lenBuildCharArray", len_buildchar )
  T1_FIELD_FIXED( "ForceBoldThreshold", forcebold_threshold )
  T1_FIELD_FIXED( "ExpansionFactor", expansion_factor )
  T1_FIELD_FIXED( "StrokeWidth", stroke_width )


#undef  FT_STRUCTURE
#define FT_STRUCTURE  PS_PrivateRec
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_PRIVATE

  T1_FIELD_NUM       ( "UniqueID", unique_id )
  T1_FIELD_NUM       ( "lenIV", lenIV )
  T1_FIELD_NUM       ( "LanguageGroup", language_group )
  T1_FIELD_NUM       ( "password", password )

  T1_FIELD_FIXED_1000( "BlueScale", blue_scale )
  T1_FIELD_NUM       ( "BlueShift", blue_shift )
  T1_FIELD_NUM       ( "BlueFuzz",  blue_fuzz )

  T1_FIELD_NUM_TABLE ( "BlueValues", blue_values, 14 )
  T1_FIELD_NUM_TABLE ( "OtherBlues", other_blues, 10 )
  T1_FIELD_NUM_TABLE ( "FamilyBlues", family_blues, 14 )
  T1_FIELD_NUM_TABLE ( "FamilyOtherBlues", family_other_blues, 10 )

  T1_FIELD_NUM_TABLE2( "StdHW", standard_width,  1 )
  T1_FIELD_NUM_TABLE2( "StdVW", standard_height, 1 )
  T1_FIELD_NUM_TABLE2( "MinFeature", min_feature, 2 )

  T1_FIELD_NUM_TABLE ( "StemSnapH", snap_widths, 12 )
  T1_FIELD_NUM_TABLE ( "StemSnapV", snap_heights, 12 )

#undef  FT_STRUCTURE
#define FT_STRUCTURE  FT_BBox
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_BBOX

  T1_FIELD_BBOX( "FontBBox", xMin )


/* END */
