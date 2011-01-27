/* list of known contentcodes
 *
 * Copyright (c) David Hammerton 2003
 *
 * FIXME: needs to be made per connection.. as some hosts may have
 *        different content codes.
 *
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

#ifndef _DAAP_CONTENTCODES_H
#define _DAAP_CONTENTCODES_H

#include "daap.h"

/* FIXME: replace this with a hashed dictionary, rather
 * than this linked list. its slow
 */

typedef struct dmap_ContentCodeContainerTAG dmap_ContentCodeContainer;
struct dmap_ContentCodeContainerTAG
{
    dmap_ContentCode c;
    dmap_ContentCodeContainer *next;
};

typedef struct
{
    char *prefix;
    dmap_ContentCodeContainer *codes;
} dmap_ContentCode_table;

const dmap_ContentCode *dmap_lookupCode(const dmap_ContentCode_table *table,
                                        const char *name);
const dmap_ContentCode *dmap_lookupCodeFromFOURCC(const dmap_ContentCode_table *table,
                                                  const dmap_contentCodeFOURCC fourcc);
/* add actually performs a duplicate test, also.
 * if the duplicate test matches, add will print an ERR if the records arn't
 * the same
 */
void dmap_addCode(dmap_ContentCode_table *table, const char *name,
                  const dmap_contentCodeFOURCC code, const dmap_DataTypes type);


extern dmap_ContentCode_table dmap_table;
extern dmap_ContentCode_table daap_table;
extern dmap_ContentCode_table com_table;

#define dmap_l(str) dmap_lookupCode(&dmap_table, str)
#define dmap_add(str, code, type) dmap_addCode(&dmap_table, str, code, type)

#define daap_l(str) dmap_lookupCode(&daap_table, str)
#define daap_add(str, code, type) dmap_addCode(&daap_table, str, code, type)

#define com_l(str) dmap_lookupCode(&com_table, str)
#define com_add(str, code, type) dmap_addCode(&com_table, str, code, type)

dmap_DataTypes dmap_isCC(const dmap_contentCodeFOURCC fourcc,
                         const dmap_ContentCode *code);


typedef void (*containerHandlerFunc)(dmap_contentCodeFOURCC code,
                                     const int size,
                                     const char *buffer,
                                     void *scopeData);
int dmap_parseContainer(containerHandlerFunc pfnHandler,
                        const int size, const char *buffer,
                        void *scopeData);

/* private structures, get passed around various container parses in daap.c */
typedef struct
{
    dmap_ContentCode c;
} scopeContentCodesDictionary;

#endif /* _DAAP_CONTENTCODES_H */

