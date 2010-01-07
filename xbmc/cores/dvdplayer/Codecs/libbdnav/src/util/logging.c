
#include <stdlib.h>

#include "logging.h"

char out[512];
debug_mask_t debug_mask = 0;
static int debug_init = 0;


char *print_hex(uint8_t *buf, int count)
{
    memset(out, 0, count);

    int zz;
    for(zz = 0; zz < count; zz++) {
        sprintf(out + (zz * 2), "%02X", buf[zz]);
    }

    return out;
}

void debug(char *file, int line, uint32_t mask, const char *format, ...)
{
    char *env;

    // Only call getenv() once.
    if (!debug_init) {
        debug_init = 1;

        if ((env = getenv("BD_DEBUG_MASK"))) {
            debug_mask = atoi(env);
        } else {
            debug_mask = 0xffff;
        }
    }

    if (mask & debug_mask) {
        char buffer[512];
        va_list args;

        va_start(args, format);
        vsprintf(buffer, format, args);
        va_end(args);

        fprintf(stderr, "%s:%d: %s", file, line, buffer);
    }
}
