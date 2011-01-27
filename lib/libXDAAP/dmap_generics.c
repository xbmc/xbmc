// Place the code and data below here into the LIBXDAAP section.
#ifndef __GNUC__
#pragma code_seg( "LIBXDAAP_TEXT" )
#pragma data_seg( "LIBXDAAP_DATA" )
#pragma bss_seg( "LIBXDAAP_BSS" )
#pragma const_seg( "LIBXDAAP_RD" )
#endif

/* dmap generic defintions
 *
 * Copyright (c) 2004 David Hammerton
 *
 * This file implements some generic protocol containers
 * and utility functions for them.
 * See comments on the individual segments for more details.
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "portability.h"

#include "debug.h"
#include "daap.h"
#include "daap_contentcodes.h"
#include "dmap_generics.h"

#define DEFAULT_DEBUG_CHANNEL "daap"

#include "daap_readtypes.h"

#define UNHANDLED_CONTENT_CODE \
    do { ERR("unhandled content code [%c%c%c%c]\n", \
         SPLITFOURCC(code) \
         ); } while(0)


/* util functions for accessing dmapGenericContainer */
dmap_GenericType dmapGeneric_LookupContainerItem(dmapGenericContainer *c,
                                            const dmap_ContentCode *code)
{
    dmap_GenericType invalidGeneric = { {0}, DMAP_DATATYPE_INVALID };
    dmapGenericItem *cur = c->head;
    while (cur)
    {
        if (code->cc_number == cur->cc)
            return cur->data;
        cur = cur->next;
    }
    return invalidGeneric;
}

void dmapGeneric_DumpContainerCCs(dmapGenericContainer *c)
{
    dmapGenericItem *cur = c->head;
    while (cur)
    {
        TRACE("cc: %c%c%c%c\n", SPLITFOURCC(cur->cc));
        cur = cur->next;
    }
}

#define IMPLEMENT_LOOKUPCONTAINERITEM_TYPE(type) \
dmap_DataTypes dmapGeneric_LookupContainerItem_##type( \
        dmapGenericContainer *c, const dmap_ContentCode *code, \
        DMAP_##type *out) \
{ \
    dmap_GenericType r; \
    r = dmapGeneric_LookupContainerItem(c, code); \
    return dmap_genericTo##type(r, out); \
}
IMPLEMENT_LOOKUPCONTAINERITEM_TYPE(INT8)
IMPLEMENT_LOOKUPCONTAINERITEM_TYPE(UINT8)
IMPLEMENT_LOOKUPCONTAINERITEM_TYPE(INT16)
IMPLEMENT_LOOKUPCONTAINERITEM_TYPE(UINT16)
IMPLEMENT_LOOKUPCONTAINERITEM_TYPE(INT32)
IMPLEMENT_LOOKUPCONTAINERITEM_TYPE(UINT32)
IMPLEMENT_LOOKUPCONTAINERITEM_TYPE(INT64)
IMPLEMENT_LOOKUPCONTAINERITEM_TYPE(UINT64)
IMPLEMENT_LOOKUPCONTAINERITEM_TYPE(STRING)
IMPLEMENT_LOOKUPCONTAINERITEM_TYPE(TIME)
IMPLEMENT_LOOKUPCONTAINERITEM_TYPE(VERSION)
#undef IMPLEMENT_LOOKUPCONTAINERITEM_TYPE

/* CONTAINER GENERIC
 * This is used by (at least) daap.serverdatabases and
 * daap.databasesongs - both from /databases urls.
 *
 * This is basically any container that contains the following:
 *  dmap.status
 *  dmap.updatetype
 *  dmap.specifiedtotalcount
 *  dmap.returnedcount
 *  dmap.listing
 *    This one is then a bit more complex, it is a container
 *    that contains a whole bunch of dmap.listingitems (yet another
 *    container).
 *    Now this is where things get odd - dmap.listingitems can
 *    contain (highly) variable sub items.
 */

void freeGenericContainer(dmapGenericContainer *gc)
{
    dmapGenericItem *item = gc->head;
    while (item)
    {
        dmapGenericItem *next = item->next;
        if (item->data.type == DMAP_DATATYPE_STRING)
            free(item->data.u.string);
        free(item);
        item = next;
    }
}

void freeGenericPreListing(protoParseResult_genericPreListing *prelist)
{
    int i;
    for (i = 0; i < prelist->returnedcount; i++)
    {
        dmapGenericContainer *item = &(prelist->listitems[i]);
        freeGenericContainer(item);
    }
    free(prelist->listitems);
}

static void listitemGenericContainer(dmap_contentCodeFOURCC code,
                                     const int size, const char *buffer,
                                     void *scopeData)
{
    dmapGenericContainer *gc = (dmapGenericContainer*)scopeData;
    const dmap_ContentCode *cc;
    dmap_DataTypes type = DMAP_DATATYPE_INVALID;
    dmapGenericItem *newItem;

    if ((cc = dmap_lookupCodeFromFOURCC(&dmap_table, code)))
    {
        type = cc->cc_type;
    }
    else if ((cc = dmap_lookupCodeFromFOURCC(&daap_table, code)))
    {
        type = cc->cc_type;
    }
    else if ((cc = dmap_lookupCodeFromFOURCC(&com_table, code)))
    {
        type = cc->cc_type;
    }
    if (type == DMAP_DATATYPE_INVALID || type == DMAP_DATATYPE_CONTAINER)
    {
        UNHANDLED_CONTENT_CODE;
        return;
    }
//    dmapGenericItem *newItem = (dmapGenericItem *) malloc(sizeof(dmapGenericItem));
    newItem = malloc(sizeof(dmapGenericItem));
    newItem->cc = code;
    newItem->data.type = type;
    switch(type)
    {
        case DMAP_DATATYPE_INT8:
            newItem->data.u.int8 = readBigEndian_INT8(buffer, size);
            break;
        case DMAP_DATATYPE_UINT8:
            newItem->data.u.uint8 = readBigEndian_UINT8(buffer, size);
            break;
        case DMAP_DATATYPE_INT16:
            newItem->data.u.int16 = readBigEndian_INT16(buffer, size);
            break;
        case DMAP_DATATYPE_UINT16:
            newItem->data.u.uint16 = readBigEndian_UINT16(buffer, size);
            break;
        case DMAP_DATATYPE_INT32:
            newItem->data.u.int32 = readBigEndian_INT32(buffer, size);
            break;
        case DMAP_DATATYPE_UINT32:
            newItem->data.u.uint32 = readBigEndian_UINT32(buffer, size);
            break;
        case DMAP_DATATYPE_INT64:
            newItem->data.u.int64 = readBigEndian_INT64(buffer, size);
            break;
        case DMAP_DATATYPE_UINT64:
            newItem->data.u.uint64 = readBigEndian_UINT64(buffer, size);
            break;
        case DMAP_DATATYPE_STRING:
            newItem->data.u.string = read_string_withalloc(buffer, size);
            break;
        case DMAP_DATATYPE_VERSION:
            newItem->data.u.version = read_version(buffer, size);
            break;
        case DMAP_DATATYPE_TIME:
            FIXME("read time\n");
        case DMAP_DATATYPE_INVALID:
        case DMAP_DATATYPE_CONTAINER:
            TRACE("can't handle this type\n");
            free(newItem);
            return;
    }
    newItem->next = gc->head;
    gc->head = newItem;
}

static void listingContainer(dmap_contentCodeFOURCC code,
                             const int size, const char *buffer,
                             void *scopeData)
{
    protoParseResult_genericPreListing* sd =
        (protoParseResult_genericPreListing*)scopeData;
    if (dmap_isCC(code, dmap_l("listingitem")) == DMAP_DATATYPE_CONTAINER)
    {
        dmap_parseContainer(listitemGenericContainer, size, buffer,
                            &(sd->listitems[sd->curIndex]));
        sd->curIndex++;
    }
    else
        UNHANDLED_CONTENT_CODE;
}

void preListingContainer(dmap_contentCodeFOURCC code,
                         const int size, const char *buffer,
                         void *scopeData)
{
    protoParseResult_genericPreListing* sd =
        (protoParseResult_genericPreListing*)scopeData;
    if (dmap_isCC(code, dmap_l("status")) == DMAP_DATATYPE_INT32)
    {
        DMAP_INT32 status = readBigEndian_INT32(buffer, size);
        if (status != 200)
            FIXME("unknown status code %i\n", status);
    }
    else if (dmap_isCC(code, dmap_l("updatetype")) == DMAP_DATATYPE_INT8)
    {
        DMAP_INT8 updatetype = readBigEndian_INT8(buffer, size);
        if (updatetype != 0)
            FIXME("unknown updatetype %i\n", updatetype);
    }
    else if (dmap_isCC(code, dmap_l("specifiedtotalcount")) == DMAP_DATATYPE_INT32)
    {
        sd->totalcount = readBigEndian_INT32(buffer, size);
    }
    else if (dmap_isCC(code, dmap_l("returnedcount")) == DMAP_DATATYPE_INT32)
    {
        sd->returnedcount = readBigEndian_INT32(buffer, size);
    }
    else if (dmap_isCC(code, dmap_l("listing")) == DMAP_DATATYPE_CONTAINER)
    {
        /* FIXME: need a way to ensure sd->returnedcount has been set */
        sd->curIndex = 0;
        if (sd->returnedcount) /* eh? mt-daapd sets to 0 */
        {
            /* needs to be inited to 0 - linked list uses head for next */
            sd->listitems = (dmapGenericContainer*)calloc(sd->returnedcount, sizeof(dmapGenericContainer));
            dmap_parseContainer(listingContainer, size, buffer, (void*)sd);
        }
        else sd->listitems = NULL;
    }
    else
        UNHANDLED_CONTENT_CODE;

}

