/***************************************************************************/
/*                                                                         */
/*  freetype.h                                                             */
/*                                                                         */
/*    FreeType high-level API and common types (specification only).       */
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


#ifndef FT_FREETYPE_H
#error "`ft2build.h' hasn't been included yet!"
#error "Please always use macros to include FreeType header files."
#error "Example:"
#error "  #include <ft2build.h>"
#error "  #include FT_FREETYPE_H"
#endif


#ifndef __FREETYPE_H__
#define __FREETYPE_H__


  /*************************************************************************/
  /*                                                                       */
  /* The `raster' component duplicates some of the declarations in         */
  /* freetype.h for stand-alone use if _FREETYPE_ isn't defined.           */
  /*                                                                       */


  /*************************************************************************/
  /*                                                                       */
  /* The FREETYPE_MAJOR and FREETYPE_MINOR macros are used to version the  */
  /* new FreeType design, which is able to host several kinds of font      */
  /* drivers.  It starts at 2.0.                                           */
  /*                                                                       */
#define FREETYPE_MAJOR 2
#define FREETYPE_MINOR 1
#define FREETYPE_PATCH 9


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_ERRORS_H
#include FT_TYPES_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*************************************************************************/
  /*                                                                       */
  /*                        B A S I C   T Y P E S                          */
  /*                                                                       */
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    base_interface                                                     */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Base Interface                                                     */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    The FreeType 2 base font interface.                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section describes the public high-level API of FreeType 2.    */
  /*                                                                       */
  /* <Order>                                                               */
  /*    FT_Library                                                         */
  /*    FT_Face                                                            */
  /*    FT_Size                                                            */
  /*    FT_GlyphSlot                                                       */
  /*    FT_CharMap                                                         */
  /*    FT_Encoding                                                        */
  /*                                                                       */
  /*    FT_FaceRec                                                         */
  /*                                                                       */
  /*    FT_FACE_FLAG_SCALABLE                                              */
  /*    FT_FACE_FLAG_FIXED_SIZES                                           */
  /*    FT_FACE_FLAG_FIXED_WIDTH                                           */
  /*    FT_FACE_FLAG_HORIZONTAL                                            */
  /*    FT_FACE_FLAG_VERTICAL                                              */
  /*    FT_FACE_FLAG_SFNT                                                  */
  /*    FT_FACE_FLAG_KERNING                                               */
  /*    FT_FACE_FLAG_MULTIPLE_MASTERS                                      */
  /*    FT_FACE_FLAG_GLYPH_NAMES                                           */
  /*    FT_FACE_FLAG_EXTERNAL_STREAM                                       */
  /*    FT_FACE_FLAG_FAST_GLYPHS                                           */
  /*                                                                       */
  /*    FT_STYLE_FLAG_BOLD                                                 */
  /*    FT_STYLE_FLAG_ITALIC                                               */
  /*                                                                       */
  /*    FT_SizeRec                                                         */
  /*    FT_Size_Metrics                                                    */
  /*                                                                       */
  /*    FT_GlyphSlotRec                                                    */
  /*    FT_Glyph_Metrics                                                   */
  /*    FT_SubGlyph                                                        */
  /*                                                                       */
  /*    FT_Bitmap_Size                                                     */
  /*                                                                       */
  /*    FT_Init_FreeType                                                   */
  /*    FT_Done_FreeType                                                   */
  /*    FT_Library_Version                                                 */
  /*                                                                       */
  /*    FT_New_Face                                                        */
  /*    FT_Done_Face                                                       */
  /*    FT_New_Memory_Face                                                 */
  /*    FT_Open_Face                                                       */
  /*    FT_Open_Args                                                       */
  /*    FT_Parameter                                                       */
  /*    FT_Attach_File                                                     */
  /*    FT_Attach_Stream                                                   */
  /*                                                                       */
  /*    FT_Set_Char_Size                                                   */
  /*    FT_Set_Pixel_Sizes                                                 */
  /*    FT_Set_Transform                                                   */
  /*    FT_Load_Glyph                                                      */
  /*    FT_Get_Char_Index                                                  */
  /*    FT_Get_Name_Index                                                  */
  /*    FT_Load_Char                                                       */
  /*                                                                       */
  /*    FT_OPEN_MEMORY                                                     */
  /*    FT_OPEN_STREAM                                                     */
  /*    FT_OPEN_PATHNAME                                                   */
  /*    FT_OPEN_DRIVER                                                     */
  /*    FT_OPEN_PARAMS                                                     */
  /*                                                                       */
  /*    FT_LOAD_DEFAULT                                                    */
  /*    FT_LOAD_RENDER                                                     */
  /*    FT_LOAD_MONOCHROME                                                 */
  /*    FT_LOAD_LINEAR_DESIGN                                              */
  /*    FT_LOAD_NO_SCALE                                                   */
  /*    FT_LOAD_NO_HINTING                                                 */
  /*    FT_LOAD_NO_BITMAP                                                  */
  /*    FT_LOAD_CROP_BITMAP                                                */
  /*                                                                       */
  /*    FT_LOAD_VERTICAL_LAYOUT                                            */
  /*    FT_LOAD_IGNORE_TRANSFORM                                           */
  /*    FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH                                */
  /*    FT_LOAD_FORCE_AUTOHINT                                             */
  /*    FT_LOAD_NO_RECURSE                                                 */
  /*    FT_LOAD_PEDANTIC                                                   */
  /*                                                                       */
  /*    FT_LOAD_TARGET_NORMAL                                              */
  /*    FT_LOAD_TARGET_LIGHT                                               */
  /*    FT_LOAD_TARGET_MONO                                                */
  /*    FT_LOAD_TARGET_LCD                                                 */
  /*    FT_LOAD_TARGET_LCD_V                                               */
  /*                                                                       */
  /*    FT_Render_Glyph                                                    */
  /*    FT_Render_Mode                                                     */
  /*    FT_Get_Kerning                                                     */
  /*    FT_Kerning_Mode                                                    */
  /*    FT_Get_Glyph_Name                                                  */
  /*    FT_Get_Postscript_Name                                             */
  /*                                                                       */
  /*    FT_CharMapRec                                                      */
  /*    FT_Select_Charmap                                                  */
  /*    FT_Set_Charmap                                                     */
  /*    FT_Get_Charmap_Index                                               */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Glyph_Metrics                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model the metrics of a single glyph.  The      */
  /*    values are expressed in 26.6 fractional pixel format; if the flag  */
  /*    FT_LOAD_NO_SCALE is used, values are returned in font units        */
  /*    instead.                                                           */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    width ::                                                           */
  /*      The glyph's width.                                               */
  /*                                                                       */
  /*    height ::                                                          */
  /*      The glyph's height.                                              */
  /*                                                                       */
  /*    horiBearingX ::                                                    */
  /*      Left side bearing for horizontal layout.                         */
  /*                                                                       */
  /*    horiBearingY ::                                                    */
  /*      Top side bearing for horizontal layout.                          */
  /*                                                                       */
  /*    horiAdvance ::                                                     */
  /*      Advance width for horizontal layout.                             */
  /*                                                                       */
  /*    vertBearingX ::                                                    */
  /*      Left side bearing for vertical layout.                           */
  /*                                                                       */
  /*    vertBearingY ::                                                    */
  /*      Top side bearing for vertical layout.                            */
  /*                                                                       */
  /*    vertAdvance ::                                                     */
  /*      Advance height for vertical layout.                              */
  /*                                                                       */
  typedef struct  FT_Glyph_Metrics_
  {
    FT_Pos  width;
    FT_Pos  height;

    FT_Pos  horiBearingX;
    FT_Pos  horiBearingY;
    FT_Pos  horiAdvance;

    FT_Pos  vertBearingX;
    FT_Pos  vertBearingY;
    FT_Pos  vertAdvance;

  } FT_Glyph_Metrics;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Bitmap_Size                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This structure models the size of a bitmap strike (i.e., a bitmap  */
  /*    instance of the font for a given resolution) in a fixed-size font  */
  /*    face.  It is used for the `available_sizes' field of the           */
  /*    @FT_FaceRec structure.                                             */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    height :: The (vertical) baseline-to-baseline distance in pixels.  */
  /*              It makes most sense to define the height of a bitmap     */
  /*              font in this way.                                        */
  /*                                                                       */
  /*    width  :: The average width of the font (in pixels).  Since the    */
  /*              algorithms to compute this value are different for the   */
  /*              various bitmap formats, it can only give an additional   */
  /*              hint if the `height' value isn't sufficient to select    */
  /*              the proper font.  For monospaced fonts the average width */
  /*              is the same as the maximum width.                        */
  /*                                                                       */
  /*    size   :: The point size in 26.6 fractional format this font shall */
  /*              represent (for a given vertical resolution).             */
  /*                                                                       */
  /*    x_ppem :: The horizontal ppem value (in 26.6 fractional format).   */
  /*                                                                       */
  /*    y_ppem :: The vertical ppem value (in 26.6 fractional format).     */
  /*              Usually, this is the `nominal' pixel height of the font. */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The values in this structure are taken from the bitmap font.  If   */
  /*    the font doesn't provide a parameter it is set to zero to indicate */
  /*    that the information is not available.                             */
  /*                                                                       */
  /*    The following formula converts from dpi to ppem:                   */
  /*                                                                       */
  /*      ppem = size * dpi / 72                                           */
  /*                                                                       */
  /*    where `size' is in points.                                         */
  /*                                                                       */
  /*    Windows FNT:                                                       */
  /*      The `size' parameter is not reliable: There exist fonts (e.g.,   */
  /*      app850.fon) which have a wrong size for some subfonts; x_ppem    */
  /*      and y_ppem are thus set equal to pixel width and height given in */
  /*      in the Windows FNT header.                                       */
  /*                                                                       */
  /*    TrueType embedded bitmaps:                                         */
  /*      `size', `width', and `height' values are not contained in the    */
  /*      bitmap strike itself.  They are computed from the global font    */
  /*      parameters.                                                      */
  /*                                                                       */
  typedef struct  FT_Bitmap_Size_
  {
    FT_Short  height;
    FT_Short  width;

    FT_Pos    size;

    FT_Pos    x_ppem;
    FT_Pos    y_ppem;

  } FT_Bitmap_Size;


  /*************************************************************************/
  /*************************************************************************/
  /*                                                                       */
  /*                     O B J E C T   C L A S S E S                       */
  /*                                                                       */
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Library                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a FreeType library instance.  Each `library' is        */
  /*    completely independent from the others; it is the `root' of a set  */
  /*    of objects like fonts, faces, sizes, etc.                          */
  /*                                                                       */
  /*    It also embeds a memory manager (see @FT_Memory), as well as a     */
  /*    scan-line converter object (see @FT_Raster).                       */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Library objects are normally created by @FT_Init_FreeType, and     */
  /*    destroyed with @FT_Done_FreeType.                                  */
  /*                                                                       */
  typedef struct FT_LibraryRec_  *FT_Library;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Module                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given FreeType module object.  Each module can be a  */
  /*    font driver, a renderer, or anything else that provides services   */
  /*    to the formers.                                                    */
  /*                                                                       */
  typedef struct FT_ModuleRec_*  FT_Module;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Driver                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given FreeType font driver object.  Each font driver */
  /*    is a special module capable of creating faces from font files.     */
  /*                                                                       */
  typedef struct FT_DriverRec_*  FT_Driver;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Renderer                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given FreeType renderer.  A renderer is a special    */
  /*    module in charge of converting a glyph image to a bitmap, when     */
  /*    necessary.  Each renderer supports a given glyph image format, and */
  /*    one or more target surface depths.                                 */
  /*                                                                       */
  typedef struct FT_RendererRec_*  FT_Renderer;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Face                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given typographic face object.  A face object models */
  /*    a given typeface, in a given style.                                */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Each face object also owns a single @FT_GlyphSlot object, as well  */
  /*    as one or more @FT_Size objects.                                   */
  /*                                                                       */
  /*    Use @FT_New_Face or @FT_Open_Face to create a new face object from */
  /*    a given filepathname or a custom input stream.                     */
  /*                                                                       */
  /*    Use @FT_Done_Face to destroy it (along with its slot and sizes).   */
  /*                                                                       */
  /* <Also>                                                                */
  /*    The @FT_FaceRec details the publicly accessible fields of a given  */
  /*    face object.                                                       */
  /*                                                                       */
  typedef struct FT_FaceRec_*  FT_Face;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Size                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given size object.  Such an object models the data   */
  /*    that depends on the current _resolution_ and _character size_ in a */
  /*    given @FT_Face.                                                    */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Each face object owns one or more sizes.  There is however a       */
  /*    single _active_ size for the face at any time that will be used by */
  /*    functions like @FT_Load_Glyph, @FT_Get_Kerning, etc.               */
  /*                                                                       */
  /*    You can use the @FT_Activate_Size API to change the current        */
  /*    active size of any given face.                                     */
  /*                                                                       */
  /* <Also>                                                                */
  /*    The @FT_SizeRec structure details the publicly accessible fields   */
  /*    of a given face object.                                            */
  /*                                                                       */
  typedef struct FT_SizeRec_*  FT_Size;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_GlyphSlot                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given `glyph slot'.  A slot is a container where it  */
  /*    is possible to load any one of the glyphs contained in its parent  */
  /*    face.                                                              */
  /*                                                                       */
  /*    In other words, each time you call @FT_Load_Glyph or               */
  /*    @FT_Load_Char, the slot's content is erased by the new glyph data, */
  /*    i.e. the glyph's metrics, its image (bitmap or outline), and       */
  /*    other control information.                                         */
  /*                                                                       */
  /* <Also>                                                                */
  /*    @FT_GlyphSlotRec details the publicly accessible glyph fields.     */
  /*                                                                       */
  typedef struct FT_GlyphSlotRec_*  FT_GlyphSlot;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_CharMap                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given character map.  A charmap is used to translate */
  /*    character codes in a given encoding into glyph indexes for its     */
  /*    parent's face.  Some font formats may provide several charmaps per */
  /*    font.                                                              */
  /*                                                                       */
  /*    Each face object owns zero or more charmaps, but only one of them  */
  /*    can be "active" and used by @FT_Get_Char_Index or @FT_Load_Char.   */
  /*                                                                       */
  /*    The list of available charmaps in a face is available through the  */
  /*    "face->num_charmaps" and "face->charmaps" fields of @FT_FaceRec.   */
  /*                                                                       */
  /*    The currently active charmap is available as "face->charmap".      */
  /*    You should call @FT_Set_Charmap to change it.                      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    When a new face is created (either through @FT_New_Face or         */
  /*    @FT_Open_Face), the library looks for a Unicode charmap within     */
  /*    the list and automatically activates it.                           */
  /*                                                                       */
  /* <Also>                                                                */
  /*    The @FT_CharMapRec details the publicly accessible fields of a     */
  /*    given character map.                                               */
  /*                                                                       */
  typedef struct FT_CharMapRec_*  FT_CharMap;


  /*************************************************************************/
  /*                                                                       */
  /* <Macro>                                                               */
  /*    FT_ENC_TAG                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This macro converts four letter tags into an unsigned long.  It is */
  /*    used to define "encoding" identifiers (see @FT_Encoding).          */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Since many 16bit compilers don't like 32bit enumerations, you      */
  /*    should redefine this macro in case of problems to something like   */
  /*    this:                                                              */
  /*                                                                       */
  /*      #define FT_ENC_TAG( value, a, b, c, d )  value                   */
  /*                                                                       */
  /*    to get a simple enumeration without assigning special numbers.     */
  /*                                                                       */

#ifndef FT_ENC_TAG
#define FT_ENC_TAG( value, a, b, c, d )         \
          value = ( ( (FT_UInt32)(a) << 24 ) |  \
                    ( (FT_UInt32)(b) << 16 ) |  \
                    ( (FT_UInt32)(c) <<  8 ) |  \
                      (FT_UInt32)(d)         )

#endif /* FT_ENC_TAG */


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    FT_Encoding                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An enumeration used to specify encodings supported by charmaps.    */
  /*    Used in the @FT_Select_Charmap API function.                       */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Because of 32-bit charcodes defined in Unicode (i.e., surrogates), */
  /*    all character codes must be expressed as FT_Longs.                 */
  /*                                                                       */
  /*    The values of this type correspond to specific character           */
  /*    repertories (i.e. charsets), and not to text encoding methods      */
  /*    (like UTF-8, UTF-16, GB2312_EUC, etc.).                            */
  /*                                                                       */
  /*    Other encodings might be defined in the future.                    */
  /*                                                                       */
  /* <Values>                                                              */
  /*   FT_ENCODING_NONE ::                                                 */
  /*     The encoding value 0 is reserved.                                 */
  /*                                                                       */
  /*   FT_ENCODING_UNICODE ::                                              */
  /*     Corresponds to the Unicode character set.  This value covers      */
  /*     all versions of the Unicode repertoire, including ASCII and       */
  /*     Latin-1.  Most fonts include a Unicode charmap, but not all       */
  /*     of them.                                                          */
  /*                                                                       */
  /*   FT_ENCODING_MS_SYMBOL ::                                            */
  /*     Corresponds to the Microsoft Symbol encoding, used to encode      */
  /*     mathematical symbols in the 32..255 character code range.  For    */
  /*     more information, see `http://www.ceviz.net/symbol.htm'.          */
  /*                                                                       */
  /*   FT_ENCODING_SJIS ::                                                 */
  /*     Corresponds to Japanese SJIS encoding.  More info at              */
  /*     at `http://langsupport.japanreference.com/encoding.shtml'.        */
  /*     See note on multi-byte encodings below.                           */
  /*                                                                       */
  /*   FT_ENCODING_GB2312 ::                                               */
  /*     Corresponds to an encoding system for Simplified Chinese as used  */
  /*     used in mainland China.                                           */
  /*                                                                       */
  /*   FT_ENCODING_BIG5 ::                                                 */
  /*     Corresponds to an encoding system for Traditional Chinese as used */
  /*     in Taiwan and Hong Kong.                                          */
  /*                                                                       */
  /*   FT_ENCODING_WANSUNG ::                                              */
  /*     Corresponds to the Korean encoding system known as Wansung.       */
  /*     For more information see                                          */
  /*     `http://www.microsoft.com/typography/unicode/949.txt'.            */
  /*                                                                       */
  /*   FT_ENCODING_JOHAB ::                                                */
  /*     The Korean standard character set (KS C-5601-1992), which         */
  /*     corresponds to MS Windows code page 1361.  This character set     */
  /*     includes all possible Hangeul character combinations.             */
  /*                                                                       */
  /*   FT_ENCODING_ADOBE_LATIN_1 ::                                        */
  /*     Corresponds to a Latin-1 encoding as defined in a Type 1          */
  /*     Postscript font.  It is limited to 256 character codes.           */
  /*                                                                       */
  /*   FT_ENCODING_ADOBE_STANDARD ::                                       */
  /*     Corresponds to the Adobe Standard encoding, as found in Type 1,   */
  /*     CFF, and OpenType/CFF fonts.  It is limited to 256 character      */
  /*     codes.                                                            */
  /*                                                                       */
  /*   FT_ENCODING_ADOBE_EXPERT ::                                         */
  /*     Corresponds to the Adobe Expert encoding, as found in Type 1,     */
  /*     CFF, and OpenType/CFF fonts.  It is limited to 256 character      */
  /*     codes.                                                            */
  /*                                                                       */
  /*   FT_ENCODING_ADOBE_CUSTOM ::                                         */
  /*     Corresponds to a custom encoding, as found in Type 1, CFF, and    */
  /*     OpenType/CFF fonts.  It is limited to 256 character codes.        */
  /*                                                                       */
  /*   FT_ENCODING_APPLE_ROMAN ::                                          */
  /*     Corresponds to the 8-bit Apple roman encoding.  Many TrueType and */
  /*     OpenType fonts contain a charmap for this encoding, since older   */
  /*     versions of Mac OS are able to use it.                            */
  /*                                                                       */
  /*   FT_ENCODING_OLD_LATIN_2 ::                                          */
  /*     This value is deprecated and was never used nor reported by       */
  /*     FreeType.  Don't use or test for it.                              */
  /*                                                                       */
  /*   FT_ENCODING_MS_SJIS ::                                              */
  /*     Same as FT_ENCODING_SJIS.  Deprecated.                            */
  /*                                                                       */
  /*   FT_ENCODING_MS_GB2312 ::                                            */
  /*     Same as FT_ENCODING_GB2312.  Deprecated.                          */
  /*                                                                       */
  /*   FT_ENCODING_MS_BIG5 ::                                              */
  /*     Same as FT_ENCODING_BIG5.  Deprecated.                            */
  /*                                                                       */
  /*   FT_ENCODING_MS_WANSUNG ::                                           */
  /*     Same as FT_ENCODING_WANSUNG.  Deprecated.                         */
  /*                                                                       */
  /*   FT_ENCODING_MS_JOHAB ::                                             */
  /*     Same as FT_ENCODING_JOHAB.  Deprecated.                           */
  /*                                                                       */
  /* <Note>                                                                */
  /*   By default, FreeType automatically synthetizes a Unicode charmap    */
  /*   for Postscript fonts, using their glyph names dictionaries.         */
  /*   However, it will also report the encodings defined explicitly in    */
  /*   the font file, for the cases when they are needed, with the Adobe   */
  /*   values as well.                                                     */
  /*                                                                       */
  /*   FT_ENCODING_NONE is set by the BDF and PCF drivers if the charmap   */
  /*   is neither Unicode nor ISO-8859-1 (otherwise it is set to           */
  /*   FT_ENCODING_UNICODE).  Use `FT_Get_BDF_Charset_ID' to find out      */
  /*   which encoding is really present.  If, for example, the             */
  /*   `cs_registry' field is `KOI8' and the `cs_encoding' field is `R',   */
  /*   the font is encoded in KOI8-R.                                      */
  /*                                                                       */
  /*   FT_ENCODING_NONE is always set (with a single exception) by the     */
  /*   winfonts driver.  Use `FT_Get_WinFNT_Header' and examine the        */
  /*   `charset' field of the `FT_WinFNT_HeaderRec' structure to find out  */
  /*   which encoding is really present.  For example, FT_WinFNT_ID_CP1251 */
  /*   (204) means Windows code page 1251 (for Russian).                   */
  /*                                                                       */
  /*   FT_ENCODING_NONE is set if `platform_id' is `TT_PLATFORM_MACINTOSH' */
  /*   and `encoding_id' is not `TT_MAC_ID_ROMAN' (otherwise it is set to  */
  /*   FT_ENCODING_APPLE_ROMAN).                                           */
  /*                                                                       */
  /*   If `platform_id' is `TT_PLATFORM_MACINTOSH', use the function       */
  /*   `FT_Get_CMap_Language_ID' to query the Mac language ID which may be */
  /*   needed to be able to distinguish Apple encoding variants.  See      */
  /*                                                                       */
  /*     http://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/README.TXT   */
  /*                                                                       */
  /*   to get an idea how to do that.  Basically, if the language ID is 0, */
  /*   dont use it, otherwise subtract 1 from the language ID.  Then       */
  /*   examine `encoding_id'.  If, for example, `encoding_id' is           */
  /*   `TT_MAC_ID_ROMAN' and the language ID (minus 1) is                  */
  /*   `TT_MAC_LANGID_GREEK', it is the Greek encoding, not Roman.         */
  /*   `TT_MAC_ID_ARABIC' with `TT_MAC_LANGID_FARSI' means the Farsi       */
  /*   variant the Arabic encoding.                                        */
  /*                                                                       */
  typedef enum  FT_Encoding_
  {
    FT_ENC_TAG( FT_ENCODING_NONE, 0, 0, 0, 0 ),

    FT_ENC_TAG( FT_ENCODING_MS_SYMBOL,  's', 'y', 'm', 'b' ),
    FT_ENC_TAG( FT_ENCODING_UNICODE,    'u', 'n', 'i', 'c' ),

    FT_ENC_TAG( FT_ENCODING_SJIS,    's', 'j', 'i', 's' ),
    FT_ENC_TAG( FT_ENCODING_GB2312,  'g', 'b', ' ', ' ' ),
    FT_ENC_TAG( FT_ENCODING_BIG5,    'b', 'i', 'g', '5' ),
    FT_ENC_TAG( FT_ENCODING_WANSUNG, 'w', 'a', 'n', 's' ),
    FT_ENC_TAG( FT_ENCODING_JOHAB,   'j', 'o', 'h', 'a' ),

    /* for backwards compatibility */
    FT_ENCODING_MS_SJIS    = FT_ENCODING_SJIS,
    FT_ENCODING_MS_GB2312  = FT_ENCODING_GB2312,
    FT_ENCODING_MS_BIG5    = FT_ENCODING_BIG5,
    FT_ENCODING_MS_WANSUNG = FT_ENCODING_WANSUNG,
    FT_ENCODING_MS_JOHAB   = FT_ENCODING_JOHAB,

    FT_ENC_TAG( FT_ENCODING_ADOBE_STANDARD, 'A', 'D', 'O', 'B' ),
    FT_ENC_TAG( FT_ENCODING_ADOBE_EXPERT,   'A', 'D', 'B', 'E' ),
    FT_ENC_TAG( FT_ENCODING_ADOBE_CUSTOM,   'A', 'D', 'B', 'C' ),
    FT_ENC_TAG( FT_ENCODING_ADOBE_LATIN_1,  'l', 'a', 't', '1' ),

    FT_ENC_TAG( FT_ENCODING_OLD_LATIN_2, 'l', 'a', 't', '2' ),

    FT_ENC_TAG( FT_ENCODING_APPLE_ROMAN, 'a', 'r', 'm', 'n' )

  } FT_Encoding;


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    ft_encoding_xxx                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    These constants are deprecated; use the corresponding @FT_Encoding */
  /*    values instead.                                                    */
  /*                                                                       */
  /* <Values>                                                              */
  /*   ft_encoding_none    :: see @FT_ENCODING_NONE                        */
  /*   ft_encoding_unicode :: see @FT_ENCODING_UNICODE                     */
  /*   ft_encoding_latin_2 :: see @FT_ENCODING_OLD_LATIN_2                 */
  /*   ft_encoding_symbol  :: see @FT_ENCODING_MS_SYMBOL                   */
  /*   ft_encoding_sjis    :: see @FT_ENCODING_SJIS                        */
  /*   ft_encoding_gb2312  :: see @FT_ENCODING_GB2312                      */
  /*   ft_encoding_big5    :: see @FT_ENCODING_BIG5                        */
  /*   ft_encoding_wansung :: see @FT_ENCODING_WANSUNG                     */
  /*   ft_encoding_johab   :: see @FT_ENCODING_JOHAB                       */
  /*                                                                       */
  /*   ft_encoding_adobe_standard :: see @FT_ENCODING_ADOBE_STANDARD       */
  /*   ft_encoding_adobe_expert   :: see @FT_ENCODING_ADOBE_EXPERT         */
  /*   ft_encoding_adobe_custom   :: see @FT_ENCODING_ADOBE_CUSTOM         */
  /*   ft_encoding_latin_1        :: see @FT_ENCODING_ADOBE_LATIN_1        */
  /*                                                                       */
  /*   ft_encoding_apple_roman    :: see @FT_ENCODING_APPLE_ROMAN          */
  /*                                                                       */
#define ft_encoding_none            FT_ENCODING_NONE
#define ft_encoding_unicode         FT_ENCODING_UNICODE
#define ft_encoding_symbol          FT_ENCODING_MS_SYMBOL
#define ft_encoding_latin_1         FT_ENCODING_ADOBE_LATIN_1
#define ft_encoding_latin_2         FT_ENCODING_OLD_LATIN_2
#define ft_encoding_sjis            FT_ENCODING_SJIS
#define ft_encoding_gb2312          FT_ENCODING_GB2312
#define ft_encoding_big5            FT_ENCODING_BIG5
#define ft_encoding_wansung         FT_ENCODING_WANSUNG
#define ft_encoding_johab           FT_ENCODING_JOHAB

#define ft_encoding_adobe_standard  FT_ENCODING_ADOBE_STANDARD
#define ft_encoding_adobe_expert    FT_ENCODING_ADOBE_EXPERT
#define ft_encoding_adobe_custom    FT_ENCODING_ADOBE_CUSTOM
#define ft_encoding_apple_roman     FT_ENCODING_APPLE_ROMAN


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_CharMapRec                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The base charmap structure.                                        */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    face        :: A handle to the parent face object.                 */
  /*                                                                       */
  /*    encoding    :: An @FT_Encoding tag identifying the charmap.  Use   */
  /*                   this with @FT_Select_Charmap.                       */
  /*                                                                       */
  /*    platform_id :: An ID number describing the platform for the        */
  /*                   following encoding ID.  This comes directly from    */
  /*                   the TrueType specification and should be emulated   */
  /*                   for other formats.                                  */
  /*                                                                       */
  /*    encoding_id :: A platform specific encoding number.  This also     */
  /*                   comes from the TrueType specification and should be */
  /*                   emulated similarly.                                 */
  /*                                                                       */
  typedef struct  FT_CharMapRec_
  {
    FT_Face      face;
    FT_Encoding  encoding;
    FT_UShort    platform_id;
    FT_UShort    encoding_id;

  } FT_CharMapRec;


  /*************************************************************************/
  /*************************************************************************/
  /*                                                                       */
  /*                 B A S E   O B J E C T   C L A S S E S                 */
  /*                                                                       */
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Face_Internal                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An opaque handle to an FT_Face_InternalRec structure, used to      */
  /*    model private data of a given @FT_Face object.                     */
  /*                                                                       */
  /*    This structure might change between releases of FreeType 2 and is  */
  /*    not generally available to client applications.                    */
  /*                                                                       */
  typedef struct FT_Face_InternalRec_*  FT_Face_Internal;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_FaceRec                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    FreeType root face class structure.  A face object models the      */
  /*    resolution and point-size independent data found in a font file.   */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    num_faces           :: In the case where the face is located in a  */
  /*                           collection (i.e., a file which embeds       */
  /*                           several faces), this is the total number of */
  /*                           faces found in the resource.  1 by default. */
  /*                           Accessing non-existent face indices causes  */
  /*                           an error.                                   */
  /*                                                                       */
  /*    face_index          :: The index of the face in its font file.     */
  /*                           Usually, this is 0 for all normal font      */
  /*                           formats.  It can be > 0 in the case of      */
  /*                           collections (which embed several fonts in a */
  /*                           single resource/file).                      */
  /*                                                                       */
  /*    face_flags          :: A set of bit flags that give important      */
  /*                           information about the face; see the         */
  /*                           @FT_FACE_FLAG_XXX constants for details.    */
  /*                                                                       */
  /*    style_flags         :: A set of bit flags indicating the style of  */
  /*                           the face (i.e., italic, bold, underline,    */
  /*                           etc).  See the @FT_STYLE_FLAG_XXX           */
  /*                           constants.                                  */
  /*                                                                       */
  /*    num_glyphs          :: The total number of glyphs in the face.     */
  /*                                                                       */
  /*    family_name         :: The face's family name.  This is an ASCII   */
  /*                           string, usually in English, which describes */
  /*                           the typeface's family (like `Times New      */
  /*                           Roman', `Bodoni', `Garamond', etc).  This   */
  /*                           is a least common denominator used to list  */
  /*                           fonts.  Some formats (TrueType & OpenType)  */
  /*                           provide localized and Unicode versions of   */
  /*                           this string.  Applications should use the   */
  /*                           format specific interface to access them.   */
  /*                                                                       */
  /*    style_name          :: The face's style name.  This is an ASCII    */
  /*                           string, usually in English, which describes */
  /*                           the typeface's style (like `Italic',        */
  /*                           `Bold', `Condensed', etc).  Not all font    */
  /*                           formats provide a style name, so this field */
  /*                           is optional, and can be set to NULL.  As    */
  /*                           for `family_name', some formats provide     */
  /*                           localized/Unicode versions of this string.  */
  /*                           Applications should use the format specific */
  /*                           interface to access them.                   */
  /*                                                                       */
  /*    num_fixed_sizes     :: The number of fixed sizes available in this */
  /*                           face.  This should be set to 0 for scalable */
  /*                           fonts, unless its face includes a set of    */
  /*                           glyphs (called a `strike') for the          */
  /*                           specified sizes.                            */
  /*                                                                       */
  /*    available_sizes     :: An array of sizes specifying the available  */
  /*                           bitmap/graymap sizes that are contained in  */
  /*                           in the font face.  Should be set to NULL if */
  /*                           the field `num_fixed_sizes' is set to 0.    */
  /*                                                                       */
  /*    num_charmaps        :: The total number of character maps in the   */
  /*                           face.                                       */
  /*                                                                       */
  /*    charmaps            :: A table of pointers to the face's charmaps. */
  /*                           Used to scan the list of available charmaps */
  /*                           -- this table might change after a call to  */
  /*                           @FT_Attach_File or @FT_Attach_Stream (e.g.  */
  /*                           if used to hook an additional encoding or   */
  /*                           CMap to the face object).                   */
  /*                                                                       */
  /*    generic             :: A field reserved for client uses.  See the  */
  /*                           @FT_Generic type description.               */
  /*                                                                       */
  /*    bbox                :: The font bounding box.  Coordinates are     */
  /*                           expressed in font units (see units_per_EM). */
  /*                           The box is large enough to contain any      */
  /*                           glyph from the font.  Thus, bbox.yMax can   */
  /*                           be seen as the `maximal ascender',          */
  /*                           bbox.yMin as the `minimal descender', and   */
  /*                           the maximal glyph width is given by         */
  /*                           `bbox.xMax-bbox.xMin' (not to be confused   */
  /*                           with the maximal _advance_width_).  Only    */
  /*                           relevant for scalable formats.              */
  /*                                                                       */
  /*    units_per_EM        :: The number of font units per EM square for  */
  /*                           this face.  This is typically 2048 for      */
  /*                           TrueType fonts, 1000 for Type1 fonts, and   */
  /*                           should be set to the (unrealistic) value 1  */
  /*                           for fixed-sizes fonts.  Only relevant for   */
  /*                           scalable formats.                           */
  /*                                                                       */
  /*    ascender            :: The face's ascender is the vertical         */
  /*                           distance from the baseline to the topmost   */
  /*                           point of any glyph in the face.  This       */
  /*                           field's value is positive, expressed in     */
  /*                           font units.  Some font designs use a value  */
  /*                           different from `bbox.yMax'.  Only relevant  */
  /*                           for scalable formats.                       */
  /*                                                                       */
  /*    descender           :: The face's descender is the vertical        */
  /*                           distance from the baseline to the           */
  /*                           bottommost point of any glyph in the face.  */
  /*                           This field's value is *negative* for values */
  /*                           below the baseline.  It is expressed in     */
  /*                           font units.  Some font designs use a value  */
  /*                           different from `bbox.yMin'.  Only relevant  */
  /*                           for scalable formats.                       */
  /*                                                                       */
  /*    height              :: The face's height is the vertical distance  */
  /*                           from one baseline to the next when writing  */
  /*                           several lines of text.  Its value is always */
  /*                           positive, expressed in font units.  The     */
  /*                           value can be computed as                    */
  /*                           `ascender+descender+line_gap' where the     */
  /*                           value of `line_gap' is also called          */
  /*                           `external leading'.  Only relevant for      */
  /*                           scalable formats.                           */
  /*                                                                       */
  /*    max_advance_width   :: The maximal advance width, in font units,   */
  /*                           for all glyphs in this face.  This can be   */
  /*                           used to make word wrapping computations     */
  /*                           faster.  Only relevant for scalable         */
  /*                           formats.                                    */
  /*                                                                       */
  /*    max_advance_height  :: The maximal advance height, in font units,  */
  /*                           for all glyphs in this face.  This is only  */
  /*                           relevant for vertical layouts, and should   */
  /*                           be set to the `height' for fonts that do    */
  /*                           not provide vertical metrics.  Only         */
  /*                           relevant for scalable formats.              */
  /*                                                                       */
  /*    underline_position  :: The position, in font units, of the         */
  /*                           underline line for this face.  It's the     */
  /*                           center of the underlining stem.  Only       */
  /*                           relevant for scalable formats.              */
  /*                                                                       */
  /*    underline_thickness :: The thickness, in font units, of the        */
  /*                           underline for this face.  Only relevant for */
  /*                           scalable formats.                           */
  /*                                                                       */
  /*    glyph               :: The face's associated glyph slot(s).  This  */
  /*                           object is created automatically with a new  */
  /*                           face object.  However, certain kinds of     */
  /*                           applications (mainly tools like converters) */
  /*                           can need more than one slot to ease their   */
  /*                           task.                                       */
  /*                                                                       */
  /*    size                :: The current active size for this face.      */
  /*                                                                       */
  /*    charmap             :: The current active charmap for this face.   */
  /*                                                                       */
  typedef struct  FT_FaceRec_
  {
    FT_Long           num_faces;
    FT_Long           face_index;

    FT_Long           face_flags;
    FT_Long           style_flags;

    FT_Long           num_glyphs;

    FT_String*        family_name;
    FT_String*        style_name;

    FT_Int            num_fixed_sizes;
    FT_Bitmap_Size*   available_sizes;

    FT_Int            num_charmaps;
    FT_CharMap*       charmaps;

    FT_Generic        generic;

    /*# the following are only relevant to scalable outlines */
    FT_BBox           bbox;

    FT_UShort         units_per_EM;
    FT_Short          ascender;
    FT_Short          descender;
    FT_Short          height;

    FT_Short          max_advance_width;
    FT_Short          max_advance_height;

    FT_Short          underline_position;
    FT_Short          underline_thickness;

    FT_GlyphSlot      glyph;
    FT_Size           size;
    FT_CharMap        charmap;

    /*@private begin */

    FT_Driver         driver;
    FT_Memory         memory;
    FT_Stream         stream;

    FT_ListRec        sizes_list;

    FT_Generic        autohint;
    void*             extensions;

    FT_Face_Internal  internal;

    /*@private end */

  } FT_FaceRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    FT_FACE_FLAG_XXX                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A list of bit flags used in the `face_flags' field of the          */
  /*    @FT_FaceRec structure.  They inform client applications of         */
  /*    properties of the corresponding face.                              */
  /*                                                                       */
  /* <Values>                                                              */
  /*    FT_FACE_FLAG_SCALABLE ::                                           */
  /*      Indicates that the face provides vectorial outlines.  This       */
  /*      doesn't prevent embedded bitmaps, i.e., a face can have both     */
  /*      this bit and @FT_FACE_FLAG_FIXED_SIZES set.                      */
  /*                                                                       */
  /*    FT_FACE_FLAG_FIXED_SIZES ::                                        */
  /*      Indicates that the face contains `fixed sizes', i.e., bitmap     */
  /*      strikes for some given pixel sizes.  See the `num_fixed_sizes'   */
  /*      and `available_sizes' fields of @FT_FaceRec.                     */
  /*                                                                       */
  /*    FT_FACE_FLAG_FIXED_WIDTH ::                                        */
  /*      Indicates that the face contains fixed-width characters (like    */
  /*      Courier, Lucido, MonoType, etc.).                                */
  /*                                                                       */
  /*    FT_FACE_FLAG_SFNT ::                                               */
  /*      Indicates that the face uses the `sfnt' storage scheme.  For     */
  /*      now, this means TrueType and OpenType.                           */
  /*                                                                       */
  /*    FT_FACE_FLAG_HORIZONTAL ::                                         */
  /*      Indicates that the face contains horizontal glyph metrics.  This */
  /*      should be set for all common formats.                            */
  /*                                                                       */
  /*    FT_FACE_FLAG_VERTICAL ::                                           */
  /*      Indicates that the face contains vertical glyph metrics.  This   */
  /*      is only available in some formats, not all of them.              */
  /*                                                                       */
  /*    FT_FACE_FLAG_KERNING ::                                            */
  /*      Indicates that the face contains kerning information.  If set,   */
  /*      the kerning distance can be retrieved through the function       */
  /*      @FT_Get_Kerning.  Note that if unset, this function will always  */
  /*      return the vector (0,0).                                         */
  /*                                                                       */
  /*    FT_FACE_FLAG_FAST_GLYPHS ::                                        */
  /*      THIS FLAG IS DEPRECATED.  DO NOT USE OR TEST IT.                 */
  /*                                                                       */
  /*    FT_FACE_FLAG_MULTIPLE_MASTERS ::                                   */
  /*      Indicates that the font contains multiple masters and is capable */
  /*      of interpolating between them.  See the multiple-masters         */
  /*      specific API for details.                                        */
  /*                                                                       */
  /*    FT_FACE_FLAG_GLYPH_NAMES ::                                        */
  /*      Indicates that the font contains glyph names that can be         */
  /*      retrieved through @FT_Get_Glyph_Name.  Note that some TrueType   */
  /*      fonts contain broken glyph name tables.  Use the function        */
  /*      @FT_Has_PS_Glyph_Names when needed.                              */
  /*                                                                       */
  /*    FT_FACE_FLAG_EXTERNAL_STREAM ::                                    */
  /*      Used internally by FreeType to indicate that a face's stream was */
  /*      provided by the client application and should not be destroyed   */
  /*      when @FT_Done_Face is called.  Don't read or test this flag.     */
  /*                                                                       */
#define FT_FACE_FLAG_SCALABLE          ( 1L <<  0 )
#define FT_FACE_FLAG_FIXED_SIZES       ( 1L <<  1 )
#define FT_FACE_FLAG_FIXED_WIDTH       ( 1L <<  2 )
#define FT_FACE_FLAG_SFNT              ( 1L <<  3 )
#define FT_FACE_FLAG_HORIZONTAL        ( 1L <<  4 )
#define FT_FACE_FLAG_VERTICAL          ( 1L <<  5 )
#define FT_FACE_FLAG_KERNING           ( 1L <<  6 )
#define FT_FACE_FLAG_FAST_GLYPHS       ( 1L <<  7 )
#define FT_FACE_FLAG_MULTIPLE_MASTERS  ( 1L <<  8 )
#define FT_FACE_FLAG_GLYPH_NAMES       ( 1L <<  9 )
#define FT_FACE_FLAG_EXTERNAL_STREAM   ( 1L << 10 )

  /* */


  /*************************************************************************/
  /*                                                                       */
  /* @macro:                                                               */
  /*    FT_HAS_HORIZONTAL( face )                                          */
  /*                                                                       */
  /* @description:                                                         */
  /*    A macro that returns true whenever a face object contains          */
  /*    horizontal metrics (this is true for all font formats though).     */
  /*                                                                       */
  /* @also:                                                                */
  /*    @FT_HAS_VERTICAL can be used to check for vertical metrics.        */
  /*                                                                       */
#define FT_HAS_HORIZONTAL( face ) \
          ( face->face_flags & FT_FACE_FLAG_HORIZONTAL )


  /*************************************************************************/
  /*                                                                       */
  /* @macro:                                                               */
  /*    FT_HAS_VERTICAL( face )                                            */
  /*                                                                       */
  /* @description:                                                         */
  /*    A macro that returns true whenever a face object contains vertical */
  /*    metrics.                                                           */
  /*                                                                       */
#define FT_HAS_VERTICAL( face ) \
          ( face->face_flags & FT_FACE_FLAG_VERTICAL )


  /*************************************************************************/
  /*                                                                       */
  /* @macro:                                                               */
  /*    FT_HAS_KERNING( face )                                             */
  /*                                                                       */
  /* @description:                                                         */
  /*    A macro that returns true whenever a face object contains kerning  */
  /*    data that can be accessed with @FT_Get_Kerning.                    */
  /*                                                                       */
#define FT_HAS_KERNING( face ) \
          ( face->face_flags & FT_FACE_FLAG_KERNING )


  /*************************************************************************/
  /*                                                                       */
  /* @macro:                                                               */
  /*    FT_IS_SCALABLE( face )                                             */
  /*                                                                       */
  /* @description:                                                         */
  /*    A macro that returns true whenever a face object contains a        */
  /*    scalable font face (true for TrueType, Type 1, CID, and            */
  /*    OpenType/CFF font formats.                                         */
  /*                                                                       */
#define FT_IS_SCALABLE( face ) \
          ( face->face_flags & FT_FACE_FLAG_SCALABLE )


  /*************************************************************************/
  /*                                                                       */
  /* @macro:                                                               */
  /*    FT_IS_SFNT( face )                                                 */
  /*                                                                       */
  /* @description:                                                         */
  /*    A macro that returns true whenever a face object contains a font   */
  /*    whose format is based on the SFNT storage scheme.  This usually    */
  /*    means: TrueType fonts, OpenType fonts, as well as SFNT-based       */
  /*    embedded bitmap fonts.                                             */
  /*                                                                       */
  /*    If this macro is true, all functions defined in @FT_SFNT_NAMES_H   */
  /*    and @FT_TRUETYPE_TABLES_H are available.                           */
  /*                                                                       */
#define FT_IS_SFNT( face ) \
          ( face->face_flags & FT_FACE_FLAG_SFNT )


  /*************************************************************************/
  /*                                                                       */
  /* @macro:                                                               */
  /*    FT_IS_FIXED_WIDTH( face )                                          */
  /*                                                                       */
  /* @description:                                                         */
  /*    A macro that returns true whenever a face object contains a font   */
  /*    face that contains fixed-width (or "monospace", "fixed-pitch",     */
  /*    etc.) glyphs.                                                      */
  /*                                                                       */
#define FT_IS_FIXED_WIDTH( face ) \
          ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )


  /*************************************************************************/
  /*                                                                       */
  /* @macro:                                                               */
  /*    FT_HAS_FIXED_SIZES( face )                                         */
  /*                                                                       */
  /* @description:                                                         */
  /*    A macro that returns true whenever a face object contains some     */
  /*    embedded bitmaps.  See the `available_sizes' field of the          */
  /*    @FT_FaceRec structure.                                             */
  /*                                                                       */
#define FT_HAS_FIXED_SIZES( face ) \
          ( face->face_flags & FT_FACE_FLAG_FIXED_SIZES )


   /* */


  /*************************************************************************/
  /*                                                                       */
  /* @macro:                                                               */
  /*    FT_HAS_FAST_GLYPHS( face )                                         */
  /*                                                                       */
  /* @description:                                                         */
  /*    Deprecated; indicates that the face contains so-called "fast"      */
  /*    glyph bitmaps.                                                     */
  /*                                                                       */
#define FT_HAS_FAST_GLYPHS( face ) \
          ( face->face_flags & FT_FACE_FLAG_FAST_GLYPHS )


  /*************************************************************************/
  /*                                                                       */
  /* @macro:                                                               */
  /*    FT_HAS_GLYPH_NAMES( face )                                         */
  /*                                                                       */
  /* @description:                                                         */
  /*    A macro that returns true whenever a face object contains some     */
  /*    glyph names that can be accessed through @FT_Get_Glyph_Name.       */
  /*                                                                       */
#define FT_HAS_GLYPH_NAMES( face ) \
          ( face->face_flags & FT_FACE_FLAG_GLYPH_NAMES )


  /*************************************************************************/
  /*                                                                       */
  /* @macro:                                                               */
  /*    FT_HAS_MULTIPLE_MASTERS( face )                                    */
  /*                                                                       */
  /* @description:                                                         */
  /*    A macro that returns true whenever a face object contains some     */
  /*    multiple masters.  The functions provided by                       */
  /*    @FT_MULTIPLE_MASTERS_H are then available to choose the exact      */
  /*    design you want.                                                   */
  /*                                                                       */
#define FT_HAS_MULTIPLE_MASTERS( face ) \
          ( face->face_flags & FT_FACE_FLAG_MULTIPLE_MASTERS )


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_STYLE_FLAG_XXX                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A list of bit-flags used to indicate the style of a given face.    */
  /*    These are used in the `style_flags' field of @FT_FaceRec.          */
  /*                                                                       */
  /* <Values>                                                              */
  /*    FT_STYLE_FLAG_ITALIC ::                                            */
  /*      Indicates that a given face is italicized.                       */
  /*                                                                       */
  /*    FT_STYLE_FLAG_BOLD ::                                              */
  /*      Indicates that a given face is bold.                             */
  /*                                                                       */
#define FT_STYLE_FLAG_ITALIC  ( 1 << 0 )
#define FT_STYLE_FLAG_BOLD    ( 1 << 1 )


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Size_Internal                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An opaque handle to an FT_Size_InternalRec structure, used to      */
  /*    model private data of a given FT_Size object.                      */
  /*                                                                       */
  typedef struct FT_Size_InternalRec_*  FT_Size_Internal;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Size_Metrics                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The size metrics structure returned scaled important distances for */
  /*    a given size object.                                               */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    x_ppem       :: The character width, expressed in integer pixels.  */
  /*                    This is the width of the EM square expressed in    */
  /*                    pixels, hence the term `ppem' (pixels per EM).     */
  /*                                                                       */
  /*    y_ppem       :: The character height, expressed in integer pixels. */
  /*                    This is the height of the EM square expressed in   */
  /*                    pixels, hence the term `ppem' (pixels per EM).     */
  /*                                                                       */
  /*    x_scale      :: A simple 16.16 fixed point format coefficient used */
  /*                    to scale horizontal distances expressed in font    */
  /*                    units to fractional (26.6) pixel coordinates.      */
  /*                                                                       */
  /*    y_scale      :: A simple 16.16 fixed point format coefficient used */
  /*                    to scale vertical distances expressed in font      */
  /*                    units to fractional (26.6) pixel coordinates.      */
  /*                                                                       */
  /*    ascender     :: The ascender, expressed in 26.6 fixed point        */
  /*                    pixels.  Positive for ascenders above the          */
  /*                    baseline.                                          */
  /*                                                                       */
  /*    descender    :: The descender, expressed in 26.6 fixed point       */
  /*                    pixels.  Negative for descenders below the         */
  /*                    baseline.                                          */
  /*                                                                       */
  /*    height       :: The text height, expressed in 26.6 fixed point     */
  /*                    pixels.  Always positive.                          */
  /*                                                                       */
  /*    max_advance  :: Maximum horizontal advance, expressed in 26.6      */
  /*                    fixed point pixels.  Always positive.              */
  /*                                                                       */
  /* <Note>                                                                */
  /*    For scalable fonts, the values of `ascender', `descender', and     */
  /*    `height' are scaled versions of `face->ascender',                  */
  /*    `face->descender', and `face->height', respectively.               */
  /*                                                                       */
  /*    Unfortunately, due to glyph hinting, these values might not be     */
  /*    exact for certain fonts.  They thus must be treated as unreliable  */
  /*    with an error margin of at least one pixel!                        */
  /*                                                                       */
  /*    Indeed, the only way to get the exact pixel ascender and descender */
  /*    is to render _all_ glyphs.  As this would be a definite            */
  /*    performance hit, it is up to client applications to perform such   */
  /*    computations.                                                      */
  /*                                                                       */
  typedef struct  FT_Size_Metrics_
  {
    FT_UShort  x_ppem;      /* horizontal pixels per EM               */
    FT_UShort  y_ppem;      /* vertical pixels per EM                 */

    FT_Fixed   x_scale;     /* two scales used to convert font units  */
    FT_Fixed   y_scale;     /* to 26.6 frac. pixel coordinates        */

    FT_Pos     ascender;    /* ascender in 26.6 frac. pixels          */
    FT_Pos     descender;   /* descender in 26.6 frac. pixels         */
    FT_Pos     height;      /* text height in 26.6 frac. pixels       */
    FT_Pos     max_advance; /* max horizontal advance, in 26.6 pixels */

  } FT_Size_Metrics;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_SizeRec                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    FreeType root size class structure.  A size object models the      */
  /*    resolution and pointsize dependent data of a given face.           */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    face    :: Handle to the parent face object.                       */
  /*                                                                       */
  /*    generic :: A typeless pointer, which is unused by the FreeType     */
  /*               library or any of its drivers.  It can be used by       */
  /*               client applications to link their own data to each size */
  /*               object.                                                 */
  /*                                                                       */
  /*    metrics :: Metrics for this size object.  This field is read-only. */
  /*                                                                       */
  typedef struct  FT_SizeRec_
  {
    FT_Face           face;      /* parent face object              */
    FT_Generic        generic;   /* generic pointer for client uses */
    FT_Size_Metrics   metrics;   /* size metrics                    */
    FT_Size_Internal  internal;

  } FT_SizeRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_SubGlyph                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The subglyph structure is an internal object used to describe      */
  /*    subglyphs (for example, in the case of composites).                */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The subglyph implementation is not part of the high-level API,     */
  /*    hence the forward structure declaration.                           */
  /*                                                                       */
  typedef struct FT_SubGlyphRec_*  FT_SubGlyph;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Slot_Internal                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An opaque handle to an FT_Slot_InternalRec structure, used to      */
  /*    model private data of a given FT_GlyphSlot object.                 */
  /*                                                                       */
  typedef struct FT_Slot_InternalRec_*  FT_Slot_Internal;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_GlyphSlotRec                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    FreeType root glyph slot class structure.  A glyph slot is a       */
  /*    container where individual glyphs can be loaded, be they           */
  /*    vectorial or bitmap/graymaps.                                      */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    library           :: A handle to the FreeType library instance     */
  /*                         this slot belongs to.                         */
  /*                                                                       */
  /*    face              :: A handle to the parent face object.           */
  /*                                                                       */
  /*    next              :: In some cases (like some font tools), several */
  /*                         glyph slots per face object can be a good     */
  /*                         thing.  As this is rare, the glyph slots are  */
  /*                         listed through a direct, single-linked list   */
  /*                         using its `next' field.                       */
  /*                                                                       */
  /*    generic           :: A typeless pointer which is unused by the     */
  /*                         FreeType library or any of its drivers.  It   */
  /*                         can be used by client applications to link    */
  /*                         their own data to each glyph slot object.     */
  /*                                                                       */
  /*    metrics           :: The metrics of the last loaded glyph in the   */
  /*                         slot.  The returned values depend on the last */
  /*                         load flags (see the @FT_Load_Glyph API        */
  /*                         function) and can be expressed either in 26.6 */
  /*                         fractional pixels or font units.              */
  /*                                                                       */
  /*                         Note that even when the glyph image is        */
  /*                         transformed, the metrics are not.             */
  /*                                                                       */
  /*    linearHoriAdvance :: For scalable formats only, this field holds   */
  /*                         the linearly scaled horizontal advance width  */
  /*                         for the glyph (i.e. the scaled and unhinted   */
  /*                         value of the hori advance).  This can be      */
  /*                         important to perform correct WYSIWYG layout.  */
  /*                                                                       */
  /*                         Note that this value is expressed by default  */
  /*                         in 16.16 pixels. However, when the glyph is   */
  /*                         loaded with the FT_LOAD_LINEAR_DESIGN flag,   */
  /*                         this field contains simply the value of the   */
  /*                         advance in original font units.               */
  /*                                                                       */
  /*    linearVertAdvance :: For scalable formats only, this field holds   */
  /*                         the linearly scaled vertical advance height   */
  /*                         for the glyph.  See linearHoriAdvance for     */
  /*                         comments.                                     */
  /*                                                                       */
  /*    advance           :: This is the transformed advance width for the */
  /*                         glyph.                                        */
  /*                                                                       */
  /*    format            :: This field indicates the format of the image  */
  /*                         contained in the glyph slot.  Typically       */
  /*                         FT_GLYPH_FORMAT_BITMAP,                       */
  /*                         FT_GLYPH_FORMAT_OUTLINE, and                  */
  /*                         FT_GLYPH_FORMAT_COMPOSITE, but others are     */
  /*                         possible.                                     */
  /*                                                                       */
  /*    bitmap            :: This field is used as a bitmap descriptor     */
  /*                         when the slot format is                       */
  /*                         FT_GLYPH_FORMAT_BITMAP.  Note that the        */
  /*                         address and content of the bitmap buffer can  */
  /*                         change between calls of @FT_Load_Glyph and a  */
  /*                         few other functions.                          */
  /*                                                                       */
  /*    bitmap_left       :: This is the bitmap's left bearing expressed   */
  /*                         in integer pixels.  Of course, this is only   */
  /*                         valid if the format is                        */
  /*                         FT_GLYPH_FORMAT_BITMAP.                       */
  /*                                                                       */
  /*    bitmap_top        :: This is the bitmap's top bearing expressed in */
  /*                         integer pixels.  Remember that this is the    */
  /*                         distance from the baseline to the top-most    */
  /*                         glyph scanline, upwards y-coordinates being   */
  /*                         *positive*.                                   */
  /*                                                                       */
  /*    outline           :: The outline descriptor for the current glyph  */
  /*                         image if its format is                        */
  /*                         FT_GLYPH_FORMAT_OUTLINE.                      */
  /*                                                                       */
  /*    num_subglyphs     :: The number of subglyphs in a composite glyph. */
  /*                         This field is only valid for the composite    */
  /*                         glyph format that should normally only be     */
  /*                         loaded with the @FT_LOAD_NO_RECURSE flag.     */
  /*                         For now this is internal to FreeType.         */
  /*                                                                       */
  /*    subglyphs         :: An array of subglyph descriptors for          */
  /*                         composite glyphs.  There are `num_subglyphs'  */
  /*                         elements in there.  Currently internal to     */
  /*                         FreeType.                                     */
  /*                                                                       */
  /*    control_data      :: Certain font drivers can also return the      */
  /*                         control data for a given glyph image (e.g.    */
  /*                         TrueType bytecode, Type 1 charstrings, etc.). */
  /*                         This field is a pointer to such data.         */
  /*                                                                       */
  /*    control_len       :: This is the length in bytes of the control    */
  /*                         data.                                         */
  /*                                                                       */
  /*    other             :: Really wicked formats can use this pointer to */
  /*                         present their own glyph image to client apps. */
  /*                         Note that the app will need to know about the */
  /*                         image format.                                 */
  /*                                                                       */
  /*    lsb_delta         :: The difference between hinted and unhinted    */
  /*                         left side bearing while autohinting is        */
  /*                         active.  Zero otherwise.                      */
  /*                                                                       */
  /*    rsb_delta         :: The difference between hinted and unhinted    */
  /*                         right side bearing while autohinting is       */
  /*                         active.  Zero otherwise.                      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If @FT_Load_Glyph is called with default flags (see                */
  /*    @FT_LOAD_DEFAULT) the glyph image is loaded in the glyph slot in   */
  /*    its native format (e.g. a vectorial outline for TrueType and       */
  /*    Type 1 formats).                                                   */
  /*                                                                       */
  /*    This image can later be converted into a bitmap by calling         */
  /*    @FT_Render_Glyph.  This function finds the current renderer for    */
  /*    the native image's format then invokes it.                         */
  /*                                                                       */
  /*    The renderer is in charge of transforming the native image through */
  /*    the slot's face transformation fields, then convert it into a      */
  /*    bitmap that is returned in `slot->bitmap'.                         */
  /*                                                                       */
  /*    Note that `slot->bitmap_left' and `slot->bitmap_top' are also used */
  /*    to specify the position of the bitmap relative to the current pen  */
  /*    position (e.g. coordinates [0,0] on the baseline).  Of course,     */
  /*    `slot->format' is also changed to `FT_GLYPH_FORMAT_BITMAP' .       */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Here a small pseudo code fragment which shows how to use           */
  /*    `lsb_delta' and `rsb_delta':                                       */
  /*                                                                       */
  /*    {                                                                  */
  /*      FT_Pos  origin_x       = 0;                                      */
  /*      FT_Pos  prev_rsb_delta = 0;                                      */
  /*                                                                       */
  /*                                                                       */
  /*      for all glyphs do                                                */
  /*        <compute kern between current and previous glyph and add it to */
  /*         `origin_x'>                                                   */
  /*                                                                       */
  /*        <load glyph with `FT_Load_Glyph'>                              */
  /*                                                                       */
  /*        if ( prev_rsb_delta - face->glyph->lsb_delta >= 32 )           */
  /*          origin_x -= 64;                                              */
  /*        else if ( prev_rsb_delta - face->glyph->lsb_delta < -32 )      */
  /*          origin_x += 64;                                              */
  /*                                                                       */
  /*        prev_rsb_delta = face->glyph->rsb_delta;                       */
  /*                                                                       */
  /*        <save glyph image, or render glyph, or ...>                    */
  /*                                                                       */
  /*        origin_x += face->glyph->advance.x;                            */
  /*      endfor                                                           */
  /*    }                                                                  */
  /*                                                                       */
  typedef struct  FT_GlyphSlotRec_
  {
    FT_Library        library;
    FT_Face           face;
    FT_GlyphSlot      next;
    FT_UInt           reserved;       /* retained for binary compatibility */
    FT_Generic        generic;

    FT_Glyph_Metrics  metrics;
    FT_Fixed          linearHoriAdvance;
    FT_Fixed          linearVertAdvance;
    FT_Vector         advance;

    FT_Glyph_Format   format;

    FT_Bitmap         bitmap;
    FT_Int            bitmap_left;
    FT_Int            bitmap_top;

    FT_Outline        outline;

    FT_UInt           num_subglyphs;
    FT_SubGlyph       subglyphs;

    void*             control_data;
    long              control_len;

    FT_Pos            lsb_delta;
    FT_Pos            rsb_delta;

    void*             other;

    FT_Slot_Internal  internal;

  } FT_GlyphSlotRec;


  /*************************************************************************/
  /*************************************************************************/
  /*                                                                       */
  /*                         F U N C T I O N S                             */
  /*                                                                       */
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Init_FreeType                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a new FreeType library object.  The set of modules     */
  /*    that are registered by this function is determined at build time.  */
  /*                                                                       */
  /* <Output>                                                              */
  /*    alibrary :: A handle to a new library object.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Init_FreeType( FT_Library  *alibrary );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Library_Version                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Return the version of the FreeType library being used.  This is    */
  /*    useful when dynamically linking to the library, since one cannot   */
  /*    use the macros FT_FREETYPE_MAJOR, FT_FREETYPE_MINOR, and           */
  /*    FT_FREETYPE_PATCH.                                                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A source library handle.                                */
  /*                                                                       */
  /* <Output>                                                              */
  /*    amajor :: The major version number.                                */
  /*                                                                       */
  /*    aminor :: The minor version number.                                */
  /*                                                                       */
  /*    apatch :: The patch version number.                                */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The reason why this function takes a 'library' argument is because */
  /*    certain programs implement library initialization in a custom way  */
  /*    that doesn't use `FT_Init_FreeType'.                               */
  /*                                                                       */
  /*    In such cases, the library version might not be available before   */
  /*    the library object has been created.                               */
  /*                                                                       */
  FT_EXPORT( void )
  FT_Library_Version( FT_Library   library,
                      FT_Int      *amajor,
                      FT_Int      *aminor,
                      FT_Int      *apatch );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_FreeType                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given FreeType library object and all of its childs,    */
  /*    including resources, drivers, faces, sizes, etc.                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle to the target library object.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Done_FreeType( FT_Library  library );


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    FT_OPEN_XXX                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A list of bit-field constants used within the `flags' field of the */
  /*    @FT_Open_Args structure.                                           */
  /*                                                                       */
  /* <Values>                                                              */
  /*    FT_OPEN_MEMORY      :: This is a memory-based stream.              */
  /*                                                                       */
  /*    FT_OPEN_STREAM      :: Copy the stream from the `stream' field.    */
  /*                                                                       */
  /*    FT_OPEN_PATHNAME    :: Create a new input stream from a C          */
  /*                           path name.                                  */
  /*                                                                       */
  /*    FT_OPEN_DRIVER      :: Use the `driver' field.                     */
  /*                                                                       */
  /*    FT_OPEN_PARAMS      :: Use the `num_params' & `params' field.      */
  /*                                                                       */
  /*    ft_open_memory      :: Deprecated; use @FT_OPEN_MEMORY instead.    */
  /*                                                                       */
  /*    ft_open_stream      :: Deprecated; use @FT_OPEN_STREAM instead.    */
  /*                                                                       */
  /*    ft_open_pathname    :: Deprecated; use @FT_OPEN_PATHNAME instead.  */
  /*                                                                       */
  /*    ft_open_driver      :: Deprecated; use @FT_OPEN_DRIVER instead.    */
  /*                                                                       */
  /*    ft_open_params      :: Deprecated; use @FT_OPEN_PARAMS instead.    */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The `FT_OPEN_MEMORY', `FT_OPEN_STREAM', and `FT_OPEN_PATHNAME'     */
  /*    flags are mutually exclusive.                                      */
  /*                                                                       */
#define FT_OPEN_MEMORY    0x1
#define FT_OPEN_STREAM    0x2
#define FT_OPEN_PATHNAME  0x4
#define FT_OPEN_DRIVER    0x8
#define FT_OPEN_PARAMS    0x10

#define ft_open_memory    FT_OPEN_MEMORY     /* deprecated */
#define ft_open_stream    FT_OPEN_STREAM     /* deprecated */
#define ft_open_pathname  FT_OPEN_PATHNAME   /* deprecated */
#define ft_open_driver    FT_OPEN_DRIVER     /* deprecated */
#define ft_open_params    FT_OPEN_PARAMS     /* deprecated */


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Parameter                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to pass more or less generic parameters    */
  /*    to @FT_Open_Face.                                                  */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    tag  :: A 4-byte identification tag.                               */
  /*                                                                       */
  /*    data :: A pointer to the parameter data.                           */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The id and function of parameters are driver-specific.             */
  /*                                                                       */
  typedef struct  FT_Parameter_
  {
    FT_ULong    tag;
    FT_Pointer  data;

  } FT_Parameter;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Open_Args                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to indicate how to open a new font file/stream.   */
  /*    A pointer to such a structure can be used as a parameter for the   */
  /*    functions @FT_Open_Face and @FT_Attach_Stream.                     */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    flags       :: A set of bit flags indicating how to use the        */
  /*                   structure.                                          */
  /*                                                                       */
  /*    memory_base :: The first byte of the file in memory.               */
  /*                                                                       */
  /*    memory_size :: The size in bytes of the file in memory.            */
  /*                                                                       */
  /*    pathname    :: A pointer to an 8-bit file pathname.                */
  /*                                                                       */
  /*    stream      :: A handle to a source stream object.                 */
  /*                                                                       */
  /*    driver      :: This field is exclusively used by @FT_Open_Face;    */
  /*                   it simply specifies the font driver to use to open  */
  /*                   the face.  If set to 0, FreeType will try to load   */
  /*                   the face with each one of the drivers in its list.  */
  /*                                                                       */
  /*    num_params  :: The number of extra parameters.                     */
  /*                                                                       */
  /*    params      :: Extra parameters passed to the font driver when     */
  /*                   opening a new face.                                 */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream type is determined by the contents of `flags' which     */
  /*    are tested in the following order by @FT_Open_Face:                */
  /*                                                                       */
  /*    If the `FT_OPEN_MEMORY' bit is set, assume that this is a          */
  /*    memory file of `memory_size' bytes,located at `memory_address'.    */
  /*                                                                       */
  /*    Otherwise, if the `FT_OPEN_STREAM' bit is set, assume that a       */
  /*    custom input stream `stream' is used.                              */
  /*                                                                       */
  /*    Otherwise, if the `FT_OPEN_PATHNAME' bit is set, assume that this  */
  /*    is a normal file and use `pathname' to open it.                    */
  /*                                                                       */
  /*    If the `FT_OPEN_DRIVER' bit is set, @FT_Open_Face will only try to */
  /*    open the file with the driver whose handler is in `driver'.        */
  /*                                                                       */
  /*    If the `FT_OPEN_PARAMS' bit is set, the parameters given by        */
  /*    `num_params' and `params' will be used.  They are ignored          */
  /*    otherwise.                                                         */
  /*                                                                       */
  typedef struct  FT_Open_Args_
  {
    FT_UInt         flags;
    const FT_Byte*  memory_base;
    FT_Long         memory_size;
    FT_String*      pathname;
    FT_Stream       stream;
    FT_Module       driver;
    FT_Int          num_params;
    FT_Parameter*   params;

  } FT_Open_Args;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Face                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new face object from a given resource and typeface index */
  /*    using a pathname to the font file.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library    :: A handle to the library resource.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    pathname   :: A path to the font file.                             */
  /*                                                                       */
  /*    face_index :: The index of the face within the resource.  The      */
  /*                  first face has index 0.                              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aface      :: A handle to a new face object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Unlike FreeType 1.x, this function automatically creates a glyph   */
  /*    slot for the face object which can be accessed directly through    */
  /*    `face->glyph'.                                                     */
  /*                                                                       */
  /*    @FT_New_Face can be used to determine and/or check the font format */
  /*    of a given font resource.  If the `face_index' field is negative,  */
  /*    the function will _not_ return any face handle in `aface';  the    */
  /*    return value is 0 if the font format is recognized, or non-zero    */
  /*    otherwise.                                                         */
  /*                                                                       */
  /*    Each new face object created with this function also owns a        */
  /*    default @FT_Size object, accessible as `face->size'.               */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_New_Face( FT_Library   library,
               const char*  filepathname,
               FT_Long      face_index,
               FT_Face     *aface );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Memory_Face                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new face object from a given resource and typeface index */
  /*    using a font file already loaded into memory.                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library    :: A handle to the library resource.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    file_base  :: A pointer to the beginning of the font data.         */
  /*                                                                       */
  /*    file_size  :: The size of the memory chunk used by the font data.  */
  /*                                                                       */
  /*    face_index :: The index of the face within the resource.  The      */
  /*                  first face has index 0.                              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aface      :: A handle to a new face object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The font data bytes are used _directly_ by the @FT_Face object.    */
  /*    This means that they are not copied, and that the client is        */
  /*    responsible for releasing/destroying them _after_ the              */
  /*    corresponding call to @FT_Done_Face .                              */
  /*                                                                       */
  /*    Unlike FreeType 1.x, this function automatically creates a glyph   */
  /*    slot for the face object which can be accessed directly through    */
  /*    `face->glyph'.                                                     */
  /*                                                                       */
  /*    @FT_New_Memory_Face can be used to determine and/or check the font */
  /*    format of a given font resource.  If the `face_index' field is     */
  /*    negative, the function will _not_ return any face handle in        */
  /*    `aface'; the return value is 0 if the font format is recognized,   */
  /*    or non-zero otherwise.                                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_New_Memory_Face( FT_Library      library,
                      const FT_Byte*  file_base,
                      FT_Long         file_size,
                      FT_Long         face_index,
                      FT_Face        *aface );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Open_Face                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Opens a face object from a given resource and typeface index using */
  /*    an `FT_Open_Args' structure.  If the face object doesn't exist, it */
  /*    will be created.                                                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library    :: A handle to the library resource.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    args       :: A pointer to an `FT_Open_Args' structure which must  */
  /*                  be filled by the caller.                             */
  /*                                                                       */
  /*    face_index :: The index of the face within the resource.  The      */
  /*                  first face has index 0.                              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aface      :: A handle to a new face object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Unlike FreeType 1.x, this function automatically creates a glyph   */
  /*    slot for the face object which can be accessed directly through    */
  /*    `face->glyph'.                                                     */
  /*                                                                       */
  /*    @FT_Open_Face can be used to determine and/or check the font       */
  /*    format of a given font resource.  If the `face_index' field is     */
  /*    negative, the function will _not_ return any face handle in        */
  /*    `*face'; the return value is 0 if the font format is recognized,   */
  /*    or non-zero otherwise.                                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Open_Face( FT_Library           library,
                const FT_Open_Args*  args,
                FT_Long              face_index,
                FT_Face             *aface );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Attach_File                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    `Attaches' a given font file to an existing face.  This is usually */
  /*    to read additional information for a single face object.  For      */
  /*    example, it is used to read the AFM files that come with Type 1    */
  /*    fonts in order to add kerning data and other metrics.              */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face         :: The target face object.                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    filepathname :: An 8-bit pathname naming the `metrics' file.       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If your font file is in memory, or if you want to provide your     */
  /*    own input stream object, use @FT_Attach_Stream.                    */
  /*                                                                       */
  /*    The meaning of the `attach' action (i.e., what really happens when */
  /*    the new file is read) is not fixed by FreeType itself.  It really  */
  /*    depends on the font format (and thus the font driver).             */
  /*                                                                       */
  /*    Client applications are expected to know what they are doing       */
  /*    when invoking this function.  Most drivers simply do not implement */
  /*    file attachments.                                                  */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Attach_File( FT_Face      face,
                  const char*  filepathname );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Attach_Stream                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is similar to @FT_Attach_File with the exception     */
  /*    that it reads the attachment from an arbitrary stream.             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face       :: The target face object.                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    parameters :: A pointer to an FT_Open_Args structure used to       */
  /*                  describe the input stream to FreeType.               */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The meaning of the `attach' (i.e. what really happens when the     */
  /*    new file is read) is not fixed by FreeType itself.  It really      */
  /*    depends on the font format (and thus the font driver).             */
  /*                                                                       */
  /*    Client applications are expected to know what they are doing       */
  /*    when invoking this function.  Most drivers simply do not implement */
  /*    file attachments.                                                  */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Attach_Stream( FT_Face        face,
                    FT_Open_Args*  parameters );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_Face                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Discards a given face object, as well as all of its child slots    */
  /*    and sizes.                                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to a target face object.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Done_Face( FT_Face  face );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Char_Size                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets the character dimensions of a given face object.  The         */
  /*    `char_width' and `char_height' values are used for the width and   */
  /*    height, respectively, expressed in 26.6 fractional points.         */
  /*                                                                       */
  /*    If the horizontal or vertical resolution values are zero, a        */
  /*    default value of 72dpi is used.  Similarly, if one of the          */
  /*    character dimensions is zero, its value is set equal to the other. */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face            :: A handle to a target face object.               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    char_width      :: The character width, in 26.6 fractional points. */
  /*                                                                       */
  /*    char_height     :: The character height, in 26.6 fractional        */
  /*                       points.                                         */
  /*                                                                       */
  /*    horz_resolution :: The horizontal resolution.                      */
  /*                                                                       */
  /*    vert_resolution :: The vertical resolution.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    When dealing with fixed-size faces (i.e., non-scalable formats),   */
  /*    @FT_Set_Pixel_Sizes provides a more convenient interface.          */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Set_Char_Size( FT_Face     face,
                    FT_F26Dot6  char_width,
                    FT_F26Dot6  char_height,
                    FT_UInt     horz_resolution,
                    FT_UInt     vert_resolution );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Pixel_Sizes                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets the character dimensions of a given face object.  The width   */
  /*    and height are expressed in integer pixels.                        */
  /*                                                                       */
  /*    If one of the character dimensions is zero, its value is set equal */
  /*    to the other.                                                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face         :: A handle to the target face object.                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    pixel_width  :: The character width, in integer pixels.            */
  /*                                                                       */
  /*    pixel_height :: The character height, in integer pixels.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The values of `pixel_width' and `pixel_height' correspond to the   */
  /*    pixel values of the _typographic_ character size, which are NOT    */
  /*    necessarily the same as the dimensions of the glyph `bitmap        */
  /*    cells'.                                                            */
  /*                                                                       */
  /*    The `character size' is really the size of an abstract square      */
  /*    called the `EM', used to design the font.  However, depending      */
  /*    on the font design, glyphs will be smaller or greater than the     */
  /*    EM.                                                                */
  /*                                                                       */
  /*    This means that setting the pixel size to, say, 8x8 doesn't        */
  /*    guarantee in any way that you will get glyph bitmaps that all fit  */
  /*    within an 8x8 cell (sometimes even far from it).                   */
  /*                                                                       */
  /*    For bitmap fonts, `pixel_height' usually is a reliable value for   */
  /*    the height of the bitmap cell.  Drivers for bitmap font formats    */
  /*    which contain a single bitmap strike only (BDF, PCF, FNT) ignore   */
  /*    `pixel_width'.                                                     */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Set_Pixel_Sizes( FT_Face  face,
                      FT_UInt  pixel_width,
                      FT_UInt  pixel_height );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Load_Glyph                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to load a single glyph within a given glyph slot,  */
  /*    for a given size.                                                  */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face        :: A handle to the target face object where the glyph  */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph_index :: The index of the glyph in the font file.  For       */
  /*                   CID-keyed fonts (either in PS or in CFF format)     */
  /*                   this argument specifies the CID value.              */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   @FT_LOAD_XXX constants can be used to control the   */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If the glyph image is not a bitmap, and if the bit flag            */
  /*    FT_LOAD_IGNORE_TRANSFORM is unset, the glyph image will be         */
  /*    transformed with the information passed to a previous call to      */
  /*    @FT_Set_Transform.                                                 */
  /*                                                                       */
  /*    Note that this also transforms the `face.glyph.advance' field, but */
  /*    *not* the values in `face.glyph.metrics'.                          */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Load_Glyph( FT_Face   face,
                 FT_UInt   glyph_index,
                 FT_Int32  load_flags );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Load_Char                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to load a single glyph within a given glyph slot,  */
  /*    for a given size, according to its character code.                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face        :: A handle to a target face object where the glyph    */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    char_code   :: The glyph's character code, according to the        */
  /*                   current charmap used in the face.                   */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   @FT_LOAD_XXX constants can be used to control the   */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If the face has no current charmap, or if the character code       */
  /*    is not defined in the charmap, this function will return an        */
  /*    error.                                                             */
  /*                                                                       */
  /*    If the glyph image is not a bitmap, and if the bit flag            */
  /*    FT_LOAD_IGNORE_TRANSFORM is unset, the glyph image will be         */
  /*    transformed with the information passed to a previous call to      */
  /*    @FT_Set_Transform.                                                 */
  /*                                                                       */
  /*    Note that this also transforms the `face.glyph.advance' field, but */
  /*    *not* the values in `face.glyph.metrics'.                          */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Load_Char( FT_Face   face,
                FT_ULong  char_code,
                FT_Int32  load_flags );


 /***************************************************************************
  *
  * @enum:
  *   FT_LOAD_XXX
  *
  * @description:
  *   A list of bit-field constants, used with @FT_Load_Glyph to indicate
  *   what kind of operations to perform during glyph loading.
  *
  * @values:
  *   FT_LOAD_DEFAULT ::
  *     Corresponding to 0, this value is used a default glyph load.  In this
  *     case, the following will happen:
  *
  *     1. FreeType looks for a bitmap for the glyph corresponding to the
  *        face's current size.  If one is found, the function returns.  The
  *        bitmap data can be accessed from the glyph slot (see note below).
  *
  *     2. If no embedded bitmap is searched or found, FreeType looks for a
  *        scalable outline.  If one is found, it is loaded from the font
  *        file, scaled to device pixels, then "hinted" to the pixel grid in
  *        order to optimize it.  The outline data can be accessed from the
  *        glyph slot (see note below).
  *
  *     Note that by default, the glyph loader doesn't render outlines into
  *     bitmaps.  The following flags are used to modify this default
  *     behaviour to more specific and useful cases.
  *
  *   FT_LOAD_NO_SCALE ::
  *     Don't scale the vector outline being loaded to 26.6 fractional
  *     pixels, but kept in font units.  Note that this also disables
  *     hinting and the loading of embedded bitmaps.  You should only use it
  *     when you want to retrieve the original glyph outlines in font units.
  *
  *   FT_LOAD_NO_HINTING ::
  *     Don't hint glyph outlines after their scaling to device pixels.
  *     This generally generates "blurrier" glyphs in anti-aliased modes.
  *
  *     This flag is ignored if @FT_LOAD_NO_SCALE is set.
  *
  *   FT_LOAD_RENDER ::
  *     Render the glyph outline immediately into a bitmap before the glyph
  *     loader returns.  By default, the glyph is rendered for the
  *     @FT_RENDER_MODE_NORMAL mode, which corresponds to 8-bit anti-aliased
  *     bitmaps using 256 opacity levels.  You can use either
  *     @FT_LOAD_TARGET_MONO or @FT_LOAD_MONOCHROME to render 1-bit
  *     monochrome bitmaps.
  *
  *     This flag is ignored if @FT_LOAD_NO_SCALE is set.
  *
  *   FT_LOAD_NO_BITMAP ::
  *     Don't look for bitmaps when loading the glyph.  Only scalable
  *     outlines will be loaded when available, and scaled, hinted, or
  *     rendered depending on other bit flags.
  *
  *     This does not prevent you from rendering outlines to bitmaps
  *     with @FT_LOAD_RENDER, however.
  *
  *   FT_LOAD_VERTICAL_LAYOUT ::
  *     Prepare the glyph image for vertical text layout.  This basically
  *     means that `face.glyph.advance' will correspond to the vertical
  *     advance height (instead of the default horizontal advance width),
  *     and that the glyph image will be translated to match the vertical
  *     bearings positions.
  *
  *   FT_LOAD_FORCE_AUTOHINT ::
  *     Force the use of the FreeType auto-hinter when a glyph outline is
  *     loaded.  You shouldn't need this in a typical application, since it
  *     is mostly used to experiment with its algorithm.
  *
  *   FT_LOAD_CROP_BITMAP ::
  *     Indicates that the glyph loader should try to crop the bitmap (i.e.,
  *     remove all space around its black bits) when loading it.  This is
  *     only useful when loading embedded bitmaps in certain fonts, since
  *     bitmaps rendered with @FT_LOAD_RENDER are always cropped by default.
  *
  *   FT_LOAD_PEDANTIC ::
  *     Indicates that the glyph loader should perform pedantic
  *     verifications during glyph loading, rejecting invalid fonts.  This
  *     is mostly used to detect broken glyphs in fonts.  By default,
  *     FreeType tries to handle broken fonts also.
  *
  *   FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH ::
  *     Indicates that the glyph loader should ignore the global advance
  *     width defined in the font.  As far as we know, this is only used by
  *     the X-TrueType font server, in order to deal correctly with the
  *     incorrect metrics contained in DynaLab's TrueType CJK fonts.
  *
  *   FT_LOAD_NO_RECURSE ::
  *     This flag is only used internally.  It merely indicates that the
  *     glyph loader should not load composite glyphs recursively.  Instead,
  *     it should set the `num_subglyph' and `subglyphs' values of the glyph
  *     slot accordingly, and set "glyph->format" to
  *     @FT_GLYPH_FORMAT_COMPOSITE.
  *
  *     The description of sub-glyphs is not available to client
  *     applications for now.
  *
  *   FT_LOAD_IGNORE_TRANSFORM ::
  *     Indicates that the glyph loader should not try to transform the
  *     loaded glyph image.  This doesn't prevent scaling, hinting, or
  *     rendering.
  *
  *   FT_LOAD_MONOCHROME ::
  *     This flag is used with @FT_LOAD_RENDER to indicate that you want
  *     to render a 1-bit monochrome glyph bitmap from a vectorial outline.
  *
  *     Note that this has no effect on the hinting algorithm used by the
  *     glyph loader.  You should better use @FT_LOAD_TARGET_MONO if you
  *     want to render monochrome-optimized glyph images instead.
  *
  *   FT_LOAD_LINEAR_DESIGN ::
  *     Return the linearly scaled metrics expressed in original font units
  *     instead of the default 16.16 pixel values.
  *
  *   FT_LOAD_NO_AUTOHINT ::
  *     Indicates that the auto-hinter should never be used to hint glyph
  *     outlines.  This doesn't prevent native format-specific hinters from
  *     being used.  This can be important for certain fonts where unhinted
  *     output is better than auto-hinted one.
  *
  *   FT_LOAD_TARGET_NORMAL ::
  *     Use hinting for @FT_RENDER_MODE_NORMAL.
  *
  *   FT_LOAD_TARGET_LIGHT ::
  *     Use hinting for @FT_RENDER_MODE_LIGHT.
  *
  *   FT_LOAD_TARGET_MONO ::
  *     Use hinting for @FT_RENDER_MODE_MONO.
  *
  *   FT_LOAD_TARGET_LCD ::
  *     Use hinting for @FT_RENDER_MODE_LCD.
  *
  *   FT_LOAD_TARGET_LCD_V ::
  *     Use hinting for @FT_RENDER_MODE_LCD_V.
  */
#define FT_LOAD_DEFAULT                      0x0
#define FT_LOAD_NO_SCALE                     0x1
#define FT_LOAD_NO_HINTING                   0x2
#define FT_LOAD_RENDER                       0x4
#define FT_LOAD_NO_BITMAP                    0x8
#define FT_LOAD_VERTICAL_LAYOUT              0x10
#define FT_LOAD_FORCE_AUTOHINT               0x20
#define FT_LOAD_CROP_BITMAP                  0x40
#define FT_LOAD_PEDANTIC                     0x80
#define FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH  0x200
#define FT_LOAD_NO_RECURSE                   0x400
#define FT_LOAD_IGNORE_TRANSFORM             0x800
#define FT_LOAD_MONOCHROME                   0x1000
#define FT_LOAD_LINEAR_DESIGN                0x2000

  /* temporary hack! */
#define FT_LOAD_SBITS_ONLY                   0x4000
#define FT_LOAD_NO_AUTOHINT                  0x8000U

  /* */

#define FT_LOAD_TARGET_( x )      ( (FT_Int32)( (x) & 15 ) << 16 )
#define FT_LOAD_TARGET_MODE( x )  ( (FT_Render_Mode)( ( (x) >> 16 ) & 15 ) )

#define FT_LOAD_TARGET_NORMAL     FT_LOAD_TARGET_( FT_RENDER_MODE_NORMAL )
#define FT_LOAD_TARGET_LIGHT      FT_LOAD_TARGET_( FT_RENDER_MODE_LIGHT  )
#define FT_LOAD_TARGET_MONO       FT_LOAD_TARGET_( FT_RENDER_MODE_MONO   )
#define FT_LOAD_TARGET_LCD        FT_LOAD_TARGET_( FT_RENDER_MODE_LCD    )
#define FT_LOAD_TARGET_LCD_V      FT_LOAD_TARGET_( FT_RENDER_MODE_LCD_V  )

  /* */


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Transform                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to set the transformation that is applied to glyph */
  /*    images just before they are converted to bitmaps in a glyph slot   */
  /*    when @FT_Render_Glyph is called.                                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the source face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    matrix :: A pointer to the transformation's 2x2 matrix.  Use 0 for */
  /*              the identity matrix.                                     */
  /*    delta  :: A pointer to the translation vector.  Use 0 for the null */
  /*              vector.                                                  */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The transformation is only applied to scalable image formats after */
  /*    the glyph has been loaded.  It means that hinting is unaltered by  */
  /*    the transformation and is performed on the character size given in */
  /*    the last call to @FT_Set_Char_Size or @FT_Set_Pixel_Sizes.         */
  /*                                                                       */
  FT_EXPORT( void )
  FT_Set_Transform( FT_Face     face,
                    FT_Matrix*  matrix,
                    FT_Vector*  delta );


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    FT_Render_Mode                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An enumeration type that lists the render modes supported by       */
  /*    FreeType 2.  Each mode corresponds to a specific type of scanline  */
  /*    conversion performed on the outline, as well as specific           */
  /*    hinting optimizations.                                             */
  /*                                                                       */
  /*    For bitmap fonts the `bitmap->pixel_mode' field in the             */
  /*    @FT_GlyphSlotRec structure gives the format of the returned        */
  /*    bitmap.                                                            */
  /*                                                                       */
  /* <Values>                                                              */
  /*    FT_RENDER_MODE_NORMAL ::                                           */
  /*      This is the default render mode; it corresponds to 8-bit         */
  /*      anti-aliased bitmaps, using 256 levels of opacity.               */
  /*                                                                       */
  /*    FT_RENDER_MODE_LIGHT ::                                            */
  /*      This is similar to @FT_RENDER_MODE_NORMAL -- you have to use     */
  /*      @FT_LOAD_TARGET_LIGHT in calls to @FT_Load_Glyph to get any      */
  /*      effect since the rendering process no longer influences the      */
  /*      positioning of glyph outlines.                                   */
  /*                                                                       */
  /*      The resulting glyph shapes are more similar to the original,     */
  /*      while being a bit more fuzzy (`better shapes' instead of `better */
  /*      contrast', so to say.                                            */
  /*                                                                       */
  /*    FT_RENDER_MODE_MONO ::                                             */
  /*      This mode corresponds to 1-bit bitmaps.                          */
  /*                                                                       */
  /*    FT_RENDER_MODE_LCD ::                                              */
  /*      This mode corresponds to horizontal RGB/BGR sub-pixel displays,  */
  /*      like LCD-screens.  It produces 8-bit bitmaps that are 3 times    */
  /*      the width of the original glyph outline in pixels, and which use */
  /*      the @FT_PIXEL_MODE_LCD mode.                                     */
  /*                                                                       */
  /*    FT_RENDER_MODE_LCD_V ::                                            */
  /*      This mode corresponds to vertical RGB/BGR sub-pixel displays     */
  /*      (like PDA screens, rotated LCD displays, etc.).  It produces     */
  /*      8-bit bitmaps that are 3 times the height of the original        */
  /*      glyph outline in pixels and use the @FT_PIXEL_MODE_LCD_V mode.   */
  /*                                                                       */
  /* <Note>                                                                */
  /*   The LCD-optimized glyph bitmaps produced by FT_Render_Glyph are     */
  /*   _not filtered_ to reduce color-fringes.  It is up to the caller to  */
  /*   perform this pass.                                                  */
  /*                                                                       */
  typedef enum  FT_Render_Mode_
  {
    FT_RENDER_MODE_NORMAL = 0,
    FT_RENDER_MODE_LIGHT,
    FT_RENDER_MODE_MONO,
    FT_RENDER_MODE_LCD,
    FT_RENDER_MODE_LCD_V,

    FT_RENDER_MODE_MAX

  } FT_Render_Mode;


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    ft_render_mode_xxx                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    These constats are deprecated.  Use the corresponding              */
  /*    @FT_Render_Mode values instead.                                    */
  /*                                                                       */
  /* <Values>                                                              */
  /*   ft_render_mode_normal :: see @FT_RENDER_MODE_NORMAL                 */
  /*   ft_render_mode_mono   :: see @FT_RENDER_MODE_MONO                   */
  /*                                                                       */
#define ft_render_mode_normal  FT_RENDER_MODE_NORMAL
#define ft_render_mode_mono    FT_RENDER_MODE_MONO


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Render_Glyph                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts a given glyph image to a bitmap.  It does so by           */
  /*    inspecting the glyph image format, find the relevant renderer, and */
  /*    invoke it.                                                         */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    slot        :: A handle to the glyph slot containing the image to  */
  /*                   convert.                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    render_mode :: This is the render mode used to render the glyph    */
  /*                   image into a bitmap.  See FT_Render_Mode for a list */
  /*                   of possible values.                                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Render_Glyph( FT_GlyphSlot    slot,
                   FT_Render_Mode  render_mode );


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    FT_Kerning_Mode                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An enumeration used to specify which kerning values to return in   */
  /*    @FT_Get_Kerning.                                                   */
  /*                                                                       */
  /* <Values>                                                              */
  /*    FT_KERNING_DEFAULT  :: Return scaled and grid-fitted kerning       */
  /*                           distances (value is 0).                     */
  /*                                                                       */
  /*    FT_KERNING_UNFITTED :: Return scaled but un-grid-fitted kerning    */
  /*                           distances.                                  */
  /*                                                                       */
  /*    FT_KERNING_UNSCALED :: Return the kerning vector in original font  */
  /*                           units.                                      */
  /*                                                                       */
  typedef enum  FT_Kerning_Mode_
  {
    FT_KERNING_DEFAULT  = 0,
    FT_KERNING_UNFITTED,
    FT_KERNING_UNSCALED

  } FT_Kerning_Mode;


  /*************************************************************************/
  /*                                                                       */
  /* <Const>                                                               */
  /*    ft_kerning_default                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This constant is deprecated.  Please use @FT_KERNING_DEFAULT       */
  /*    instead.                                                           */
  /*                                                                       */
#define ft_kerning_default   FT_KERNING_DEFAULT


  /*************************************************************************/
  /*                                                                       */
  /* <Const>                                                               */
  /*    ft_kerning_unfitted                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This constant is deprecated.  Please use @FT_KERNING_UNFITTED      */
  /*    instead.                                                           */
  /*                                                                       */
#define ft_kerning_unfitted  FT_KERNING_UNFITTED


  /*************************************************************************/
  /*                                                                       */
  /* <Const>                                                               */
  /*    ft_kerning_unscaled                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This constant is deprecated.  Please use @FT_KERNING_UNSCALED      */
  /*    instead.                                                           */
  /*                                                                       */
#define ft_kerning_unscaled  FT_KERNING_UNSCALED


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Kerning                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the kerning vector between two glyphs of a same face.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to a source face object.                   */
  /*                                                                       */
  /*    left_glyph  :: The index of the left glyph in the kern pair.       */
  /*                                                                       */
  /*    right_glyph :: The index of the right glyph in the kern pair.      */
  /*                                                                       */
  /*    kern_mode   :: See @FT_Kerning_Mode for more information.          */
  /*                   Determines the scale/dimension of the returned      */
  /*                   kerning vector.                                     */
  /*                                                                       */
  /* <Output>                                                              */
  /*    akerning    :: The kerning vector.  This is in font units for      */
  /*                   scalable formats, and in pixels for fixed-sizes     */
  /*                   formats.                                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only horizontal layouts (left-to-right & right-to-left) are        */
  /*    supported by this method.  Other layouts, or more sophisticated    */
  /*    kernings, are out of the scope of this API function -- they can be */
  /*    implemented through format-specific interfaces.                    */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Get_Kerning( FT_Face     face,
                  FT_UInt     left_glyph,
                  FT_UInt     right_glyph,
                  FT_UInt     kern_mode,
                  FT_Vector  *akerning );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Glyph_Name                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the ASCII name of a given glyph in a face.  This only    */
  /*    works for those faces where FT_HAS_GLYPH_NAME(face) returns true.  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to a source face object.                   */
  /*                                                                       */
  /*    glyph_index :: The glyph index.                                    */
  /*                                                                       */
  /*    buffer_max  :: The maximal number of bytes available in the        */
  /*                   buffer.                                             */
  /*                                                                       */
  /* <Output>                                                              */
  /*    buffer      :: A pointer to a target buffer where the name will be */
  /*                   copied to.                                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    An error is returned if the face doesn't provide glyph names or if */
  /*    the glyph index is invalid.  In all cases of failure, the first    */
  /*    byte of `buffer' will be set to 0 to indicate an empty name.       */
  /*                                                                       */
  /*    The glyph name is truncated to fit within the buffer if it is too  */
  /*    long.  The returned string is always zero-terminated.              */
  /*                                                                       */
  /*    This function is not compiled within the library if the config     */
  /*    macro FT_CONFIG_OPTION_NO_GLYPH_NAMES is defined in                */
  /*    `include/freetype/config/ftoptions.h'                              */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Get_Glyph_Name( FT_Face     face,
                     FT_UInt     glyph_index,
                     FT_Pointer  buffer,
                     FT_UInt     buffer_max );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Postscript_Name                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the ASCII Postscript name of a given face, if available. */
  /*    This should only work with Postscript and TrueType fonts.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the source face object.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A pointer to the face's Postscript name.  NULL if un-available.    */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The returned pointer is owned by the face and will be destroyed    */
  /*    with it.                                                           */
  /*                                                                       */
  FT_EXPORT( const char* )
  FT_Get_Postscript_Name( FT_Face  face );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Select_Charmap                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Selects a given charmap by its encoding tag (as listed in          */
  /*    `freetype.h').                                                     */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face     :: A handle to the source face object.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    encoding :: A handle to the selected charmap.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function will return an error if no charmap in the face       */
  /*    corresponds to the encoding queried here.                          */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Select_Charmap( FT_Face      face,
                     FT_Encoding  encoding );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Charmap                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Selects a given charmap for character code to glyph index          */
  /*    decoding.                                                          */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face    :: A handle to the source face object.                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charmap :: A handle to the selected charmap.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function will return an error if the charmap is not part of   */
  /*    the face (i.e., if it is not listed in the face->charmaps[]        */
  /*    table).                                                            */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Set_Charmap( FT_Face     face,
                  FT_CharMap  charmap );


  /*************************************************************************/
  /*                                                                       */
  /* @function:                                                            */
  /*    FT_Get_Charmap_Index                                               */
  /*                                                                       */
  /* @description:                                                         */
  /*    Retrieve index of a given charmap.                                 */
  /*                                                                       */
  /* @input:                                                               */
  /*    charmap :: A handle to a charmap.                                  */
  /*                                                                       */
  /* @return:                                                              */
  /*    The index into the array of character maps within the face to      */
  /*    which `charmap' belongs.                                           */
  /*                                                                       */
  FT_EXPORT( FT_Int )
  FT_Get_Charmap_Index( FT_CharMap  charmap );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Char_Index                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the glyph index of a given character code.  This function  */
  /*    uses a charmap object to do the translation.                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the source face object.                    */
  /*                                                                       */
  /*    charcode :: The character code.                                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The glyph index.  0 means `undefined character code'.              */
  /*                                                                       */
  /* <Note>                                                                */
  /*    FreeType computes its own glyph indices which are not necessarily  */
  /*    the same as used in the font in case the font is based on glyph    */
  /*    indices.  Reason for this behaviour is to assure that index 0 is   */
  /*    never used, representing the missing glyph.                        */
  /*                                                                       */
  FT_EXPORT( FT_UInt )
  FT_Get_Char_Index( FT_Face   face,
                     FT_ULong  charcode );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_First_Char                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used to return the first character code in the    */
  /*    current charmap of a given face.  It will also return the          */
  /*    corresponding glyph index.                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face    :: A handle to the source face object.                     */
  /*                                                                       */
  /* <Output>                                                              */
  /*    agindex :: Glyph index of first character code.  0 if charmap is   */
  /*               empty.                                                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The charmap's first character code.                                */
  /*                                                                       */
  /* <Note>                                                                */
  /*    You should use this function with @FT_Get_Next_Char to be able to  */
  /*    parse all character codes available in a given charmap.  The code  */
  /*    should look like this:                                             */
  /*                                                                       */
  /*    {                                                                  */
  /*      FT_ULong  charcode;                                              */
  /*      FT_UInt   gindex;                                                */
  /*                                                                       */
  /*                                                                       */
  /*      charcode = FT_Get_First_Char( face, &gindex );                   */
  /*      while ( gindex != 0 )                                            */
  /*      {                                                                */
  /*        ... do something with (charcode,gindex) pair ...               */
  /*                                                                       */
  /*        charcode = FT_Get_Next_Char( face, charcode, &gindex );        */
  /*      }                                                                */
  /*    }                                                                  */
  /*                                                                       */
  /*    Note that `*agindex' will be set to 0 if the charmap is empty.     */
  /*    The result itself can be 0 in two cases: if the charmap is empty   */
  /*    or when the value 0 is the first valid character code.             */
  /*                                                                       */
  FT_EXPORT( FT_ULong )
  FT_Get_First_Char( FT_Face   face,
                     FT_UInt  *agindex );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Next_Char                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used to return the next character code in the     */
  /*    current charmap of a given face following the value 'char_code',   */
  /*    as well as the corresponding glyph index.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face      :: A handle to the source face object.                   */
  /*    char_code :: The starting character code.                          */
  /*                                                                       */
  /* <Output>                                                              */
  /*    agindex   :: Glyph index of first character code.  0 if charmap    */
  /*                 is empty.                                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The charmap's next character code.                                 */
  /*                                                                       */
  /* <Note>                                                                */
  /*    You should use this function with @FT_Get_First_Char to walk       */
  /*    through all character codes available in a given charmap.  See     */
  /*    the note for this function for a simple code example.              */
  /*                                                                       */
  /*    Note that `*agindex' will be set to 0 when there are no more codes */
  /*    in the charmap.                                                    */
  /*                                                                       */
  FT_EXPORT( FT_ULong )
  FT_Get_Next_Char( FT_Face    face,
                    FT_ULong   char_code,
                    FT_UInt   *agindex );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Name_Index                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the glyph index of a given glyph name.  This function uses */
  /*    driver specific objects to do the translation.                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face       :: A handle to the source face object.                  */
  /*                                                                       */
  /*    glyph_name :: The glyph name.                                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The glyph index.  0 means `undefined character code'.              */
  /*                                                                       */
  FT_EXPORT( FT_UInt )
  FT_Get_Name_Index( FT_Face     face,
                     FT_String*  glyph_name );



  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    computations                                                       */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Computations                                                       */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    Crunching fixed numbers and vectors                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section contains various functions used to perform            */
  /*    computations on 16.16 fixed-float numbers or 2d vectors.           */
  /*                                                                       */
  /* <Order>                                                               */
  /*    FT_MulDiv                                                          */
  /*    FT_MulFix                                                          */
  /*    FT_DivFix                                                          */
  /*    FT_RoundFix                                                        */
  /*    FT_CeilFix                                                         */
  /*    FT_FloorFix                                                        */
  /*    FT_Vector_Transform                                                */
  /*    FT_Matrix_Multiply                                                 */
  /*    FT_Matrix_Invert                                                   */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_MulDiv                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to perform the computation `(a*b)/c'   */
  /*    with maximal accuracy (it uses a 64-bit intermediate integer       */
  /*    whenever necessary).                                               */
  /*                                                                       */
  /*    This function isn't necessarily as fast as some processor specific */
  /*    operations, but is at least completely portable.                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The first multiplier.                                         */
  /*    b :: The second multiplier.                                        */
  /*    c :: The divisor.                                                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `(a*b)/c'.  This function never traps when trying to */
  /*    divide by zero; it simply returns `MaxInt' or `MinInt' depending   */
  /*    on the signs of `a' and `b'.                                       */
  /*                                                                       */
  FT_EXPORT( FT_Long )
  FT_MulDiv( FT_Long  a,
             FT_Long  b,
             FT_Long  c );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_MulFix                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to perform the computation             */
  /*    `(a*b)/0x10000' with maximal accuracy.  Most of the time this is   */
  /*    used to multiply a given value by a 16.16 fixed float factor.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The first multiplier.                                         */
  /*    b :: The second multiplier.  Use a 16.16 factor here whenever      */
  /*         possible (see note below).                                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `(a*b)/0x10000'.                                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function has been optimized for the case where the absolute   */
  /*    value of `a' is less than 2048, and `b' is a 16.16 scaling factor. */
  /*    As this happens mainly when scaling from notional units to         */
  /*    fractional pixels in FreeType, it resulted in noticeable speed     */
  /*    improvements between versions 2.x and 1.x.                         */
  /*                                                                       */
  /*    As a conclusion, always try to place a 16.16 factor as the         */
  /*    _second_ argument of this function; this can make a great          */
  /*    difference.                                                        */
  /*                                                                       */
  FT_EXPORT( FT_Long )
  FT_MulFix( FT_Long  a,
             FT_Long  b );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_DivFix                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to perform the computation             */
  /*    `(a*0x10000)/b' with maximal accuracy.  Most of the time, this is  */
  /*    used to divide a given value by a 16.16 fixed float factor.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The first multiplier.                                         */
  /*    b :: The second multiplier.  Use a 16.16 factor here whenever      */
  /*         possible (see note below).                                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `(a*0x10000)/b'.                                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The optimization for FT_DivFix() is simple: If (a << 16) fits in   */
  /*    32 bits, then the division is computed directly.  Otherwise, we    */
  /*    use a specialized version of @FT_MulDiv.                           */
  /*                                                                       */
  FT_EXPORT( FT_Long )
  FT_DivFix( FT_Long  a,
             FT_Long  b );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_RoundFix                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to round a 16.16 fixed number.         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The number to be rounded.                                     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `(a + 0x8000) & -0x10000'.                           */
  /*                                                                       */
  FT_EXPORT( FT_Fixed )
  FT_RoundFix( FT_Fixed  a );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_CeilFix                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to compute the ceiling function of a   */
  /*    16.16 fixed number.                                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The number for which the ceiling function is to be computed.  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `(a + 0x10000 - 1) & -0x10000'.                      */
  /*                                                                       */
  FT_EXPORT( FT_Fixed )
  FT_CeilFix( FT_Fixed  a );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_FloorFix                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to compute the floor function of a     */
  /*    16.16 fixed number.                                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The number for which the floor function is to be computed.    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `a & -0x10000'.                                      */
  /*                                                                       */
  FT_EXPORT( FT_Fixed )
  FT_FloorFix( FT_Fixed  a );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Vector_Transform                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Transforms a single vector through a 2x2 matrix.                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    vector :: The target vector to transform.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    matrix :: A pointer to the source 2x2 matrix.                      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The result is undefined if either `vector' or `matrix' is invalid. */
  /*                                                                       */
  FT_EXPORT( void )
  FT_Vector_Transform( FT_Vector*  vec,
                       FT_Matrix*  matrix );


  /* */


FT_END_HEADER

#endif /* __FREETYPE_H__ */


/* END */
