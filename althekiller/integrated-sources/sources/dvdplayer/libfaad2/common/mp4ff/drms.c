/*****************************************************************************
 * drms.c : DRMS
 *****************************************************************************
 * Copyright (C) 2004 VideoLAN
 * $Id: drms.c,v 1.3 2004/01/11 15:52:18 menno Exp $
 *
 * Author: Jon Lech Johansen <jon-vl@nanocrew.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#include <stdlib.h>                                      /* malloc(), free() */

#include "mp4ffint.h"

#ifdef ITUNES_DRM

#ifdef _WIN32
#include <tchar.h>
#include <shlobj.h>
#include <windows.h>
#endif

#include "drms.h"
#include "drmstables.h"

static __inline uint32_t U32_AT( void * _p )
{
    uint8_t * p = (uint8_t *)_p;
    return ( ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
              | ((uint32_t)p[2] << 8) | p[3] );
}

#define TAOS_INIT( tmp, i ) \
    memset( tmp, 0, sizeof(tmp) ); \
    tmp[ i + 0 ] = 0x67452301; \
    tmp[ i + 1 ] = 0xEFCDAB89; \
    tmp[ i + 2 ] = 0x98BADCFE; \
    tmp[ i + 3 ] = 0x10325476;

#define ROR( x, n ) (((x) << (32-(n))) | ((x) >> (n)))

static void init_ctx( uint32_t *p_ctx, uint32_t *p_input )
{
    uint32_t i;
    uint32_t p_tmp[ 6 ];

    p_ctx[ 0 ] = sizeof(*p_input);

    memset( &p_ctx[ 1 + 4 ], 0, sizeof(*p_input) * 4 );
    memcpy( &p_ctx[ 1 + 0 ], p_input, sizeof(*p_input) * 4 );

    p_tmp[ 0 ] = p_ctx[ 1 + 3 ];

    for( i = 0; i < sizeof(p_drms_tab1)/sizeof(p_drms_tab1[ 0 ]); i++ )
    {
        p_tmp[ 0 ] = ROR( p_tmp[ 0 ], 8 );

        p_tmp[ 5 ] = p_drms_tab2[ (p_tmp[ 0 ] >> 24) & 0xFF ]
                   ^ ROR( p_drms_tab2[ (p_tmp[ 0 ] >> 16) & 0xFF ], 8 )
                   ^ ROR( p_drms_tab2[ (p_tmp[ 0 ] >> 8) & 0xFF ], 16 )
                   ^ ROR( p_drms_tab2[ p_tmp[ 0 ] & 0xFF ], 24 )
                   ^ p_drms_tab1[ i ]
                   ^ p_ctx[ 1 + ((i + 1) * 4) - 4 ];

        p_ctx[ 1 + ((i + 1) * 4) + 0 ] = p_tmp[ 5 ];
        p_tmp[ 5 ] ^= p_ctx[ 1 + ((i + 1) * 4) - 3 ];
        p_ctx[ 1 + ((i + 1) * 4) + 1 ] = p_tmp[ 5 ];
        p_tmp[ 5 ] ^= p_ctx[ 1 + ((i + 1) * 4) - 2 ];
        p_ctx[ 1 + ((i + 1) * 4) + 2 ] = p_tmp[ 5 ];
        p_tmp[ 5 ] ^= p_ctx[ 1 + ((i + 1) * 4) - 1 ];
        p_ctx[ 1 + ((i + 1) * 4) + 3 ] = p_tmp[ 5 ];

        p_tmp[ 0 ] = p_tmp[ 5 ];
    }

    memcpy( &p_ctx[ 1 + 64 ], &p_ctx[ 1 ], sizeof(*p_ctx) * 4 );

    for( i = 4; i < sizeof(p_drms_tab1); i++ )
    {
        p_tmp[ 2 ] = p_ctx[ 1 + 4 + (i - 4) ];

        p_tmp[ 0 ] = (((p_tmp[ 2 ] >> 7) & 0x01010101) * 27)
                   ^ ((p_tmp[ 2 ] & 0xFF7F7F7F) << 1);
        p_tmp[ 1 ] = (((p_tmp[ 0 ] >> 7) & 0x01010101) * 27)
                   ^ ((p_tmp[ 0 ] & 0xFF7F7F7F) << 1);
        p_tmp[ 4 ] = (((p_tmp[ 1 ] >> 7) & 0x01010101) * 27)
                   ^ ((p_tmp[ 1 ] & 0xFF7F7F7F) << 1);

        p_tmp[ 2 ] ^= p_tmp[ 4 ];

        p_tmp[ 3 ] = ROR( p_tmp[ 1 ] ^ p_tmp[ 2 ], 16 )
                   ^ ROR( p_tmp[ 0 ] ^ p_tmp[ 2 ], 8 )
                   ^ ROR( p_tmp[ 2 ], 24 );

        p_ctx[ 1 + 4 + 64 + (i - 4) ] = p_tmp[ 3 ] ^ p_tmp[ 4 ]
                                      ^ p_tmp[ 1 ] ^ p_tmp[ 0 ];
    }
}

static void ctx_xor( uint32_t *p_ctx, uint32_t *p_in, uint32_t *p_out,
                     uint32_t p_table1[ 256 ], uint32_t p_table2[ 256 ] )
{
    uint32_t i, x, y;
    uint32_t p_tmp1[ 4 ];
    uint32_t p_tmp2[ 4 ];

    i = p_ctx[ 0 ] * 4;

    p_tmp1[ 0 ] = p_ctx[ 1 + i + 24 ] ^ p_in[ 0 ];
    p_tmp1[ 1 ] = p_ctx[ 1 + i + 25 ] ^ p_in[ 1 ];
    p_tmp1[ 2 ] = p_ctx[ 1 + i + 26 ] ^ p_in[ 2 ];
    p_tmp1[ 3 ] = p_ctx[ 1 + i + 27 ] ^ p_in[ 3 ];

    i += 84;

#define XOR_ROR( p_table, p_tmp, i_ctx ) \
    p_table[ (p_tmp[ y > 2 ? y - 3 : y + 1 ] >> 24) & 0xFF ] \
    ^ ROR( p_table[ (p_tmp[ y > 1 ? y - 2 : y + 2 ] >> 16) & 0xFF ], 8 ) \
    ^ ROR( p_table[ (p_tmp[ y > 0 ? y - 1 : y + 3 ] >> 8) & 0xFF ], 16 ) \
    ^ ROR( p_table[ p_tmp[ y ] & 0xFF ], 24 ) \
    ^ p_ctx[ i_ctx ]

    for( x = 0; x < 1; x++ )
    {
        memcpy( p_tmp2, p_tmp1, sizeof(p_tmp1) );

        for( y = 0; y < 4; y++ )
        {
            p_tmp1[ y ] = XOR_ROR( p_table1, p_tmp2, 1 + i - x + y );
        }
    }

    for( ; x < 9; x++ )
    {
        memcpy( p_tmp2, p_tmp1, sizeof(p_tmp1) );

        for( y = 0; y < 4; y++ )
        {
            p_tmp1[ y ] = XOR_ROR( p_table1, p_tmp2,
                                   1 + i - x - ((x * 3) - y) );
        }
    }

    for( y = 0; y < 4; y++ )
    {
        p_out[ y ] = XOR_ROR( p_table2, p_tmp1,
                              1 + i - x - ((x * 3) - y) );
    }

#undef XOR_ROR
}

static void taos( uint32_t *p_buffer, uint32_t *p_input )
{
    uint32_t i;
    uint32_t x = 0;
    uint32_t p_tmp1[ 4 ];
    uint32_t p_tmp2[ 4 ];

    memcpy( p_tmp1, p_buffer, sizeof(p_tmp1) );

    p_tmp2[ 0 ] = ((~p_tmp1[ 1 ] & p_tmp1[ 3 ])
                |   (p_tmp1[ 2 ] & p_tmp1[ 1 ])) + p_input[ x ];
    p_tmp1[ 0 ] = p_tmp2[ 0 ] + p_tmp1[ 0 ] + p_drms_tab_taos[ x++ ];

    for( i = 0; i < 4; i++ )
    {
        p_tmp2[ 0 ] = ((p_tmp1[ 0 ] >> 0x19)
                    |  (p_tmp1[ 0 ] << 0x7)) + p_tmp1[ 1 ];
        p_tmp2[ 1 ] = ((~p_tmp2[ 0 ] & p_tmp1[ 2 ])
                    |   (p_tmp1[ 1 ] & p_tmp2[ 0 ])) + p_input[ x ];
        p_tmp2[ 1 ] += p_tmp1[ 3 ] + p_drms_tab_taos[ x++ ];

        p_tmp1[ 3 ] = ((p_tmp2[ 1 ] >> 0x14)
                    |  (p_tmp2[ 1 ] << 0xC)) + p_tmp2[ 0 ];
        p_tmp2[ 1 ] = ((~p_tmp1[ 3 ] & p_tmp1[ 1 ])
                    |   (p_tmp1[ 3 ] & p_tmp2[ 0 ])) + p_input[ x ];
        p_tmp2[ 1 ] += p_tmp1[ 2 ] + p_drms_tab_taos[ x++ ];

        p_tmp1[ 2 ] = ((p_tmp2[ 1 ] >> 0xF)
                    |  (p_tmp2[ 1 ] << 0x11)) + p_tmp1[ 3 ];
        p_tmp2[ 1 ] = ((~p_tmp1[ 2 ] & p_tmp2[ 0 ])
                    |   (p_tmp1[ 3 ] & p_tmp1[ 2 ])) + p_input[ x ];
        p_tmp2[ 2 ] = p_tmp2[ 1 ] + p_tmp1[ 1 ] + p_drms_tab_taos[ x++ ];

        p_tmp1[ 1 ] = ((p_tmp2[ 2 ] << 0x16)
                    |  (p_tmp2[ 2 ] >> 0xA)) + p_tmp1[ 2 ];
        if( i == 3 )
        {
            p_tmp2[ 1 ] = ((~p_tmp1[ 3 ] & p_tmp1[ 2 ])
                        |   (p_tmp1[ 3 ] & p_tmp1[ 1 ])) + p_input[ 1 ];
        }
        else
        {
            p_tmp2[ 1 ] = ((~p_tmp1[ 1 ] & p_tmp1[ 3 ])
                        |   (p_tmp1[ 2 ] & p_tmp1[ 1 ])) + p_input[ x ];
        }
        p_tmp1[ 0 ] = p_tmp2[ 0 ] + p_tmp2[ 1 ] + p_drms_tab_taos[ x++ ];
    }

    for( i = 0; i < 4; i++ )
    {
        uint8_t p_table[ 4 ][ 4 ] =
        {
            {  6, 11,  0,  5 },
            { 10, 15,  4,  9 },
            { 14,  3,  8, 13 },
            {  2,  7, 12,  5 }
        };

        p_tmp2[ 0 ] = ((p_tmp1[ 0 ] >> 0x1B)
                    |  (p_tmp1[ 0 ] << 0x5)) + p_tmp1[ 1 ];
        p_tmp2[ 1 ] = ((~p_tmp1[ 2 ] & p_tmp1[ 1 ])
                    |   (p_tmp1[ 2 ] & p_tmp2[ 0 ]))
                    +   p_input[ p_table[ i ][ 0 ] ];
        p_tmp2[ 1 ] += p_tmp1[ 3 ] + p_drms_tab_taos[ x++ ];

        p_tmp1[ 3 ] = ((p_tmp2[ 1 ] >> 0x17)
                    |  (p_tmp2[ 1 ] << 0x9)) + p_tmp2[ 0 ];
        p_tmp2[ 1 ] = ((~p_tmp1[ 1 ] & p_tmp2[ 0 ])
                    |   (p_tmp1[ 3 ] & p_tmp1[ 1 ]))
                    +   p_input[ p_table[ i ][ 1 ] ];
        p_tmp2[ 1 ] += p_tmp1[ 2 ] + p_drms_tab_taos[ x++ ];

        p_tmp1[ 2 ] = ((p_tmp2[ 1 ] >> 0x12)
                    |  (p_tmp2[ 1 ] << 0xE)) + p_tmp1[ 3 ];
        p_tmp2[ 1 ] = ((~p_tmp2[ 0 ] & p_tmp1[ 3 ])
                    |   (p_tmp1[ 2 ] & p_tmp2[ 0 ]))
                    +   p_input[ p_table[ i ][ 2 ] ];
        p_tmp2[ 1 ] += p_tmp1[ 1 ] + p_drms_tab_taos[ x++ ];

        p_tmp1[ 1 ] = ((p_tmp2[ 1 ] << 0x14)
                    |  (p_tmp2[ 1 ] >> 0xC)) + p_tmp1[ 2 ];
        if( i == 3 )
        {
            p_tmp2[ 1 ] = (p_tmp1[ 3 ] ^ p_tmp1[ 2 ] ^ p_tmp1[ 1 ])
                        + p_input[ p_table[ i ][ 3 ] ];
        }
        else
        {
            p_tmp2[ 1 ] = ((~p_tmp1[ 3 ] & p_tmp1[ 2 ])
                        |   (p_tmp1[ 3 ] & p_tmp1[ 1 ]))
                        +   p_input[ p_table[ i ][ 3 ] ];
        }
        p_tmp1[ 0 ] = p_tmp2[ 0 ] + p_tmp2[ 1 ] + p_drms_tab_taos[ x++ ];
    }

    for( i = 0; i < 4; i++ )
    {
        uint8_t p_table[ 4 ][ 4 ] =
        {
            {  8, 11, 14,  1 },
            {  4,  7, 10, 13 },
            {  0,  3,  6,  9 },
            { 12, 15,  2,  0 }
        };

        p_tmp2[ 0 ] = ((p_tmp1[ 0 ] >> 0x1C)
                    |  (p_tmp1[ 0 ] << 0x4)) + p_tmp1[ 1 ];
        p_tmp2[ 1 ] = (p_tmp1[ 2 ] ^ p_tmp1[ 1 ] ^ p_tmp2[ 0 ])
                    + p_input[ p_table[ i ][ 0 ] ];
        p_tmp2[ 1 ] += p_tmp1[ 3 ] + p_drms_tab_taos[ x++ ];

        p_tmp1[ 3 ] = ((p_tmp2[ 1 ] >> 0x15)
                    |  (p_tmp2[ 1 ] << 0xB)) + p_tmp2[ 0 ];
        p_tmp2[ 1 ] = (p_tmp1[ 3 ] ^ p_tmp1[ 1 ] ^ p_tmp2[ 0 ])
                    + p_input[ p_table[ i ][ 1 ] ];
        p_tmp2[ 1 ] += p_tmp1[ 2 ] + p_drms_tab_taos[ x++ ];

        p_tmp1[ 2 ] = ((p_tmp2[ 1 ] >> 0x10)
                    |  (p_tmp2[ 1 ] << 0x10)) + p_tmp1[ 3 ];
        p_tmp2[ 1 ] = (p_tmp1[ 3 ] ^ p_tmp1[ 2 ] ^ p_tmp2[ 0 ])
                    + p_input[ p_table[ i ][ 2 ] ];
        p_tmp2[ 1 ] += p_tmp1[ 1 ] + p_drms_tab_taos[ x++ ];

        p_tmp1[ 1 ] = ((p_tmp2[ 1 ] << 0x17)
                    |  (p_tmp2[ 1 ] >> 0x9)) + p_tmp1[ 2 ];
        if( i == 3 )
        {
            p_tmp2[ 1 ] = ((~p_tmp1[ 3 ] | p_tmp1[ 1 ]) ^ p_tmp1[ 2 ])
                        +   p_input[ p_table[ i ][ 3 ] ];
        }
        else
        {
            p_tmp2[ 1 ] = (p_tmp1[ 3 ] ^ p_tmp1[ 2 ] ^ p_tmp1[ 1 ])
                        + p_input[ p_table[ i ][ 3 ] ];
        }
        p_tmp1[ 0 ] = p_tmp2[ 0 ] + p_tmp2[ 1 ] + p_drms_tab_taos[ x++ ];
    }

    for( i = 0; i < 4; i++ )
    {
        uint8_t p_table[ 4 ][ 4 ] =
        {
            {  7, 14,  5, 12 },
            {  3, 10,  1,  8 },
            { 15,  6, 13,  4 },
            { 11,  2,  9,  0 }
        };

        p_tmp2[ 0 ] = ((p_tmp1[ 0 ] >> 0x1A)
                    |  (p_tmp1[ 0 ] << 0x6)) + p_tmp1[ 1 ];
        p_tmp2[ 1 ] = ((~p_tmp1[ 2 ] | p_tmp2[ 0 ]) ^ p_tmp1[ 1 ])
                    +   p_input[ p_table[ i ][ 0 ] ];
        p_tmp2[ 1 ] += p_tmp1[ 3 ] + p_drms_tab_taos[ x++ ];

        p_tmp1[ 3 ] = ((p_tmp2[ 1 ] >> 0x16)
                    |  (p_tmp2[ 1 ] << 0xA)) + p_tmp2[ 0 ];
        p_tmp2[ 1 ] = ((~p_tmp1[ 1 ] | p_tmp1[ 3 ]) ^ p_tmp2[ 0 ])
                    +   p_input[ p_table[ i ][ 1 ] ];
        p_tmp2[ 1 ] += p_tmp1[ 2 ] + p_drms_tab_taos[ x++ ];

        p_tmp1[ 2 ] = ((p_tmp2[ 1 ] >> 0x11)
                    |  (p_tmp2[ 1 ] << 0xF)) + p_tmp1[ 3 ];
        p_tmp2[ 1 ] = ((~p_tmp2[ 0 ] | p_tmp1[ 2 ]) ^ p_tmp1[ 3 ])
                    +   p_input[ p_table[ i ][ 2 ] ];
        p_tmp2[ 1 ] += p_tmp1[ 1 ] + p_drms_tab_taos[ x++ ];

        p_tmp1[ 1 ] = ((p_tmp2[ 1 ] << 0x15)
                    |  (p_tmp2[ 1 ] >> 0xB)) + p_tmp1[ 2 ];

        if( i < 3 )
        {
            p_tmp2[ 1 ] = ((~p_tmp1[ 3 ] | p_tmp1[ 1 ]) ^ p_tmp1[ 2 ])
                        +   p_input[ p_table[ i ][ 3 ] ];
            p_tmp1[ 0 ] = p_tmp2[ 0 ] + p_tmp2[ 1 ] + p_drms_tab_taos[ x++ ];
        }
    }

    p_buffer[ 0 ] += p_tmp2[ 0 ];
    p_buffer[ 1 ] += p_tmp1[ 1 ];
    p_buffer[ 2 ] += p_tmp1[ 2 ];
    p_buffer[ 3 ] += p_tmp1[ 3 ];
}

static void taos_add1( uint32_t *p_buffer,
                       uint8_t *p_in, uint32_t i_len )
{
    uint32_t i;
    uint32_t x, y;
    uint32_t p_tmp[ 16 ];
    uint32_t i_offset = 0;

    x = p_buffer[ 6 ] & 63;
    y = 64 - x;

    p_buffer[ 6 ] += i_len;

    if( i_len < y )
    {
        memcpy( &((uint8_t *)p_buffer)[ 48 + x ], p_in, i_len );
    }
    else
    {
        if( x )
        {
            memcpy( &((uint8_t *)p_buffer)[ 48 + x ], p_in, y );
            taos( &p_buffer[ 8 ], &p_buffer[ 12 ] );
            i_offset = y;
            i_len -= y;
        }

        if( i_len >= 64 )
        {
            for( i = 0; i < i_len / 64; i++ )
            {
                memcpy( p_tmp, &p_in[ i_offset ], sizeof(p_tmp) );
                taos( &p_buffer[ 8 ], p_tmp );
                i_offset += 64;
                i_len -= 64;
            }
        }

        if( i_len )
        {
            memcpy( &p_buffer[ 12 ], &p_in[ i_offset ], i_len );
        }
    }
}

static void taos_end1( uint32_t *p_buffer, uint32_t *p_out )
{
    uint32_t x, y;

    x = p_buffer[ 6 ] & 63;
    y = 63 - x;

    ((uint8_t *)p_buffer)[ 48 + x++ ] = 128;

    if( y < 8 )
    {
        memset( &((uint8_t *)p_buffer)[ 48 + x ], 0, y );
        taos( &p_buffer[ 8 ], &p_buffer[ 12 ] );
        y = 64;
        x = 0;
    }

    memset( &((uint8_t *)p_buffer)[ 48 + x ], 0, y );

    p_buffer[ 26 ] = p_buffer[ 6 ] * 8;
    p_buffer[ 27 ] = p_buffer[ 6 ] >> 29;
    taos( &p_buffer[ 8 ], &p_buffer[ 12 ] );

    memcpy( p_out, &p_buffer[ 8 ], sizeof(*p_out) * 4 );
}

static void taos_add2( uint32_t *p_buffer, uint8_t *p_in, uint32_t i_len )
{
    uint32_t i, x;
    uint32_t p_tmp[ 16 ];

    x = (p_buffer[ 0 ] / 8) & 63;
    i = p_buffer[ 0 ] + i_len * 8;

    if( i < p_buffer[ 0 ] )
    {
        p_buffer[ 1 ] += 1;
    }

    p_buffer[ 0 ] = i;
    p_buffer[ 1 ] += i_len >> 29;

    for( i = 0; i < i_len; i++ )
    {
        ((uint8_t *)p_buffer)[ 24 + x++ ] = p_in[ i ];

        if( x != 64 )
            continue;

        memcpy( p_tmp, &p_buffer[ 6 ], sizeof(p_tmp) );
        taos( &p_buffer[ 2 ], p_tmp );
    }
}

static void taos_add2e( uint32_t *p_buffer, uint32_t *p_in, uint32_t i_len )
{
    uint32_t i, x, y;
    uint32_t p_tmp[ 32 ];

    if( i_len )
    {
        for( x = i_len; x; x -= y )
        {
            y = x > 32 ? 32 : x;

            for( i = 0; i < y; i++ )
            {
                p_tmp[ i ] = U32_AT(&p_in[ i ]);
            }
        }
    }

    taos_add2( p_buffer, (uint8_t *)p_tmp, i_len * sizeof(p_tmp[ 0 ]) );
}

static void taos_end2( uint32_t *p_buffer )
{
    uint32_t x;
    uint32_t p_tmp[ 16 ];

    p_tmp[ 14 ] = p_buffer[ 0 ];
    p_tmp[ 15 ] = p_buffer[ 1 ];

    x = (p_buffer[ 0 ] / 8) & 63;

    taos_add2( p_buffer, p_drms_tab_tend, 56 - x );
    memcpy( p_tmp, &p_buffer[ 6 ], 56 );
    taos( &p_buffer[ 2 ], p_tmp );
    memcpy( &p_buffer[ 22 ], &p_buffer[ 2 ], sizeof(*p_buffer) * 4 );
}

static void taos_add3( uint32_t *p_buffer, uint8_t *p_key, uint32_t i_len )
{
    uint32_t x, y;
    uint32_t i = 0;

    x = (p_buffer[ 4 ] / 8) & 63;
    p_buffer[ 4 ] += i_len * 8;

    if( p_buffer[ 4 ] < i_len * 8 )
        p_buffer[ 5 ] += 1;

    p_buffer[ 5 ] += i_len >> 29;

    y = 64 - x;

    if( i_len >= y )
    {
        memcpy( &((uint8_t *)p_buffer)[ 24 + x ], p_key, y );
        taos( p_buffer, &p_buffer[ 6 ] );

        i = y;
        y += 63;

        if( y < i_len )
        {
            for( ; y < i_len; y += 64, i += 64 )
            {
                taos( p_buffer, (uint32_t *)&p_key[y - 63] );
            }
        }
        else
        {
            x = 0;
        }
    }

    memcpy( &((uint8_t *)p_buffer)[ 24 + x ], &p_key[ i ], i_len - i );
}

static int taos_osi( uint32_t *p_buffer )
{
    int i_ret = 0;

#ifdef _WIN32
    HKEY i_key;
    uint32_t i;
    DWORD i_size;
    DWORD i_serial;
    LPBYTE p_reg_buf;

    static LPCTSTR p_reg_keys[ 3 ][ 2 ] =
    {
        {
            _T("HARDWARE\\DESCRIPTION\\System"),
            _T("SystemBiosVersion")
        },

        {
            _T("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
            _T("ProcessorNameString")
        },

        {
            _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),
            _T("ProductId")
        }
    };

    taos_add1( p_buffer, "cache-control", 13 );
    taos_add1( p_buffer, "Ethernet", 8 );

    GetVolumeInformation( _T("C:\\"), NULL, 0, &i_serial,
                          NULL, NULL, NULL, 0 );
    taos_add1( p_buffer, (uint8_t *)&i_serial, 4 );

    for( i = 0; i < sizeof(p_reg_keys)/sizeof(p_reg_keys[ 0 ]); i++ )
    {
        if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, p_reg_keys[ i ][ 0 ],
                          0, KEY_READ, &i_key ) == ERROR_SUCCESS )
        {
            if( RegQueryValueEx( i_key, p_reg_keys[ i ][ 1 ],
                                 NULL, NULL, NULL,
                                 &i_size ) == ERROR_SUCCESS )
            {
                p_reg_buf = malloc( i_size );

                if( p_reg_buf != NULL )
                {
                    if( RegQueryValueEx( i_key, p_reg_keys[ i ][ 1 ],
                                         NULL, NULL, p_reg_buf,
                                         &i_size ) == ERROR_SUCCESS )
                    {
                        taos_add1( p_buffer, (uint8_t *)p_reg_buf,
                                   i_size );
                    }

                    free( p_reg_buf );
                }
            }

            RegCloseKey( i_key );
        }
    }

#else
    i_ret = -1;
#endif

    return( i_ret );
}

static int get_sci_data( uint32_t p_sci[ 11 ][ 4 ] )
{
    int i_ret = -1;

#ifdef _WIN32
    HANDLE i_file;
    DWORD i_size, i_read;
    TCHAR p_path[ MAX_PATH ];
    TCHAR *p_filename = _T("\\Apple Computer\\iTunes\\SC Info\\SC Info.sidb");

    typedef HRESULT (WINAPI *SHGETFOLDERPATH)( HWND, int, HANDLE, DWORD,
                                               LPTSTR );

    HINSTANCE shfolder_dll = NULL;
    SHGETFOLDERPATH dSHGetFolderPath = NULL;

    if( ( shfolder_dll = LoadLibrary( _T("SHFolder.dll") ) ) != NULL )
    {
        dSHGetFolderPath =
            (SHGETFOLDERPATH)GetProcAddress( shfolder_dll,
#ifdef _UNICODE
                                             _T("SHGetFolderPathW") );
#else
                                             _T("SHGetFolderPathA") );
#endif
    }

    if( dSHGetFolderPath != NULL &&
        SUCCEEDED( dSHGetFolderPath( NULL, /*CSIDL_COMMON_APPDATA*/ 0x0023,
                                     NULL, 0, p_path ) ) )
    {
        _tcsncat( p_path, p_filename, min( _tcslen( p_filename ),
                  (MAX_PATH-1) - _tcslen( p_path ) ) );

        i_file = CreateFile( p_path, GENERIC_READ, 0, NULL,
                             OPEN_EXISTING, 0, NULL );
        if( i_file != INVALID_HANDLE_VALUE )
        {
            i_read = sizeof(p_sci[ 0 ]) * 11;
            i_size = GetFileSize( i_file, NULL );
            if( i_size != INVALID_FILE_SIZE && i_size >= i_read )
            {
                i_size = SetFilePointer( i_file, 4, NULL, FILE_BEGIN );
                if( i_size != /*INVALID_SET_FILE_POINTER*/ ((DWORD)-1))
                {
                    if( ReadFile( i_file, p_sci, i_read, &i_size, NULL ) &&
                        i_size == i_read )
                    {
                        i_ret = 0;
                    }
                }
            }

            CloseHandle( i_file );
        }
    }
#endif

    return( i_ret );
}

static void acei_taxs( uint32_t *p_acei, uint32_t i_val )
{
    uint32_t i, x;

    i = (i_val / 16) & 15;
    x = (~(i_val & 15)) & 15;

    if( (i_val & 768) == 768 )
    {
        x = (~i) & 15;
        i = i_val & 15;

        p_acei[ 25 + i ] = p_acei[ 25 + ((16 - x) & 15) ]
                         + p_acei[ 25 + (15 - x) ];
    }
    else if( (i_val & 512) == 512 )
    {
        p_acei[ 25 + i ] ^= p_drms_tab_xor[ 15 - i ][ x ];
    }
    else if( (i_val & 256) == 256 )
    {
        p_acei[ 25 + i ] -= p_drms_tab_sub[ 15 - i ][ x ];
    }
    else
    {
        p_acei[ 25 + i ] += p_drms_tab_add[ 15 - i ][ x ];
    }
}

static void acei( uint32_t *p_acei, uint8_t *p_buffer, uint32_t i_len )
{
    uint32_t i, x;
    uint32_t p_tmp[ 26 ];

    for( i = 5; i < 25; i++ )
    {
        if( p_acei[ i ] )
        {
            acei_taxs( p_acei, p_acei[ i ] );
        }
    }

    TAOS_INIT( p_tmp, 2 );
    taos_add2e( p_tmp, &p_acei[ 25 ], sizeof(*p_acei) * 4 );
    taos_end2( p_tmp );

    x = i_len < 16 ? i_len : 16;

    if( x > 0 )
    {
        for( i = 0; i < x; i++ )
        {
            p_buffer[ i ] ^= ((uint8_t *)&p_tmp)[ 88 + i ];
        }
    }
}

static uint32_t ttov_calc( uint32_t *p_acei )
{
    int32_t i_val;
    uint32_t p_tmp[ 26 ];

    TAOS_INIT( p_tmp, 2 );
    taos_add2e( p_tmp, &p_acei[ 0 ], 4 );
    taos_add2e( p_tmp, &p_acei[ 4 ], 1 );
    taos_end2( p_tmp );

    p_acei[ 4 ]++;

    i_val = ((int32_t)U32_AT(&p_tmp[ 22 ])) % 1024;

    return( i_val < 0 ? i_val * -1 : i_val );
}

static void acei_init( uint32_t *p_acei, uint32_t *p_sys_key )
{
    uint32_t i;

    for( i = 0; i < 4; i++ )
    {
        p_acei[ i ] = U32_AT(&p_sys_key[ i ]);
    }

    p_acei[ 4 ] = 0x5476212A;

    for( i = 5; i < 25; i++ )
    {
        p_acei[ i ] = ttov_calc( p_acei );
    }

    p_acei[ 25 + 0 ] = p_acei[ 0 ];
    p_acei[ 25 + 1 ] = 0x68723876;
    p_acei[ 25 + 2 ] = 0x41617376;
    p_acei[ 25 + 3 ] = 0x4D4B4F76;

    p_acei[ 25 + 4 ] = p_acei[ 1 ];
    p_acei[ 25 + 5 ] = 0x48556646;
    p_acei[ 25 + 6 ] = 0x38393725;
    p_acei[ 25 + 7 ] = 0x2E3B5B3D;

    p_acei[ 25 + 8 ] = p_acei[ 2 ];
    p_acei[ 25 + 9 ] = 0x37363866;
    p_acei[ 25 + 10 ] = 0x30383637;
    p_acei[ 25 + 11 ] = 0x34333661;

    p_acei[ 25 + 12 ] = p_acei[ 3 ];
    p_acei[ 25 + 13 ] = 0x37386162;
    p_acei[ 25 + 14 ] = 0x494F6E66;
    p_acei[ 25 + 15 ] = 0x2A282966;
}

static __inline void block_xor( uint32_t *p_in, uint32_t *p_key,
                              uint32_t *p_out )
{
    uint32_t i;

    for( i = 0; i < 4; i++ )
    {
        p_out[ i ] = p_key[ i ] ^ p_in[ i ];
    }
}

int drms_get_sys_key( uint32_t *p_sys_key )
{
    uint32_t p_tmp[ 128 ];
    uint32_t p_tmp_key[ 4 ];

    TAOS_INIT( p_tmp, 8 );
    if( taos_osi( p_tmp ) )
    {
        return( -1 );
    }
    taos_end1( p_tmp, p_tmp_key );

    TAOS_INIT( p_tmp, 2 );
    taos_add2( p_tmp, "YuaFlafu", 8 );
    taos_add2( p_tmp, (uint8_t *)p_tmp_key, 6 );
    taos_add2( p_tmp, (uint8_t *)p_tmp_key, 6 );
    taos_add2( p_tmp, (uint8_t *)p_tmp_key, 6 );
    taos_add2( p_tmp, "zPif98ga", 8 );
    taos_end2( p_tmp );

    memcpy( p_sys_key, &p_tmp[ 2 ], sizeof(*p_sys_key) * 4 );

    return( 0 );
}

int drms_get_user_key( uint32_t *p_sys_key, uint32_t *p_user_key )
{
    uint32_t i;
    uint32_t p_tmp[ 4 ];
    uint32_t *p_cur_key;
    uint32_t p_acei[ 41 ];
    uint32_t p_ctx[ 128 ];
    uint32_t p_sci[ 2 ][ 11 ][ 4 ];

    uint32_t p_sci_key[ 4 ] =
    {
        0x6E66556D, 0x6E676F70, 0x67666461, 0x33373866
    };

    if( p_sys_key == NULL )
    {
        if( drms_get_sys_key( p_tmp ) )
        {
            return( -1 );
        }

        p_sys_key = p_tmp;
    }

    if( get_sci_data( p_sci[ 0 ] ) )
    {
        return( -1 );
    }

    init_ctx( p_ctx, p_sys_key );

    for( i = 0, p_cur_key = p_sci_key;
         i < sizeof(p_sci[ 0 ])/sizeof(p_sci[ 0 ][ 0 ]); i++ )
    {
        ctx_xor( p_ctx, &p_sci[ 0 ][ i ][ 0 ], &p_sci[ 1 ][ i ][ 0 ],
                 p_drms_tab3, p_drms_tab4 );
        block_xor( &p_sci[ 1 ][ i ][ 0 ], p_cur_key, &p_sci[ 1 ][ i ][ 0 ] );

        p_cur_key = &p_sci[ 0 ][ i ][ 0 ];
    }

    acei_init( p_acei, p_sys_key );

    for( i = 0; i < sizeof(p_sci[ 1 ])/sizeof(p_sci[ 1 ][ 0 ]); i++ )
    {
        acei( p_acei, (uint8_t *)&p_sci[ 1 ][ i ][ 0 ],
              sizeof(p_sci[ 1 ][ i ]) );
    }

    memcpy( p_user_key, &p_sci[ 1 ][ 10 ][ 0 ], sizeof(p_sci[ 1 ][ i ]) );

    return( 0 );
}

struct drms_s
{
    uint8_t *p_iviv;
    uint32_t i_iviv_len;
    uint8_t *p_name;
    uint32_t i_name_len;

    uint32_t *p_tmp;
    uint32_t i_tmp_len;

    uint32_t p_key[ 4 ];
    uint32_t p_ctx[ 128 ];
};

#define P_DRMS ((struct drms_s *)p_drms)

void *drms_alloc()
{
    struct drms_s *p_drms;

    p_drms = malloc( sizeof(struct drms_s) );

    if( p_drms != NULL )
    {
        memset( p_drms, 0, sizeof(struct drms_s) );

        p_drms->i_tmp_len = 1024;
        p_drms->p_tmp = malloc( p_drms->i_tmp_len );
        if( p_drms->p_tmp == NULL )
        {
            free( (void *)p_drms );
            p_drms = NULL;
        }
    }

    return( (void *)p_drms );
}

void drms_free( void *p_drms )
{
    if( P_DRMS->p_name != NULL )
    {
        free( (void *)P_DRMS->p_name );
    }

    if( P_DRMS->p_iviv != NULL )
    {
        free( (void *)P_DRMS->p_iviv );
    }

    if( P_DRMS->p_tmp != NULL )
    {
        free( (void *)P_DRMS->p_tmp );
    }

    free( p_drms );
}

void drms_decrypt( void *p_drms, uint32_t *p_buffer, uint32_t i_len )
{
    uint32_t i, x, y;
    uint32_t *p_cur_key = P_DRMS->p_key;

    x = (i_len / sizeof(P_DRMS->p_key)) * sizeof(P_DRMS->p_key);

    if( P_DRMS->i_tmp_len < x )
    {
        free( (void *)P_DRMS->p_tmp );

        P_DRMS->i_tmp_len = x;
        P_DRMS->p_tmp = malloc( P_DRMS->i_tmp_len );
    }

    if( P_DRMS->p_tmp != NULL )
    {
        memcpy( P_DRMS->p_tmp, p_buffer, x );

        for( i = 0, x /= sizeof(P_DRMS->p_key); i < x; i++ )
        {
            y = i * sizeof(*p_buffer);

            ctx_xor( P_DRMS->p_ctx, P_DRMS->p_tmp + y, p_buffer + y,
                     p_drms_tab3, p_drms_tab4 );
            block_xor( p_buffer + y, p_cur_key, p_buffer + y );

            p_cur_key = P_DRMS->p_tmp + y;
        }
    }
}

int drms_init( void *p_drms, uint32_t i_type,
               uint8_t *p_info, uint32_t i_len )
{
    int i_ret = 0;

    switch( i_type )
    {
        case DRMS_INIT_UKEY:
        {
            if( i_len != sizeof(P_DRMS->p_key) )
            {
                i_ret = -1;
                break;
            }

            init_ctx( P_DRMS->p_ctx, (uint32_t *)p_info );
        }
        break;

        case DRMS_INIT_IVIV:
        {
            if( i_len != sizeof(P_DRMS->p_key) )
            {
                i_ret = -1;
                break;
            }

            P_DRMS->p_iviv = malloc( i_len );
            if( P_DRMS->p_iviv == NULL )
            {
                i_ret = -1;
                break;
            }

            memcpy( P_DRMS->p_iviv, p_info, i_len );
            P_DRMS->i_iviv_len = i_len;
        }
        break;

        case DRMS_INIT_NAME:
        {
            P_DRMS->p_name = malloc( i_len );
            if( P_DRMS->p_name == NULL )
            {
                i_ret = -1;
                break;
            }

            memcpy( P_DRMS->p_name, p_info, i_len );
            P_DRMS->i_name_len = i_len;
        }
        break;

        case DRMS_INIT_PRIV:
        {
            uint32_t i;
            uint32_t p_priv[ 64 ];
            uint32_t p_tmp[ 128 ];

            if( i_len < 64 )
            {
                i_ret = -1;
                break;
            }

            TAOS_INIT( p_tmp, 0 );
            taos_add3( p_tmp, P_DRMS->p_name, P_DRMS->i_name_len );
            taos_add3( p_tmp, P_DRMS->p_iviv, P_DRMS->i_iviv_len );
            memcpy( p_priv, &p_tmp[ 4 ], sizeof(p_priv[ 0 ]) * 2 );
            i = (p_tmp[ 4 ] / 8) & 63;
            i = i >= 56 ? 120 - i : 56 - i;
            taos_add3( p_tmp, p_drms_tab_tend, i );
            taos_add3( p_tmp, (uint8_t *)p_priv, sizeof(p_priv[ 0 ]) * 2 );

            memcpy( p_priv, p_info, 64 );
            memcpy( P_DRMS->p_key, p_tmp, sizeof(P_DRMS->p_key) );
            drms_decrypt( p_drms, p_priv, sizeof(p_priv) );

            init_ctx( P_DRMS->p_ctx, &p_priv[ 6 ] );
            memcpy( P_DRMS->p_key, &p_priv[ 12 ], sizeof(P_DRMS->p_key) );

            free( (void *)P_DRMS->p_name );
            P_DRMS->p_name = NULL;
            free( (void *)P_DRMS->p_iviv );
            P_DRMS->p_iviv = NULL;
        }
        break;
    }

    return( i_ret );
}

#undef P_DRMS

#endif

