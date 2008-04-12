/***************************************************************************/
/*                                                                         */
/*  psmodule.c                                                             */
/*                                                                         */
/*    PSNames module implementation (body).                                */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2005, 2006 by                         */
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
#include FT_INTERNAL_OBJECTS_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H

#include "psmodule.h"
#include "pstables.h"

#include "psnamerr.h"


#ifndef FT_CONFIG_OPTION_NO_POSTSCRIPT_NAMES


#ifdef FT_CONFIG_OPTION_ADOBE_GLYPH_LIST


#define VARIANT_BIT         ( 1L << 31 )
#define BASE_GLYPH( code )  ( (code) & ~VARIANT_BIT )


  /* Return the Unicode value corresponding to a given glyph.  Note that */
  /* we do deal with glyph variants by detecting a non-initial dot in    */
  /* the name, as in `A.swash' or `e.final'; in this case, the           */
  /* VARIANT_BIT is set in the return value.                             */
  /*                                                                     */
  static FT_UInt32
  ps_unicode_value( const char*  glyph_name )
  {
    /* If the name begins with `uni', then the glyph name may be a */
    /* hard-coded unicode character code.                          */
    if ( glyph_name[0] == 'u' &&
         glyph_name[1] == 'n' &&
         glyph_name[2] == 'i' )
    {
      /* determine whether the next four characters following are */
      /* hexadecimal.                                             */

      /* XXX: Add code to deal with ligatures, i.e. glyph names like */
      /*      `uniXXXXYYYYZZZZ'...                                   */

      FT_Int       count;
      FT_ULong     value = 0;
      const char*  p     = glyph_name + 3;


      for ( count = 4; count > 0; count--, p++ )
      {
        char          c = *p;
        unsigned int  d;


        d = (unsigned char)c - '0';
        if ( d >= 10 )
        {
          d = (unsigned char)c - 'A';
          if ( d >= 6 )
            d = 16;
          else
            d += 10;
        }

        /* Exit if a non-uppercase hexadecimal character was found   */
        /* -- this also catches character codes below `0' since such */
        /* negative numbers cast to `unsigned int' are far too big.  */
        if ( d >= 16 )
          break;

        value = ( value << 4 ) + d;
      }

      /* there must be exactly four hex digits */
      if ( count == 0 )
      {
        if ( *p == '\0' )
          return value;
        if ( *p == '.' )
          return value ^ VARIANT_BIT;
      }
    }

    /* If the name begins with `u', followed by four to six uppercase */
    /* hexadicimal digits, it is a hard-coded unicode character code. */
    if ( glyph_name[0] == 'u' )
    {
      FT_Int       count;
      FT_ULong     value = 0;
      const char*  p     = glyph_name + 1;


      for ( count = 6; count > 0; count--, p++ )
      {
        char          c = *p;
        unsigned int  d;


        d = (unsigned char)c - '0';
        if ( d >= 10 )
        {
          d = (unsigned char)c - 'A';
          if ( d >= 6 )
            d = 16;
          else
            d += 10;
        }

        if ( d >= 16 )
          break;

        value = ( value << 4 ) + d;
      }

      if ( count <= 2 )
      {
        if ( *p == '\0' )
          return value;
        if ( *p == '.' )
          return value ^ VARIANT_BIT;
      }
    }

    /* Look for a non-initial dot in the glyph name in order to */
    /* find variants like `A.swash', `e.final', etc.            */
    {
      const char*  p   = glyph_name;
      const char*  dot = NULL;


      for ( ; *p; p++ )
      {
        if ( *p == '.' && p > glyph_name )
        {
          dot = p;
          break;
        }
      }

      /* now look up the glyph in the Adobe Glyph List */
      if ( !dot )
        return ft_get_adobe_glyph_index( glyph_name, p );
      else
        return ft_get_adobe_glyph_index( glyph_name, dot ) ^ VARIANT_BIT;
    }
  }


  /* ft_qsort callback to sort the unicode map */
  FT_CALLBACK_DEF( int )
  compare_uni_maps( const void*  a,
                    const void*  b )
  {
    PS_UniMap*  map1 = (PS_UniMap*)a;
    PS_UniMap*  map2 = (PS_UniMap*)b;
    FT_UInt32   unicode1 = BASE_GLYPH( map1->unicode );
    FT_UInt32   unicode2 = BASE_GLYPH( map2->unicode );


    /* sort base glyphs before glyph variants */
    if ( unicode1 == unicode2 )
      return map1->unicode - map2->unicode;
    else
      return unicode1 - unicode2;
  }


  /* Build a table that maps Unicode values to glyph indices. */
  static FT_Error
  ps_unicodes_init( FT_Memory          memory,
                    PS_Unicodes        table,
                    FT_UInt            num_glyphs,
                    PS_Glyph_NameFunc  get_glyph_name,
                    FT_Pointer         glyph_data )
  {
    FT_Error  error;


    /* we first allocate the table */
    table->num_maps = 0;
    table->maps     = 0;

    if ( !FT_NEW_ARRAY( table->maps, num_glyphs ) )
    {
      FT_UInt     n;
      FT_UInt     count;
      PS_UniMap*  map;
      FT_UInt32   uni_char;


      map = table->maps;

      for ( n = 0; n < num_glyphs; n++ )
      {
        const char*  gname = get_glyph_name( glyph_data, n );


        if ( gname )
        {
          uni_char = ps_unicode_value( gname );

          if ( BASE_GLYPH( uni_char ) != 0 )
          {
            map->unicode     = uni_char;
            map->glyph_index = n;
            map++;
          }
        }
      }

      /* now compress the table a bit */
      count = (FT_UInt)( map - table->maps );

      if ( count == 0 )
      {
        FT_FREE( table->maps );
        if ( !error )
          error = PSnames_Err_Invalid_Argument;  /* No unicode chars here! */
      }
      else {
        /* Reallocate if the number of used entries is much smaller. */
        if ( count < num_glyphs / 2 )
        {
          (void)FT_RENEW_ARRAY( table->maps, num_glyphs, count );
          error = PSnames_Err_Ok;
        }

        /* Sort the table in increasing order of unicode values, */
        /* taking care of glyph variants.                        */
        ft_qsort( table->maps, count, sizeof ( PS_UniMap ),
                  compare_uni_maps );
      }

      table->num_maps = count;
    }

    return error;
  }


  static FT_UInt
  ps_unicodes_char_index( PS_Unicodes  table,
                          FT_UInt32    unicode )
  {
    PS_UniMap  *min, *max, *mid, *result = NULL;


    /* Perform a binary search on the table. */

    min = table->maps;
    max = min + table->num_maps - 1;

    while ( min <= max )
    {
      FT_UInt32  base_glyph;


      mid = min + ( ( max - min ) >> 1 );

      if ( mid->unicode == unicode )
      {
        result = mid;
        break;
      }

      base_glyph = BASE_GLYPH( mid->unicode );

      if ( base_glyph == unicode )
        result = mid; /* remember match but continue search for base glyph */

      if ( min == max )
        break;

      if ( base_glyph < unicode )
        min = mid + 1;
      else
        max = mid - 1;
    }

    if ( result )
      return result->glyph_index;
    else
      return 0;
  }


  static FT_ULong
  ps_unicodes_char_next( PS_Unicodes  table,
                         FT_UInt32   *unicode )
  {
    FT_UInt    result    = 0;
    FT_UInt32  char_code = *unicode + 1;


    {
      FT_UInt     min = 0;
      FT_UInt     max = table->num_maps;
      FT_UInt     mid;
      PS_UniMap*  map;
      FT_UInt32   base_glyph;


      while ( min < max )
      {
        mid = min + ( ( max - min ) >> 1 );
        map = table->maps + mid;

        if ( map->unicode == char_code )
        {
          result = map->glyph_index;
          goto Exit;
        }

        base_glyph = BASE_GLYPH( map->unicode );

        if ( base_glyph == char_code )
          result = map->glyph_index;

        if ( base_glyph < char_code )
          min = mid + 1;
        else
          max = mid;
      }

      if ( result )
        goto Exit;               /* we have a variant glyph */

      /* we didn't find it; check whether we have a map just above it */
      char_code = 0;

      if ( min < table->num_maps )
      {
        map       = table->maps + min;
        result    = map->glyph_index;
        char_code = BASE_GLYPH( map->unicode );
      }
    }

  Exit:
    *unicode = char_code;
    return result;
  }


#endif /* FT_CONFIG_OPTION_ADOBE_GLYPH_LIST */


  static const char*
  ps_get_macintosh_name( FT_UInt  name_index )
  {
    if ( name_index >= FT_NUM_MAC_NAMES )
      name_index = 0;

    return ft_standard_glyph_names + ft_mac_names[name_index];
  }


  static const char*
  ps_get_standard_strings( FT_UInt  sid )
  {
    if ( sid >= FT_NUM_SID_NAMES )
      return 0;

    return ft_standard_glyph_names + ft_sid_names[sid];
  }


  static
  const FT_Service_PsCMapsRec  pscmaps_interface =
  {
#ifdef FT_CONFIG_OPTION_ADOBE_GLYPH_LIST

    (PS_Unicode_ValueFunc)     ps_unicode_value,
    (PS_Unicodes_InitFunc)     ps_unicodes_init,
    (PS_Unicodes_CharIndexFunc)ps_unicodes_char_index,
    (PS_Unicodes_CharNextFunc) ps_unicodes_char_next,

#else

    0,
    0,
    0,
    0,

#endif /* FT_CONFIG_OPTION_ADOBE_GLYPH_LIST */

    (PS_Macintosh_NameFunc)    ps_get_macintosh_name,
    (PS_Adobe_Std_StringsFunc) ps_get_standard_strings,

    t1_standard_encoding,
    t1_expert_encoding
  };


  static const FT_ServiceDescRec  pscmaps_services[] =
  {
    { FT_SERVICE_ID_POSTSCRIPT_CMAPS, &pscmaps_interface },
    { NULL, NULL }
  };


  static FT_Pointer
  psnames_get_service( FT_Module    module,
                       const char*  service_id )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( pscmaps_services, service_id );
  }

#endif /* !FT_CONFIG_OPTION_NO_POSTSCRIPT_NAMES */



  FT_CALLBACK_TABLE_DEF
  const FT_Module_Class  psnames_module_class =
  {
    0,  /* this is not a font driver, nor a renderer */
    sizeof ( FT_ModuleRec ),

    "psnames",  /* driver name                         */
    0x10000L,   /* driver version                      */
    0x20000L,   /* driver requires FreeType 2 or above */

#ifdef FT_CONFIG_OPTION_NO_POSTSCRIPT_NAMES
    0,
    (FT_Module_Constructor)0,
    (FT_Module_Destructor) 0,
    (FT_Module_Requester)  0
#else
    (void*)&pscmaps_interface,   /* module specific interface */
    (FT_Module_Constructor)0,
    (FT_Module_Destructor) 0,
    (FT_Module_Requester)  psnames_get_service
#endif
  };


/* END */
