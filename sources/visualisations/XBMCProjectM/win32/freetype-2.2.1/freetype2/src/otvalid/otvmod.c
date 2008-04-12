/***************************************************************************/
/*                                                                         */
/*  otvmod.c                                                               */
/*                                                                         */
/*    FreeType's OpenType validation module implementation (body).         */
/*                                                                         */
/*  Copyright 2004, 2005 by                                                */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H
#include FT_OPENTYPE_VALIDATE_H
#include FT_INTERNAL_OBJECTS_H
#include FT_SERVICE_OPENTYPE_VALIDATE_H

#include "otvmod.h"
#include "otvalid.h"
#include "otvcommn.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_otvmodule


  static FT_Error
  otv_load_table( FT_Face    face,
                  FT_Tag     tag,
                  FT_Byte*  *table,
                  FT_ULong  *table_len )
  {
    FT_Error   error;
    FT_Memory  memory = FT_FACE_MEMORY( face );


    error = FT_Load_Sfnt_Table( face, tag, 0, NULL, table_len );
    if ( error == OTV_Err_Table_Missing )
      return OTV_Err_Ok;
    if ( error )
      goto Exit;

    if ( FT_ALLOC( *table, *table_len ) )
      goto Exit;

    error = FT_Load_Sfnt_Table( face, tag, 0, *table, table_len );

  Exit:
    return error;
  }


  static FT_Error
  otv_validate( FT_Face    face,
                FT_UInt    ot_flags,
                FT_Bytes  *ot_base,
                FT_Bytes  *ot_gdef,
                FT_Bytes  *ot_gpos,
                FT_Bytes  *ot_gsub,
                FT_Bytes  *ot_jstf )
  {
    FT_Error         error = OTV_Err_Ok;
    FT_Byte          *base, *gdef, *gpos, *gsub, *jstf;
    FT_ULong         len_base, len_gdef, len_gpos, len_gsub, len_jstf;
    FT_ValidatorRec  valid;


    base     = gdef     = gpos     = gsub     = jstf     = NULL;
    len_base = len_gdef = len_gpos = len_gsub = len_jstf = 0;

    /* load tables */

    if ( ot_flags & FT_VALIDATE_BASE )
    {
      error = otv_load_table( face, TTAG_BASE, &base, &len_base );
      if ( error )
        goto Exit;
    }

    if ( ot_flags & FT_VALIDATE_GDEF )
    {
      error = otv_load_table( face, TTAG_GDEF, &gdef, &len_gdef );
      if ( error )
        goto Exit;
    }

    if ( ot_flags & FT_VALIDATE_GPOS )
    {
      error = otv_load_table( face, TTAG_GPOS, &gpos, &len_gpos );
      if ( error )
        goto Exit;
    }

    if ( ot_flags & FT_VALIDATE_GSUB )
    {
      error = otv_load_table( face, TTAG_GSUB, &gsub, &len_gsub );
      if ( error )
        goto Exit;
    }

    if ( ot_flags & FT_VALIDATE_JSTF )
    {
      error = otv_load_table( face, TTAG_JSTF, &jstf, &len_jstf );
      if ( error )
        goto Exit;
    }

    /* validate tables */

    if ( base )
    {
      ft_validator_init( &valid, base, base + len_base, FT_VALIDATE_DEFAULT );
      if ( ft_validator_run( &valid ) == 0 )
        otv_BASE_validate( base, &valid );
      error = valid.error;
      if ( error )
        goto Exit;
    }

    if ( gpos )
    {
      ft_validator_init( &valid, gpos, gpos + len_gpos, FT_VALIDATE_DEFAULT );
      if ( ft_validator_run( &valid ) == 0 )
        otv_GPOS_validate( gpos, face->num_glyphs, &valid );
      error = valid.error;
      if ( error )
        goto Exit;
    }

    if ( gsub )
    {
      ft_validator_init( &valid, gsub, gsub + len_gsub, FT_VALIDATE_DEFAULT );
      if ( ft_validator_run( &valid ) == 0 )
        otv_GSUB_validate( gsub, face->num_glyphs, &valid );
      error = valid.error;
      if ( error )
        goto Exit;
    }

    if ( gdef )
    {
      ft_validator_init( &valid, gdef, gdef + len_gdef, FT_VALIDATE_DEFAULT );
      if ( ft_validator_run( &valid ) == 0 )
        otv_GDEF_validate( gdef, gsub, gpos, &valid );
      error = valid.error;
      if ( error )
        goto Exit;
    }

    if ( jstf )
    {
      ft_validator_init( &valid, jstf, jstf + len_jstf, FT_VALIDATE_DEFAULT );
      if ( ft_validator_run( &valid ) == 0 )
        otv_JSTF_validate( jstf, gsub, gpos, face->num_glyphs, &valid );
      error = valid.error;
      if ( error )
        goto Exit;
    }

    *ot_base = (FT_Bytes)base;
    *ot_gdef = (FT_Bytes)gdef;
    *ot_gpos = (FT_Bytes)gpos;
    *ot_gsub = (FT_Bytes)gsub;
    *ot_jstf = (FT_Bytes)jstf;

  Exit:
    if ( error ) {
      FT_Memory  memory = FT_FACE_MEMORY( face );


      FT_FREE( base );
      FT_FREE( gdef );
      FT_FREE( gpos );
      FT_FREE( gsub );
      FT_FREE( jstf );
    }

    return error;
  }


  static
  const FT_Service_OTvalidateRec  otvalid_interface = 
  {
    otv_validate
  };


  static
  const FT_ServiceDescRec  otvalid_services[] = 
  {
    { FT_SERVICE_ID_OPENTYPE_VALIDATE, &otvalid_interface },
    { NULL, NULL }
  };


  static FT_Pointer
  otvalid_get_service( FT_Module    module,
                       const char*  service_id )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( otvalid_services, service_id );
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Module_Class  otv_module_class =
  {
    0,
    sizeof( FT_ModuleRec ),
    "otvalid",
    0x10000L,
    0x20000L,

    0,              /* module-specific interface */

    (FT_Module_Constructor)0,
    (FT_Module_Destructor) 0,
    (FT_Module_Requester)  otvalid_get_service
  };


/* END */
