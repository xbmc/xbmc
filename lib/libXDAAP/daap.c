// Place the code and data below here into the LIBXDAAP section.
#ifndef __GNUC__
#pragma code_seg( "LIBXDAAP_TEXT" )
#pragma data_seg( "LIBXDAAP_DATA" )
#pragma bss_seg( "LIBXDAAP_BSS" )
#pragma const_seg( "LIBXDAAP_RD" )
#endif

/* daap
 *
 * core protocol
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

#include "portability.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libXDAAP.h"

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

static int dmap_initilized = 0;

#define IMPLEMENT_GENERICTOTYPE(_type, umember) \
dmap_DataTypes dmap_genericTo##_type(dmap_GenericType in, DMAP_##_type *out) \
{ \
    if (in.type != DMAP_DATATYPE_##_type) return in.type; \
    *out = in.u.umember; \
    return in.type; \
}
IMPLEMENT_GENERICTOTYPE(INT8, int8)
IMPLEMENT_GENERICTOTYPE(UINT8, uint8)
IMPLEMENT_GENERICTOTYPE(INT16, int16)
IMPLEMENT_GENERICTOTYPE(UINT16, uint16)
IMPLEMENT_GENERICTOTYPE(INT32, int32)
IMPLEMENT_GENERICTOTYPE(UINT32, uint32)
IMPLEMENT_GENERICTOTYPE(INT64, int64)
IMPLEMENT_GENERICTOTYPE(UINT64, uint64)
IMPLEMENT_GENERICTOTYPE(STRING, string)
IMPLEMENT_GENERICTOTYPE(TIME, time)
IMPLEMENT_GENERICTOTYPE(VERSION, version)
#undef IMPLEMENT_GENERICTOTYPE


/* FIXME - replace with dictionary implementation */
dmap_ContentCode_table dmap_table =
    {
        "dmap",
        NULL
    };

dmap_ContentCode_table daap_table =
    {
        "daap",
        NULL
    };

dmap_ContentCode_table com_table =
    {
        "com",
        NULL
    };


const dmap_ContentCode *dmap_lookupCode(const dmap_ContentCode_table *table,
                                        const char *name)
{
    dmap_ContentCodeContainer *cur = table->codes;
    while (cur)
    {
        if (strcmp(cur->c.cc_name, name) == 0)
            return &cur->c;
        cur = cur->next;
    }
    return NULL;
}

const dmap_ContentCode *dmap_lookupCodeFromFOURCC(const dmap_ContentCode_table *table,
                                                  const dmap_contentCodeFOURCC fourcc)
{
    dmap_ContentCodeContainer *cur = table->codes;
    while (cur)
    {
        if (cur->c.cc_number == fourcc)
            return &cur->c;
        cur = cur->next;
    }
    return NULL;
}

void dmap_addCode(dmap_ContentCode_table *table, const char *name,
                  const dmap_contentCodeFOURCC code, const dmap_DataTypes type)
{
    dmap_ContentCodeContainer *newItem;
    const dmap_ContentCode *existing = dmap_lookupCode(table, name);

    if (existing)
    {
        if (existing->cc_number != code)
            ERR("code for existing content code differs [%s] [%c%c%c%c vs %c%c%c%c]\n",
                name, SPLITFOURCC(existing->cc_number), SPLITFOURCC(code));
        if (existing->cc_type != type)
            ERR("type for existing content code differs [%s] [%i vs %i]\n",
                name, existing->cc_type, type);
        return;
    }

    newItem = (dmap_ContentCodeContainer *) malloc(sizeof(dmap_ContentCodeContainer) + strlen(name) + 1);
    newItem->c.cc_number = code;
    newItem->c.cc_name = (char*)newItem + sizeof(dmap_ContentCodeContainer);
    strcpy(newItem->c.cc_name, name);
    newItem->c.cc_type = type;

    newItem->next = table->codes;
    table->codes = newItem;
}

static const char *getTypeString(dmap_DataTypes type)
{
    switch (type)
    {
        case DMAP_DATATYPE_INVALID:
            return "DMAP_DATATYPE_INVALID\n";

        case DMAP_DATATYPE_INT8:
            return "DMAP_DATATYPE_INT8";
        case DMAP_DATATYPE_UINT8:
            return "DMAP_DATATYPE_UINT8";

        case DMAP_DATATYPE_INT16:
            return "DMAP_DATATYPE_INT16";
        case DMAP_DATATYPE_UINT16:
            return "DMAP_DATATYPE_UINT16";

        case DMAP_DATATYPE_INT32:
            return "DMAP_DATATYPE_INT32";
        case DMAP_DATATYPE_UINT32:
            return "DMAP_DATATYPE_UINT32";

        case DMAP_DATATYPE_UINT64:
            return "DMAP_DATATYPE_UINT64";
        case DMAP_DATATYPE_INT64:
            return "DMAP_DATATYPE_INT64";

        case DMAP_DATATYPE_STRING:
            return "DMAP_DATATYPE_STRING";
        case DMAP_DATATYPE_TIME:
            return "DMAP_DATATYPE_TIME";
        case DMAP_DATATYPE_VERSION:
            return "DMAP_DATATYPE_VERSION";
        case DMAP_DATATYPE_CONTAINER:
            return "DMAP_DATATYPE_CONTAINER";
    }
    return "UNKNOWN_TYPE!\n";
}

static void dumpContentCodes(dmap_ContentCode_table *table)
{
    dmap_ContentCodeContainer *cur = table->codes;
    if (!TRACE_ON) return;
    while (cur)
    {
        DPRINTF("/* %c%c%c%c */\n", SPLITFOURCC(cur->c.cc_number));
        DPRINTF("%s_add(\"%s\", ", table->prefix, cur->c.cc_name);
        DPRINTF("MAKEFOURCC('%c','%c','%c','%c'),\n", SPLITFOURCC(cur->c.cc_number));
        DPRINTF("         %s);\n", getTypeString(cur->c.cc_type));
        DPRINTF("\n");
        cur = cur->next;
    }
}


/* END FIXME */

dmap_DataTypes dmap_isCC(const dmap_contentCodeFOURCC fourcc,
                         const dmap_ContentCode *code)
{
    if (!code)
    {
        ERR("unknown / unsupported content code\n");
        return DMAP_DATATYPE_INVALID;
    }
    if (code->cc_number == fourcc)
        return code->cc_type;
    return DMAP_DATATYPE_INVALID;
}

/* utility functions */

int dmap_parseContainer(containerHandlerFunc pfnHandler,
                        const int size, const char *buffer,
                        void *scopeData)
{
    int n = 0;
    while (n < size)
    {
        dmap_contentCodeFOURCC code;
        int codesize;

        /* we expect a content code first */
        code = read_fourcc(&buffer[n], sizeof(code));
        n+=4;

        /* then a code size */
        codesize = readBigEndian_INT32(&buffer[n], sizeof(codesize));
        n+=4;

        /* send it off to the container parser */
        pfnHandler(code, codesize, &buffer[n], scopeData);
        n += codesize;
    }
    return 1;
}

/* container parsers */

/* content codes */
static void contentCodesDictionary(dmap_contentCodeFOURCC code,
                                   const int size, const char *buffer,
                                   void *scopeData)
{
    scopeContentCodesDictionary *cd = (scopeContentCodesDictionary*)scopeData;
    if (dmap_isCC(code, dmap_l("contentcodesnumber")) == DMAP_DATATYPE_INT32)
    {
        dmap_contentCodeFOURCC fourcc = read_fourcc(buffer, size);
        cd->c.cc_number = fourcc;
    }
    else if (dmap_isCC(code, dmap_l("contentcodesname")) == DMAP_DATATYPE_STRING)
    {
        cd->c.cc_name = read_string_withalloc(buffer, size); /* freed by parent */
    }
    else if (dmap_isCC(code, dmap_l("contentcodestype")) == DMAP_DATATYPE_INT16)
    {
        DMAP_INT16 type = readBigEndian_INT16(buffer, size);
        cd->c.cc_type = type;
    }
    else
        UNHANDLED_CONTENT_CODE;
}

static void contentCodesResponse(dmap_contentCodeFOURCC code,
                                 const int size, const char *buffer,
                                 void *scopeData)
{
    if (dmap_isCC(code, dmap_l("status")) == DMAP_DATATYPE_INT32)
    {
        DMAP_INT32 status = readBigEndian_INT32(buffer, size);
        if (status != 200)
            FIXME("unknown status code %i\n", status);
    }
    else if (dmap_isCC(code, dmap_l("dictionary")) == DMAP_DATATYPE_CONTAINER)
    {
        scopeContentCodesDictionary sd;
        memset(&sd, 0, sizeof(sd));
        dmap_parseContainer(contentCodesDictionary, size, buffer, (void*)&sd);
        if (sd.c.cc_name)
        {
            if (strncmp("dmap.", sd.c.cc_name, 5) == 0)
            {
                const char *name = sd.c.cc_name + 5;
                dmap_add(name, sd.c.cc_number, sd.c.cc_type);
            }
            else if (strncmp("daap.", sd.c.cc_name, 5) == 0)
            {
                const char *name = sd.c.cc_name + 5;
                daap_add(name, sd.c.cc_number, sd.c.cc_type);
            }
            else if (strncmp("com.", sd.c.cc_name, 4) == 0)
            {
                const char *name = sd.c.cc_name + 4;
                com_add(name, sd.c.cc_number, sd.c.cc_type);
            }
            else
                ERR("unknown class for content code: %s\n", sd.c.cc_name);
            free(sd.c.cc_name);
        }
    }
    else
        UNHANDLED_CONTENT_CODE;
}

/* server info container */
static void serverInfoResponse(dmap_contentCodeFOURCC code,
                               const int size, const char *buffer,
                               void *scopeData)
{
    protoParseResult_serverinfo *sd = (protoParseResult_serverinfo*)scopeData;
    if (dmap_isCC(code, dmap_l("status")) == DMAP_DATATYPE_INT32)
    {
        DMAP_INT32 status = readBigEndian_INT32(buffer, size);
        if (status != 200)
            FIXME("unknown status code %i\n", status);
    }
    else if (dmap_isCC(code, dmap_l("protocolversion")) == DMAP_DATATYPE_VERSION)
    {
        if (sd)
            sd->dmap_version = read_version(buffer, size);
    }
    else if (dmap_isCC(code, daap_l("protocolversion")) == DMAP_DATATYPE_VERSION)
    {
        if (sd)
            sd->daap_version = read_version(buffer, size);
    }
    else if (dmap_isCC(code, dmap_l("itemname")) == DMAP_DATATYPE_STRING)
    {
        if (sd)
            sd->hostname = read_string_withalloc(buffer, size);
    }
    else if (dmap_isCC(code, dmap_l("authenticationmethod")) == DMAP_DATATYPE_INT8)
    {
        if (readBigEndian_INT8(buffer, size))
            TRACE("requires a login\n");
    }
    else if (dmap_isCC(code, dmap_l("loginrequired")) == DMAP_DATATYPE_INT8)
    {
        if (readBigEndian_INT8(buffer, size))
            TRACE("requires a login\n");
    }
    else if (dmap_isCC(code, dmap_l("timeoutinterval")) == DMAP_DATATYPE_INT32)
    {
        TRACE("timeout interval: %i\n", readBigEndian_INT32(buffer, size));
    }
    else if (dmap_isCC(code, dmap_l("supportsautologout")) == DMAP_DATATYPE_INT8)
    {
    }
    else if (dmap_isCC(code, dmap_l("supportsupdate")) == DMAP_DATATYPE_INT8)
    {
    }
    else if (dmap_isCC(code, dmap_l("supportspersistentids")) == DMAP_DATATYPE_INT8)
    {
    }
    else if (dmap_isCC(code, dmap_l("supportsextensions")) == DMAP_DATATYPE_INT8)
    {
    }
    else if (dmap_isCC(code, dmap_l("supportsbrowse")) == DMAP_DATATYPE_INT8)
    {
    }
    else if (dmap_isCC(code, dmap_l("supportsquery")) == DMAP_DATATYPE_INT8)
    {
    }
    else if (dmap_isCC(code, dmap_l("supportsindex")) == DMAP_DATATYPE_INT8)
    {
    }
    else if (dmap_isCC(code, dmap_l("supportsresolve")) == DMAP_DATATYPE_INT8)
    {
    }
    else if (dmap_isCC(code, dmap_l("databasescount")) == DMAP_DATATYPE_INT32)
    {
        if (sd)
            sd->databasescount = readBigEndian_INT32(buffer, size);
    }
    else
        UNHANDLED_CONTENT_CODE;
}

/* login container */
static void loginResponse(dmap_contentCodeFOURCC code,
                          const int size, const char *buffer,
                          void *scopeData)
{
    protoParseResult_login *sd = (protoParseResult_login*)scopeData;
    if (dmap_isCC(code, dmap_l("status")) == DMAP_DATATYPE_INT32)
    {
        DMAP_INT32 status = readBigEndian_INT32(buffer, size);
        if (status != 200)
            FIXME("unknown status code %i\n", status);
    }
    else if (dmap_isCC(code, dmap_l("sessionid")) == DMAP_DATATYPE_INT32)
    {
        sd->sessionid = readBigEndian_INT32(buffer, size);
    }
    else
        UNHANDLED_CONTENT_CODE;
}

/* update container */
static void updateResponse(dmap_contentCodeFOURCC code,
                           const int size, const char *buffer,
                           void *scopeData)
{
    protoParseResult_update *sd = (protoParseResult_update*)scopeData;
    if (dmap_isCC(code, dmap_l("status")) == DMAP_DATATYPE_INT32)
    {
        DMAP_INT32 status = readBigEndian_INT32(buffer, size);
        if (status != 200)
            FIXME("unknown status code %i\n", status);
    }
    else if (dmap_isCC(code, dmap_l("serverrevision")) == DMAP_DATATYPE_INT32)
    {
        sd->serverrevision = readBigEndian_INT32(buffer, size);
    }
    else
        UNHANDLED_CONTENT_CODE;
}

/* main container */
static void toplevelResponse(dmap_contentCodeFOURCC code,
                             const int size, const char *buffer,
                             void *scopeData)
{
    protoParseResult *sd = (protoParseResult*)scopeData;
    if (dmap_isCC(code, dmap_l("serverinforesponse")) == DMAP_DATATYPE_CONTAINER)
    {
        if (sd && sd->expecting == QUERY_SERVERINFORESPONSE)
            dmap_parseContainer(serverInfoResponse, size, buffer, sd);
    }
    else if (dmap_isCC(code, dmap_l("contentcodesresponse")) == DMAP_DATATYPE_CONTAINER)
    {
        dmap_parseContainer(contentCodesResponse, size, buffer, NULL);
#if 1
        dumpContentCodes(&dmap_table);
        dumpContentCodes(&daap_table);
        dumpContentCodes(&com_table);
#endif
    }
    else if (dmap_isCC(code, dmap_l("loginresponse")) == DMAP_DATATYPE_CONTAINER)
    {
        if (sd && sd->expecting == QUERY_LOGINRESPONSE)
            dmap_parseContainer(loginResponse, size, buffer, sd);
    }
    else if (dmap_isCC(code, dmap_l("updateresponse")) == DMAP_DATATYPE_CONTAINER)
    {
        if (sd && sd->expecting == QUERY_UPDATERESPONSE)
            dmap_parseContainer(updateResponse, size, buffer, sd);
    }
    else if (dmap_isCC(code, daap_l("serverdatabases")) == DMAP_DATATYPE_CONTAINER)
    {
        if (sd && sd->expecting == QUERY_GENERICLISTING)
            dmap_parseContainer(preListingContainer, size, buffer, sd);
    }
    else if (dmap_isCC(code, daap_l("databasesongs")) == DMAP_DATATYPE_CONTAINER)
    {
        if (sd && sd->expecting == QUERY_GENERICLISTING)
            dmap_parseContainer(preListingContainer, size, buffer, sd);
    }
    else if (dmap_isCC(code, daap_l("databaseplaylists")) == DMAP_DATATYPE_CONTAINER)
    {
        if (sd && sd->expecting == QUERY_GENERICLISTING)
            dmap_parseContainer(preListingContainer, size, buffer, sd);
    }
    else if (dmap_isCC(code, daap_l("playlistsongs")) == DMAP_DATATYPE_CONTAINER)
    {
        if (sd && sd->expecting == QUERY_GENERICLISTING)
            dmap_parseContainer(preListingContainer, size, buffer, sd);
    }
    else
        UNHANDLED_CONTENT_CODE;
}

/* we load most of the content codes using the /content-codes
 * request. But to get there we must first be able to
 * pass the /server-info response and the /content-codes response
 * so this is the 'boot strapper' for that
 */
static void dmap_ContentCodesBootstrap()
{
    /* common */
    dmap_add("status", MAKEFOURCC('m','s','t','t'),
             DMAP_DATATYPE_INT32);
    dmap_add("dictionary", MAKEFOURCC('m','d','c','l'),
             DMAP_DATATYPE_CONTAINER);

    /* server info */
    dmap_add("serverinforesponse", MAKEFOURCC('m','s','r','v'),
             DMAP_DATATYPE_CONTAINER);
    dmap_add("protocolversion", MAKEFOURCC('m','p','r','o'),
             DMAP_DATATYPE_VERSION);
    daap_add("protocolversion", MAKEFOURCC('a','p','r','o'),
             DMAP_DATATYPE_VERSION);
    dmap_add("itemname", MAKEFOURCC('m','i','n','m'),
             DMAP_DATATYPE_STRING);
    dmap_add("authenticationmethod", MAKEFOURCC('m','s','a','u'),
             DMAP_DATATYPE_INT8);
    dmap_add("loginrequired", MAKEFOURCC('m','s','l','r'),
             DMAP_DATATYPE_INT8);
    dmap_add("timeoutinterval", MAKEFOURCC('m','s','t','m'),
             DMAP_DATATYPE_INT32);
    dmap_add("supportsautologout", MAKEFOURCC('m','s','a','l'),
             DMAP_DATATYPE_INT8);
    dmap_add("supportsupdate", MAKEFOURCC('m','s','u','p'),
             DMAP_DATATYPE_INT8);
    dmap_add("supportspersistentids", MAKEFOURCC('m','s','p','i'),
             DMAP_DATATYPE_INT8);
    dmap_add("supportsextensions", MAKEFOURCC('m','s','e','x'),
             DMAP_DATATYPE_INT8);
    dmap_add("supportsbrowse", MAKEFOURCC('m','s','b','r'),
             DMAP_DATATYPE_INT8);
    dmap_add("supportsquery", MAKEFOURCC('m','s','q','y'),
             DMAP_DATATYPE_INT8);
    dmap_add("supportsindex", MAKEFOURCC('m','s','i','x'),
             DMAP_DATATYPE_INT8);
    dmap_add("supportsresolve", MAKEFOURCC('m','s','r','s'),
             DMAP_DATATYPE_INT8);
    dmap_add("databasescount", MAKEFOURCC('m','s','d','c'),
             DMAP_DATATYPE_INT32);


    dmap_add("contentcodesresponse", MAKEFOURCC('m','c','c','r'),
             DMAP_DATATYPE_CONTAINER);
    dmap_add("contentcodesnumber", MAKEFOURCC('m','c','n','m'),
             DMAP_DATATYPE_INT32);
    dmap_add("contentcodesname", MAKEFOURCC('m','c','n','a'),
             DMAP_DATATYPE_STRING);
    dmap_add("contentcodestype", MAKEFOURCC('m','c','t','y'),
             DMAP_DATATYPE_INT16);
}

/* main function called to parse the thing */

void dmap_init()
{
    if (dmap_initilized) return;
    dmap_ContentCodesBootstrap();
    dmap_initilized = 1;
}

void dmap_deinit()
{
	if (dmap_initilized)
	{
		dmap_ContentCodeContainer *newItem;

		while(dmap_table.codes)
		{
			newItem = dmap_table.codes->next;
			free(dmap_table.codes);
			dmap_table.codes = newItem;
		}
		dmap_initilized = 0;
	}
		
	return;
}

void dmap_parseProtocolData(const int size, const char *buffer, protoParseResult *res)
{
    if (!dmap_initilized)
    {
        ERR("dmap_init must be called first!\n");
        return;
    }
    dmap_parseContainer(toplevelResponse, size, buffer, res);
}


