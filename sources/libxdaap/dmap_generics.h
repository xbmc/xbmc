/* dmap
 *
 * generic definitions
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

#include "daap.h"

/* anonymous container */
typedef struct _dmapGenericItem dmapGenericItem;

struct _dmapGenericItem
{
    dmap_contentCodeFOURCC cc;
    dmap_GenericType       data;
    dmapGenericItem        *next;
};

typedef struct
{
    struct _dmapGenericItem *head;
} dmapGenericContainer;

void freeGenericContainer(dmapGenericContainer *gc);

void dmapGeneric_DumpContainerCCs(dmapGenericContainer *c);

dmap_GenericType dmapGeneric_LookupContainerItem(dmapGenericContainer *c,
                                            const dmap_ContentCode *code);

dmap_DataTypes dmapGeneric_LookupContainerItem_INT8(
        dmapGenericContainer *c, const dmap_ContentCode *code,
        DMAP_INT8 *out);
dmap_DataTypes dmapGeneric_LookupContainerItem_UINT8(
        dmapGenericContainer *c, const dmap_ContentCode *code,
        DMAP_UINT8 *out);
dmap_DataTypes dmapGeneric_LookupContainerItem_INT16(
        dmapGenericContainer *c, const dmap_ContentCode *code,
        DMAP_INT16 *out);
dmap_DataTypes dmapGeneric_LookupContainerItem_UINT16(
        dmapGenericContainer *c, const dmap_ContentCode *code,
        DMAP_UINT16 *out);
dmap_DataTypes dmapGeneric_LookupContainerItem_INT32(
        dmapGenericContainer *c, const dmap_ContentCode *code,
        DMAP_INT32 *out);
dmap_DataTypes dmapGeneric_LookupContainerItem_UINT32(
        dmapGenericContainer *c, const dmap_ContentCode *code,
        DMAP_UINT32 *out);
dmap_DataTypes dmapGeneric_LookupContainerItem_INT64(
        dmapGenericContainer *c, const dmap_ContentCode *code,
        DMAP_INT64 *out);
dmap_DataTypes dmapGeneric_LookupContainerItem_UINT64(
        dmapGenericContainer *c, const dmap_ContentCode *code,
        DMAP_UINT64 *out);
dmap_DataTypes dmapGeneric_LookupContainerItem_STRING(
        dmapGenericContainer *c, const dmap_ContentCode *code,
        DMAP_STRING *out);
dmap_DataTypes dmapGeneric_LookupContainerItem_TIME(
        dmapGenericContainer *c, const dmap_ContentCode *code,
        DMAP_TIME *out);
dmap_DataTypes dmapGeneric_LookupContainerItem_VERSION(
        dmapGenericContainer *c, const dmap_ContentCode *code,
        DMAP_VERSION *out);

/* listing container */

typedef struct
{
    protoParseResult h;
    DMAP_INT32 totalcount;
    DMAP_INT32 returnedcount;
    dmapGenericContainer *listitems;
    int curIndex; /* used for building */
} protoParseResult_genericPreListing;

void preListingContainer(dmap_contentCodeFOURCC code,
                         const int size, const char *buffer,
                         void *scopeData);

void freeGenericPreListing(protoParseResult_genericPreListing *prelist);

