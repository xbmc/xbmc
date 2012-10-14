/*
 * Copyright 2009-2012 10gen, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Portions Copyright 2001 Unicode, Inc.
 *
 * Disclaimer
 *
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 *
 * Limitations on Rights to Redistribute This Code
 *
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */


#include "bson.h"
#include "encoding.h"

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 */
static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/* --------------------------------------------------------------------- */

/*
 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
 * This must be called with the length pre-determined by the first byte.
 * The length can be set by:
 *  length = trailingBytesForUTF8[*source]+1;
 * and the sequence is illegal right away if there aren't that many bytes
 * available.
 * If presented with a length > 4, this returns 0.  The Unicode
 * definition of UTF-8 goes up to 4-byte sequences.
 */
static int isLegalUTF8( const unsigned char *source, int length ) {
    unsigned char a;
    const unsigned char *srcptr = source + length;
    switch ( length ) {
    default:
        return 0;
        /* Everything else falls through when "true"... */
    case 4:
        if ( ( a = ( *--srcptr ) ) < 0x80 || a > 0xBF ) return 0;
    case 3:
        if ( ( a = ( *--srcptr ) ) < 0x80 || a > 0xBF ) return 0;
    case 2:
        if ( ( a = ( *--srcptr ) ) > 0xBF ) return 0;
        switch ( *source ) {
            /* no fall-through in this inner switch */
        case 0xE0:
            if ( a < 0xA0 ) return 0;
            break;
        case 0xF0:
            if ( a < 0x90 ) return 0;
            break;
        case 0xF4:
            if ( a > 0x8F ) return 0;
            break;
        default:
            if ( a < 0x80 ) return 0;
        }
    case 1:
        if ( *source >= 0x80 && *source < 0xC2 ) return 0;
        if ( *source > 0xF4 ) return 0;
    }
    return 1;
}

/* If the name is part of a db ref ($ref, $db, or $id), then return true. */
static int bson_string_is_db_ref( const unsigned char *string, const int length ) {
    int result = 0;

    if( length >= 4 ) {
      if( string[1] == 'r' && string[2] == 'e' && string[3] == 'f' )
        result = 1;
    }
    else if( length >= 3 ) {
      if( string[1] == 'i' && string[2] == 'd' )
        result = 1;
      else if( string[1] == 'd' && string[2] == 'b' )
        result = 1;
    }

   return result;
}

static int bson_validate_string( bson *b, const unsigned char *string,
                                 const int length, const char check_utf8, const char check_dot,
                                 const char check_dollar ) {

    int position = 0;
    int sequence_length = 1;

    if( check_dollar && string[0] == '$' ) {
        if( !bson_string_is_db_ref( string, length ) )
            b->err |= BSON_FIELD_INIT_DOLLAR;
    }

    while ( position < length ) {
        if ( check_dot && *( string + position ) == '.' ) {
            b->err |= BSON_FIELD_HAS_DOT;
        }

        if ( check_utf8 ) {
            sequence_length = trailingBytesForUTF8[*( string + position )] + 1;
            if ( ( position + sequence_length ) > length ) {
                b->err |= BSON_NOT_UTF8;
                return BSON_ERROR;
            }
            if ( !isLegalUTF8( string + position, sequence_length ) ) {
                b->err |= BSON_NOT_UTF8;
                return BSON_ERROR;
            }
        }
        position += sequence_length;
    }

    return BSON_OK;
}


int bson_check_string( bson *b, const char *string,
                       const int length ) {

    return bson_validate_string( b, ( const unsigned char * )string, length, 1, 0, 0 );
}

int bson_check_field_name( bson *b, const char *string,
                           const int length ) {

    return bson_validate_string( b, ( const unsigned char * )string, length, 1, 1, 1 );
}
