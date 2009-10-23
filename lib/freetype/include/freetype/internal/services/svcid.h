/***************************************************************************/
/*                                                                         */
/*  svcid.h                                                                */
/*                                                                         */
/*    The FreeType CID font services (specification).                      */
/*                                                                         */
/*  Copyright 2007, 2009 by Derek Clegg, Michael Toftdal.                  */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __SVCID_H__
#define __SVCID_H__

#include FT_INTERNAL_SERVICE_H


FT_BEGIN_HEADER


#define FT_SERVICE_ID_CID  "CID"

  typedef FT_Error
  (*FT_CID_GetRegistryOrderingSupplementFunc)( FT_Face       face,
                                               const char*  *registry,
                                               const char*  *ordering,
                                               FT_Int       *supplement );
  typedef FT_Error
  (*FT_CID_GetIsInternallyCIDKeyedFunc)( FT_Face   face,
                                         FT_Bool  *is_cid );
  typedef FT_Error
  (*FT_CID_GetCIDFromGlyphIndexFunc)( FT_Face   face,
                                      FT_UInt   glyph_index,
                                      FT_UInt  *cid );

  FT_DEFINE_SERVICE( CID )
  {
    FT_CID_GetRegistryOrderingSupplementFunc  get_ros;
    FT_CID_GetIsInternallyCIDKeyedFunc        get_is_cid;
    FT_CID_GetCIDFromGlyphIndexFunc           get_cid_from_glyph_index;
  };

  /* */


FT_END_HEADER


#endif /* __SVCID_H__ */


/* END */
