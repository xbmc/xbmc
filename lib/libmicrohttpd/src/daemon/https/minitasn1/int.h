/*
 *      Copyright (C) 2004, 2006 Free Software Foundation, Inc.
 *      Copyright (C) 2002 Fabio Fiorina
 *
 * This file is part of LIBTASN1.
 *
 * The LIBTASN1 library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#ifndef INT_H
#define INT_H

#include <libtasn1.h>
#include <defines.h>

/*
#define LIBTASN1_DEBUG
#define LIBTASN1_DEBUG_PARSER
#define LIBTASN1_DEBUG_INTEGER
*/

#include <mem.h>

#define MAX_LOG_SIZE 1024       /* maximum number of characters of a log message */

/* Define used for visiting trees. */
#define UP     1
#define RIGHT  2
#define DOWN   3

/****************************************/
/* Returns the first 8 bits.            */
/* Used with the field type of node_asn */
/****************************************/
#define type_field(x)     (x&0xFF)

/* List of constants for field type of typedef node_asn  */
#define TYPE_CONSTANT       1
#define TYPE_IDENTIFIER     2
#define TYPE_INTEGER        3
#define TYPE_BOOLEAN        4
#define TYPE_SEQUENCE       5
#define TYPE_BIT_STRING     6
#define TYPE_OCTET_STRING   7
#define TYPE_TAG            8
#define TYPE_DEFAULT        9
#define TYPE_SIZE          10
#define TYPE_SEQUENCE_OF   11
#define TYPE_OBJECT_ID     12
#define TYPE_ANY           13
#define TYPE_SET           14
#define TYPE_SET_OF        15
#define TYPE_DEFINITIONS   16
#define TYPE_TIME          17
#define TYPE_CHOICE        18
#define TYPE_IMPORTS       19
#define TYPE_NULL          20
#define TYPE_ENUMERATED    21
#define TYPE_GENERALSTRING 27


/***********************************************************************/
/* List of constants to better specify the type of typedef node_asn.   */
/***********************************************************************/
/*  Used with TYPE_TAG  */
#define CONST_UNIVERSAL   (1<<8)
#define CONST_PRIVATE     (1<<9)
#define CONST_APPLICATION (1<<10)
#define CONST_EXPLICIT    (1<<11)
#define CONST_IMPLICIT    (1<<12)

#define CONST_TAG         (1<<13)       /*  Used in ASN.1 assignement  */
#define CONST_OPTION      (1<<14)
#define CONST_DEFAULT     (1<<15)
#define CONST_TRUE        (1<<16)
#define CONST_FALSE       (1<<17)

#define CONST_LIST        (1<<18)       /*  Used with TYPE_INTEGER and TYPE_BIT_STRING  */
#define CONST_MIN_MAX     (1<<19)

#define CONST_1_PARAM     (1<<20)

#define CONST_SIZE        (1<<21)

#define CONST_DEFINED_BY  (1<<22)

#define CONST_GENERALIZED (1<<23)
#define CONST_UTC         (1<<24)

/* #define CONST_IMPORTS     (1<<25) */

#define CONST_NOT_USED    (1<<26)
#define CONST_SET         (1<<27)
#define CONST_ASSIGN      (1<<28)

#define CONST_DOWN        (1<<29)
#define CONST_RIGHT       (1<<30)

#endif /* INT_H */
