/* Copyright (c) David Hammerton 2003
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <memory.h>

#include "debug.h"

#define DEFAULT_DEBUG_CHANNEL "debug"

static int tracesEnabled = 0;
static int errEnabled = 1;
static int fixmeEnabled = 1;

/* returns -1 on failure, 0 on success */
int daap_debug_init(const char *const debug_init_string)
{
    const char *p = debug_init_string;

    while (*p)
    {
        char *class_or_channel;
        int class_or_channel_size;
        const char *end;
        int the_switch;

        switch (*p)
        {
        case '+': the_switch = 1;
                  break;
        case '-': the_switch = 0;
                  break;
        default:
                  goto err;
        }

        p++;
        if (!p) goto err;
        end = strchr(p, ',');
        if (end) end--;
        else end = p + strlen(p);

        class_or_channel_size = end - p + 1;

        class_or_channel = (char*)malloc(class_or_channel_size + 1);
        strncpy(class_or_channel, p, class_or_channel_size);
        class_or_channel[class_or_channel_size] = 0;

        /* now check if it is a class */
        if (strcmp(class_or_channel, "err") == 0)
            errEnabled = the_switch;
        else if (strcmp(class_or_channel, "fixme") == 0)
            fixmeEnabled = the_switch;
        else if (strcmp(class_or_channel, "trace") == 0)
            tracesEnabled = the_switch;
        else /* check channels */
        {
            FIXME("sorry, but currently you can only toggle debug classes. Not switching '%s'.\n",
                  class_or_channel);
        }

        p = end + 1;
        if (*p == ',') p++;
    };
    return 0;
err:
    return -1;
}

static char *get_debug_class_str(enum __DEBUG_CLASS debug_class)
{
    switch (debug_class)
    {
        case __DEBUG_TRACE: return "trace";
        case __DEBUG_ERR: return "err";
        case __DEBUG_FIXME: return "fixme";
        default: return "";
    }
}

/* FIXME: don't care about channel for now */
int debug_get_debugging(enum __DEBUG_CLASS debug_class, const char *debug_channel)
{
    switch (debug_class)
    {
        case __DEBUG_TRACE: return tracesEnabled;
        case __DEBUG_ERR: return errEnabled;
        case __DEBUG_FIXME: return fixmeEnabled;
        default: return 0;
    }
}

int debug_log(enum __DEBUG_CLASS debug_class, const char *module,
              const char *function,
              const int line, const char *format, ...)
{
    va_list valist;
    int ret = 0;
    char *debug_class_str = get_debug_class_str(debug_class);

    va_start(valist, format);

    ret += fprintf(stderr, "%s:%s:", debug_class_str, module);
    ret += fprintf(stderr, "%s:%i ", function, line);

    ret += vfprintf(stderr, format, valist);

    va_end(valist);

    return ret;
}

int debug_printf(const char *format, ...)
{
    va_list valist;
    int ret = 0;

    va_start(valist, format);

    ret += vfprintf(stderr, format, valist);

    va_end(valist);

    return ret;
}

void debug_hexdump(void *data, unsigned long len)
{
    unsigned int i;
    char *cdata = (char *)data;
    char textout[16];

    for (i = 0; i < len; i++)
    {
        if ((i % 8) == 0 && i) fprintf(stderr, "  ");
        if ((i % 16) == 0 && i) fprintf(stderr, "     '%.8s' '%.8s'\n", &textout[0], &textout[8]);
        textout[i % 16] = (cdata[i] && isprint(cdata[i])) ? cdata[i] : '.';
        fprintf(stderr, "%02hhx ", cdata[i]);
    }
    if (i % 16)
    {
        unsigned int j;
        for (j = 0; j < (16 - (i % 16)); j++)
        {
            if (((16 - (i % 16)) - j) == 8) fprintf(stderr, "  ");
            fprintf(stderr, ".. ");
        }
        fprintf(stderr, "       '");
        for (j = 0; j < (i % 16); j++)
        {
            fprintf(stderr, "%c", textout[j]);
            if (j == 8) fprintf(stderr, "' '");
        }
        fprintf(stderr, "'\n");
    }
    fprintf(stderr, "\n");
}

