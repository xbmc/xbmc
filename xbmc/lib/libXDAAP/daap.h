/* daap
 *
 * core protocol definitions
 *
 * Copyright (c) 2004 David Hammerton
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/* DMAP is probable a parent protocol which DAAP
 * is implemented over.
 * For now I'm not splitting them up, but all
 * DMAP_ things seem to be generic to this parent
 * protocol
 */

#ifndef _DAAP_H
#define _DAAP_H

#include "portability.h"

typedef int32_t dmap_contentCodeFOURCC;
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
    ( (int32_t)(char)(ch0) | ( (int32_t)(char)(ch1) << 8 ) | \
    ( (int32_t)(char)(ch2) << 16 ) | \
    ( (int32_t)(char)(ch3) << 24 ) )
#endif

#ifndef SLPITFOURCC
/* splits it into ch0, ch1, ch2, ch3 - use for printf's */
#define SPLITFOURCC(code) \
    (char)code, (char)((int32_t)code >> 8), \
    (char)((int32_t)code >> 16), \
    (char)((int32_t)code >> 24)


#endif

/* I'm not sure if these ints are signed or not,
 * however I would hazard a guess that they are signed,
 * with types 2,4,6 and 8 being unsigned.
 */

typedef enum
{
    DMAP_DATATYPE_INVALID   = -1,

    DMAP_DATATYPE_INT8      = 1,
    DMAP_DATATYPE_UINT8     = 2,
    DMAP_DATATYPE_INT16     = 3,
    DMAP_DATATYPE_UINT16    = 4,
    DMAP_DATATYPE_INT32     = 5,
    DMAP_DATATYPE_UINT32    = 6,
    DMAP_DATATYPE_INT64     = 7,
    DMAP_DATATYPE_UINT64    = 8,

    DMAP_DATATYPE_STRING    = 9,
    DMAP_DATATYPE_TIME      = 10,
    DMAP_DATATYPE_VERSION   = 11,
    DMAP_DATATYPE_CONTAINER = 12
} dmap_DataTypes;

typedef int8_t  DMAP_INT8;
typedef u_int8_t  DMAP_UINT8;

typedef int16_t DMAP_INT16;
typedef u_int16_t DMAP_UINT16;

typedef int32_t DMAP_INT32;
typedef u_int32_t DMAP_UINT32;

typedef int64_t DMAP_INT64;
typedef u_int64_t DMAP_UINT64;

typedef char* DMAP_STRING;
typedef time_t DMAP_TIME;
typedef struct { int16_t v1, v2; } DMAP_VERSION;

typedef struct
{
    union
    {
        DMAP_INT8 int8;
        DMAP_UINT8 uint8;
        DMAP_INT16 int16;
        DMAP_UINT16 uint16;
        DMAP_INT32 int32;
        DMAP_UINT32 uint32;
        DMAP_INT64 int64;
        DMAP_UINT64 uint64;
        DMAP_STRING string;
        DMAP_TIME time;
        DMAP_VERSION version;
    } u;
    dmap_DataTypes type;
} dmap_GenericType;

typedef struct
{
    dmap_contentCodeFOURCC  cc_number;
    char *                  cc_name;
    dmap_DataTypes          cc_type;
} dmap_ContentCode;

/* to query the protocol parser engine */
typedef enum
{
    QUERY_SERVERINFORESPONSE = 0,
    QUERY_LOGINRESPONSE,
    QUERY_UPDATERESPONSE,
    QUERY_GENERICLISTING
} PROTO_PARSE_EXPECTING;

typedef struct
{
    PROTO_PARSE_EXPECTING expecting;
} protoParseResult;

typedef struct
{
    protoParseResult h;
    DMAP_VERSION dmap_version;
    DMAP_VERSION daap_version;
    DMAP_INT32 databasescount;
    DMAP_STRING hostname;
} protoParseResult_serverinfo;

typedef struct
{
    protoParseResult h;
    DMAP_INT32 sessionid;
} protoParseResult_login;

typedef struct
{
    protoParseResult h;
    DMAP_INT32 serverrevision;
} protoParseResult_update;

/* functions */
/* dmap_init MUST be called during process attach. */
void dmap_init();
void dmap_deinit();
void dmap_parseProtocolData(const int size, const char *buffer,
                            protoParseResult *res);

/* generic type */
dmap_DataTypes dmap_genericToINT8(dmap_GenericType in, DMAP_INT8 *out);
dmap_DataTypes dmap_genericToUINT8(dmap_GenericType in, DMAP_UINT8 *out);
dmap_DataTypes dmap_genericToINT16(dmap_GenericType in, DMAP_INT16 *out);
dmap_DataTypes dmap_genericToUINT16(dmap_GenericType in, DMAP_UINT16 *out);
dmap_DataTypes dmap_genericToINT32(dmap_GenericType in, DMAP_INT32 *out);
dmap_DataTypes dmap_genericToUINT32(dmap_GenericType in, DMAP_UINT32 *out);
dmap_DataTypes dmap_genericToINT64(dmap_GenericType in, DMAP_INT64 *out);
dmap_DataTypes dmap_genericToUINT64(dmap_GenericType in, DMAP_UINT64 *out);

dmap_DataTypes dmap_genericToSTRING(dmap_GenericType in, DMAP_STRING *out);
dmap_DataTypes dmap_genericToTIME(dmap_GenericType in, DMAP_TIME *out);
dmap_DataTypes dmap_genericToVERSION(dmap_GenericType in, DMAP_VERSION *out);


#endif /* _DAAP_H */

