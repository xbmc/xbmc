/***************************************************************************/
/*                                                                         */
/*  ftcache.h                                                              */
/*                                                                         */
/*    FreeType Cache subsystem (specification).                            */
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


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*********                                                       *********/
  /*********             WARNING, THIS IS BETA CODE.               *********/
  /*********                                                       *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#ifndef __FTCACHE_H__
#define __FTCACHE_H__


#include <ft2build.h>
#include FT_GLYPH_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    cache_subsystem                                                    */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Cache Sub-System                                                   */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    How to cache face, size, and glyph data with FreeType 2.           */
  /*                                                                       */
  /* <Description>                                                         */
  /*   This section describes the FreeType 2 cache sub-system which is     */
  /*   still in beta.                                                      */
  /*                                                                       */
  /* <Order>                                                               */
  /*   FTC_Manager                                                         */
  /*   FTC_FaceID                                                          */
  /*   FTC_Face_Requester                                                  */
  /*                                                                       */
  /*   FTC_Manager_New                                                     */
  /*   FTC_Manager_Reset                                                   */
  /*   FTC_Manager_Done                                                    */
  /*   FTC_Manager_LookupFace                                              */
  /*   FTC_Manager_LookupSize                                              */
  /*   FTC_Manager_RemoveFaceID                                            */
  /*                                                                       */
  /*   FTC_Node                                                            */
  /*   FTC_Node_Unref                                                      */
  /*                                                                       */
  /*   FTC_Font                                                            */
  /*   FTC_ImageCache                                                      */
  /*   FTC_ImageCache_New                                                  */
  /*   FTC_ImageCache_Lookup                                               */
  /*                                                                       */
  /*   FTC_SBit                                                            */
  /*   FTC_SBitCache                                                       */
  /*   FTC_SBitCache_New                                                   */
  /*   FTC_SBitCache_Lookup                                                */
  /*                                                                       */
  /*   FTC_CMapCache                                                       */
  /*   FTC_CMapCache_New                                                   */
  /*   FTC_CMapCache_Lookup                                                */
  /*                                                                       */
  /*                                                                       */
  /*   FTC_Image_Desc                                                      */
  /*   FTC_Image_Cache                                                     */
  /*   FTC_Image_Cache_Lookup                                              */
  /*                                                                       */
  /*   FTC_SBit_Cache                                                      */
  /*   FTC_SBit_Cache_Lookup                                               */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    BASIC TYPE DEFINITIONS                     *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_FaceID                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An opaque pointer type that is used to identity face objects.  The */
  /*    contents of such objects is application-dependent.                 */
  /*                                                                       */
  typedef struct FTC_FaceIDRec_*  FTC_FaceID;


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FTC_Face_Requester                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A callback function provided by client applications.  It is used   */
  /*    to translate a given @FTC_FaceID into a new valid @FT_Face object. */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face_id :: The face ID to resolve.                                 */
  /*                                                                       */
  /*    library :: A handle to a FreeType library object.                  */
  /*                                                                       */
  /*    data    :: Application-provided request data.                      */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aface   :: A new @FT_Face handle.                                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The face requester should not perform funny things on the returned */
  /*    face object, like creating a new @FT_Size for it, or setting a     */
  /*    transformation through @FT_Set_Transform!                          */
  /*                                                                       */
  typedef FT_Error
  (*FTC_Face_Requester)( FTC_FaceID  face_id,
                         FT_Library  library,
                         FT_Pointer  request_data,
                         FT_Face*    aface );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_FontRec                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to describe a given `font' to the cache    */
  /*    manager.  Note that a `font' is the combination of a given face    */
  /*    with a given character size.                                       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    face_id    :: The ID of the face to use.                           */
  /*                                                                       */
  /*    pix_width  :: The character width in integer pixels.               */
  /*                                                                       */
  /*    pix_height :: The character height in integer pixels.              */
  /*                                                                       */
  typedef struct  FTC_FontRec_
  {
    FTC_FaceID  face_id;
    FT_UShort   pix_width;
    FT_UShort   pix_height;

  } FTC_FontRec;


  /* */


#define FTC_FONT_COMPARE( f1, f2 )                  \
          ( (f1)->face_id    == (f2)->face_id    && \
            (f1)->pix_width  == (f2)->pix_width  && \
            (f1)->pix_height == (f2)->pix_height )

#define FT_POINTER_TO_ULONG( p )  ((FT_ULong)(FT_Pointer)(p))

#define FTC_FACE_ID_HASH( i )                              \
          ((FT_UInt32)(( FT_POINTER_TO_ULONG( i ) >> 3 ) ^ \
                       ( FT_POINTER_TO_ULONG( i ) << 7 ) ) )

#define FTC_FONT_HASH( f )                              \
          (FT_UInt32)( FTC_FACE_ID_HASH((f)->face_id) ^ \
                       ((f)->pix_width << 8)          ^ \
                       ((f)->pix_height)              )


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_Font                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple handle to an @FTC_FontRec structure.                      */
  /*                                                                       */
  typedef FTC_FontRec*  FTC_Font;


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      CACHE MANAGER OBJECT                     *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_Manager                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This object is used to cache one or more @FT_Face objects, along   */
  /*    with corresponding @FT_Size objects.                               */
  /*                                                                       */
  typedef struct FTC_ManagerRec_*  FTC_Manager;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_Node                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An opaque handle to a cache node object.  Each cache node is       */
  /*    reference-counted.  A node with a count of 0 might be flushed      */
  /*    out of a full cache whenever a lookup request is performed.        */
  /*                                                                       */
  /*    If you lookup nodes, you have the ability to "acquire" them, i.e., */
  /*    to increment their reference count.  This will prevent the node    */
  /*    from being flushed out of the cache until you explicitly "release" */
  /*    it (see @FTC_Node_Unref).                                          */
  /*                                                                       */
  /*    See also @FTC_SBitCache_Lookup and @FTC_ImageCache_Lookup.         */
  /*                                                                       */
  typedef struct FTC_NodeRec_*  FTC_Node;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_New                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new cache manager.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library     :: The parent FreeType library handle to use.          */
  /*                                                                       */
  /*    max_bytes   :: Maximum number of bytes to use for cached data.     */
  /*                   Use 0 for defaults.                                 */
  /*                                                                       */
  /*    requester   :: An application-provided callback used to translate  */
  /*                   face IDs into real @FT_Face objects.                */
  /*                                                                       */
  /*    req_data    :: A generic pointer that is passed to the requester   */
  /*                   each time it is called (see @FTC_Face_Requester).   */
  /*                                                                       */
  /* <Output>                                                              */
  /*    amanager  :: A handle to a new manager object.  0 in case of       */
  /*                 failure.                                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_Manager_New( FT_Library          library,
                   FT_UInt             max_faces,
                   FT_UInt             max_sizes,
                   FT_ULong            max_bytes,
                   FTC_Face_Requester  requester,
                   FT_Pointer          req_data,
                   FTC_Manager        *amanager );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_Reset                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Empties a given cache manager.  This simply gets rid of all the    */
  /*    currently cached @FT_Face and @FT_Size objects within the manager. */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    manager :: A handle to the manager.                                */
  /*                                                                       */
  FT_EXPORT( void )
  FTC_Manager_Reset( FTC_Manager  manager );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_Done                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given manager after emptying it.                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: A handle to the target cache manager object.            */
  /*                                                                       */
  FT_EXPORT( void )
  FTC_Manager_Done( FTC_Manager  manager );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_LookupFace                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the @FT_Face object that corresponds to a given face ID  */
  /*    through a cache manager.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: A handle to the cache manager.                          */
  /*                                                                       */
  /*    face_id :: The ID of the face object.                              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aface   :: A handle to the face object.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The returned @FT_Face object is always owned by the manager.  You  */
  /*    should never try to discard it yourself.                           */
  /*                                                                       */
  /*    The @FT_Face object doesn't necessarily have a current size object */
  /*    (i.e., face->size can be 0).  If you need a specific `font size',  */
  /*    use @FTC_Manager_LookupSize instead.                               */
  /*                                                                       */
  /*    Never change the face's transformation matrix (i.e., never call    */
  /*    the @FT_Set_Transform function) on a returned face!  If you need   */
  /*    to transform glyphs, do it yourself after glyph loading.           */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_Manager_LookupFace( FTC_Manager  manager,
                          FTC_FaceID   face_id,
                          FT_Face     *aface );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_ScalerRec                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to describe a given character size in either      */
  /*    pixels or points to the cache manager.  See                        */
  /*    @FTC_Manager_LookupSize.                                           */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    face_id :: The source face ID.                                     */
  /*                                                                       */
  /*    width   :: The character width.                                    */
  /*                                                                       */
  /*    height  :: The character height.                                   */
  /*                                                                       */
  /*    pixel   :: A Boolean.  If TRUE, the `width' and `height' fields    */
  /*               are interpreted as integer pixel character sizes.       */
  /*               Otherwise, they are expressed as 1/64th of points.      */
  /*                                                                       */
  /*    x_res   :: Only used when `pixel' is FALSE to indicate the         */
  /*               horizontal resolution in dpi.                           */
  /*                                                                       */
  /*    y_res   :: Only used when `pixel' is FALSE to indicate the         */
  /*               vertical resolution in dpi.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This type is mainly used to retrieve @FT_Size objects through the  */
  /*    cache manager.                                                     */
  /*                                                                       */
  typedef struct  FTC_ScalerRec_
  {
    FTC_FaceID  face_id;
    FT_UInt     width;
    FT_UInt     height;
    FT_Int      pixel;
    FT_UInt     x_res;
    FT_UInt     y_res;

  } FTC_ScalerRec, *FTC_Scaler;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_LookupSize                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieve the @FT_Size object that corresponds to a given           */
  /*    @FTC_Scaler through a cache manager.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: A handle to the cache manager.                          */
  /*                                                                       */
  /*    scaler  :: A scaler handle.                                        */
  /*                                                                       */
  /* <Output>                                                              */
  /*    asize   :: A handle to the size object.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The returned @FT_Size object is always owned by the manager.  You  */
  /*    should never try to discard it by yourself.                        */
  /*                                                                       */
  /*    You can access the parent @FT_Face object simply as `size->face'   */
  /*    if you need it.  Note that this object is also owned by the        */
  /*    manager.                                                           */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_Manager_LookupSize( FTC_Manager  manager,
                          FTC_Scaler   scaler,
                          FT_Size     *asize );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Node_Unref                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Decrement a cache node's internal reference count.  When the count */
  /*    reaches 0, it is not destroyed but becomes eligible for subsequent */
  /*    cache flushes.                                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    node    :: The cache node handle.                                  */
  /*                                                                       */
  /*    manager :: The cache manager handle.                               */
  /*                                                                       */
  FT_EXPORT( void )
  FTC_Node_Unref( FTC_Node     node,
                  FTC_Manager  manager );


  /* remove all nodes belonging to a given face_id */
  FT_EXPORT( void )
  FTC_Manager_RemoveFaceID( FTC_Manager  manager,
                            FTC_FaceID   face_id );


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    cache_subsystem                                                    */
  /*                                                                       */
  /*************************************************************************/

  /************************************************************************
   *
   * @type:
   *    FTC_CMapCache
   *
   * @description:
   *    An opaque handle used to manager a charmap cache.  This cache is
   *    to hold character codes -> glyph indices mappings.
   */
  typedef struct FTC_CMapCacheRec_*  FTC_CMapCache;


  /*************************************************************************/
  /*                                                                       */
  /* @function:                                                            */
  /*    FTC_CMapCache_New                                                  */
  /*                                                                       */
  /* @description:                                                         */
  /*    Create a new charmap cache.                                        */
  /*                                                                       */
  /* @input:                                                               */
  /*    manager :: A handle to the cache manager.                          */
  /*                                                                       */
  /* @output:                                                              */
  /*    acache  :: A new cache handle.  NULL in case of error.             */
  /*                                                                       */
  /* @return:                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* @note:                                                                */
  /*    Like all other caches, this one will be destroyed with the cache   */
  /*    manager.                                                           */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_CMapCache_New( FTC_Manager     manager,
                     FTC_CMapCache  *acache );


  /*************************************************************************/
  /*                                                                       */
  /* @function:                                                            */
  /*    FTC_CMapCache_Lookup                                               */
  /*                                                                       */
  /* @description:                                                         */
  /*    Translate a character code into a glyph index, using the charmap   */
  /*    cache.                                                             */
  /*                                                                       */
  /* @input:                                                               */
  /*    cache      :: A charmap cache handle.                              */
  /*                                                                       */
  /*    face_id    :: The source face ID.                                  */
  /*                                                                       */
  /*    cmap_index :: The index of the charmap in the source face.         */
  /*                                                                       */
  /*    char_code  :: The character code (in the corresponding charmap).   */
  /*                                                                       */
  /* @return:                                                              */
  /*    Glyph index.  0 means `no glyph'.                                  */
  /*                                                                       */
  FT_EXPORT( FT_UInt )
  FTC_CMapCache_Lookup( FTC_CMapCache  cache,
                        FTC_FaceID     face_id,
                        FT_Int         cmap_index,
                        FT_UInt32      char_code );


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    cache_subsystem                                                    */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       IMAGE CACHE OBJECT                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  typedef struct  FTC_ImageTypeRec_
  {
    FTC_FaceID   face_id;
    FT_Int       width;
    FT_Int       height;
    FT_Int32     flags;

  } FTC_ImageTypeRec;

  typedef struct FTC_ImageTypeRec_*   FTC_ImageType;

 /* */

#define FTC_IMAGE_TYPE_COMPARE( d1, d2 )                    \
          ( FTC_FONT_COMPARE( &(d1)->font, &(d2)->font ) && \
            (d1)->flags == (d2)->flags                   )

#define FTC_IMAGE_TYPE_HASH( d )                    \
          (FT_UFast)( FTC_FONT_HASH( &(d)->font ) ^ \
                      ( (d)->flags << 4 )         )


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_ImageCache                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to an glyph image cache object.  They are designed to     */
  /*    hold many distinct glyph images while not exceeding a certain      */
  /*    memory threshold.                                                  */
  /*                                                                       */
  typedef struct FTC_ImageCacheRec_*  FTC_ImageCache;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_ImageCache_New                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new glyph image cache.                                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: The parent manager for the image cache.                 */
  /*                                                                       */
  /* <Output>                                                              */
  /*    acache  :: A handle to the new glyph image cache object.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_ImageCache_New( FTC_Manager      manager,
                      FTC_ImageCache  *acache );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_ImageCache_Lookup                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves a given glyph image from a glyph image cache.            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    cache  :: A handle to the source glyph image cache.                */
  /*                                                                       */
  /*    type   :: A pointer to a glyph image type descriptor.              */
  /*                                                                       */
  /*    gindex :: The glyph index to retrieve.                             */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aglyph :: The corresponding @FT_Glyph object.  0 in case of        */
  /*              failure.                                                 */
  /*                                                                       */
  /*    anode  :: Used to return the address of of the corresponding cache */
  /*              node after incrementing its reference count (see note    */
  /*              below).                                                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The returned glyph is owned and managed by the glyph image cache.  */
  /*    Never try to transform or discard it manually!  You can however    */
  /*    create a copy with @FT_Glyph_Copy and modify the new one.          */
  /*                                                                       */
  /*    If `anode' is _not_ NULL, it receives the address of the cache     */
  /*    node containing the glyph image, after increasing its reference    */
  /*    count.  This ensures that the node (as well as the FT_Glyph) will  */
  /*    always be kept in the cache until you call @FTC_Node_Unref to      */
  /*    `release' it.                                                      */
  /*                                                                       */
  /*    If `anode' is NULL, the cache node is left unchanged, which means  */
  /*    that the FT_Glyph could be flushed out of the cache on the next    */
  /*    call to one of the caching sub-system APIs.  Don't assume that it  */
  /*    is persistent!                                                     */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_ImageCache_Lookup( FTC_ImageCache  cache,
                         FTC_ImageType   type,
                         FT_UInt         gindex,
                         FT_Glyph       *aglyph,
                         FTC_Node       *anode );


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_SBit                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a small bitmap descriptor.  See the @FTC_SBitRec       */
  /*    structure for details.                                             */
  /*                                                                       */
  typedef struct FTC_SBitRec_*  FTC_SBit;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_SBitRec                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very compact structure used to describe a small glyph bitmap.    */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    width     :: The bitmap width in pixels.                           */
  /*                                                                       */
  /*    height    :: The bitmap height in pixels.                          */
  /*                                                                       */
  /*    left      :: The horizontal distance from the pen position to the  */
  /*                 left bitmap border (a.k.a. `left side bearing', or    */
  /*                 `lsb').                                               */
  /*                                                                       */
  /*    top       :: The vertical distance from the pen position (on the   */
  /*                 baseline) to the upper bitmap border (a.k.a. `top     */
  /*                 side bearing').  The distance is positive for upwards */
  /*                 Y coordinates.                                        */
  /*                                                                       */
  /*    format    :: The format of the glyph bitmap (monochrome or gray).  */
  /*                                                                       */
  /*    max_grays :: Maximum gray level value (in the range 1 to 255).     */
  /*                                                                       */
  /*    pitch     :: The number of bytes per bitmap line.  May be positive */
  /*                 or negative.                                          */
  /*                                                                       */
  /*    xadvance  :: The horizontal advance width in pixels.               */
  /*                                                                       */
  /*    yadvance  :: The vertical advance height in pixels.                */
  /*                                                                       */
  /*    buffer    :: A pointer to the bitmap pixels.                       */
  /*                                                                       */
  typedef struct  FTC_SBitRec_
  {
    FT_Byte   width;
    FT_Byte   height;
    FT_Char   left;
    FT_Char   top;

    FT_Byte   format;
    FT_Byte   max_grays;
    FT_Short  pitch;
    FT_Char   xadvance;
    FT_Char   yadvance;

    FT_Byte*  buffer;

  } FTC_SBitRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_SBitCache                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a small bitmap cache.  These are special cache objects */
  /*    used to store small glyph bitmaps (and anti-aliased pixmaps) in a  */
  /*    much more efficient way than the traditional glyph image cache     */
  /*    implemented by @FTC_ImageCache.                                    */
  /*                                                                       */
  typedef struct FTC_SBitCacheRec_*  FTC_SBitCache;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_SBitCache_New                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new cache to store small glyph bitmaps.                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: A handle to the source cache manager.                   */
  /*                                                                       */
  /* <Output>                                                              */
  /*    acache  :: A handle to the new sbit cache.  NULL in case of error. */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_SBitCache_New( FTC_Manager     manager,
                     FTC_SBitCache  *acache );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_SBitCache_Lookup                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Looks up a given small glyph bitmap in a given sbit cache and      */
  /*    `lock' it to prevent its flushing from the cache until needed.     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    cache  :: A handle to the source sbit cache.                       */
  /*                                                                       */
  /*    type   :: A pointer to the glyph image type descriptor.            */
  /*                                                                       */
  /*    gindex :: The glyph index.                                         */
  /*                                                                       */
  /* <Output>                                                              */
  /*    sbit   :: A handle to a small bitmap descriptor.                   */
  /*                                                                       */
  /*    anode  :: Used to return the address of of the corresponding cache */
  /*              node after incrementing its reference count (see note    */
  /*              below).                                                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The small bitmap descriptor and its bit buffer are owned by the    */
  /*    cache and should never be freed by the application.  They might    */
  /*    as well disappear from memory on the next cache lookup, so don't   */
  /*    treat them as persistent data.                                     */
  /*                                                                       */
  /*    The descriptor's `buffer' field is set to 0 to indicate a missing  */
  /*    glyph bitmap.                                                      */
  /*                                                                       */
  /*    If `anode' is _not_ NULL, it receives the address of the cache     */
  /*    node containing the bitmap, after increasing its reference count.  */
  /*    This ensures that the node (as well as the image) will always be   */
  /*    kept in the cache until you call @FTC_Node_Unref to `release' it.  */
  /*                                                                       */
  /*    If `anode' is NULL, the cache node is left unchanged, which means  */
  /*    that the bitmap could be flushed out of the cache on the next      */
  /*    call to one of the caching sub-system APIs.  Don't assume that it  */
  /*    is persistent!                                                     */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_SBitCache_Lookup( FTC_SBitCache    cache,
                        FTC_ImageType    type,
                        FT_UInt          gindex,
                        FTC_SBit        *sbit,
                        FTC_Node        *anode );


 /* */

FT_END_HEADER

#endif /* __FTCACHE_H__ */


/* END */
