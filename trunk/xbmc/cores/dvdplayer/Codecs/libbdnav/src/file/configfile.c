#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "configfile.h"
#include "file.h"
#include "../util/macro.h"
#include "../util/logging.h"

uint8_t *configfile_record(CONFIGFILE *kf, enum configfile_types type, uint16_t *entries, size_t *entry_len)
{
    size_t pos = 0, len = 0;

    while (pos + 4 <= kf->size) {
        len = MKINT_BE24(kf->buf + pos + 1);

        if (entries) {
            *entries = MKINT_BE16(kf->buf + pos + 4);
        }

        if (entry_len) {
            *entry_len = MKINT_BE32(kf->buf + pos + 6);
        }

        if (kf->buf[pos] == type) {
            DEBUG(DBG_CONFIGFILE, "Retrieved CONFIGFILE record 0x%02x (0x%08x)\n", type, kf->buf + pos + 10);

            return kf->buf + pos + 10;  // only return ptr to first byte of entry
        }

        pos += len;
    }

    return NULL;
}

CONFIGFILE *configfile_open(const char *path)
{
    FILE_H *fp = NULL;
    CONFIGFILE *kf = malloc(sizeof(CONFIGFILE));

    DEBUG(DBG_CONFIGFILE, "Opening configfile %s... (0x%08x)\n", path, kf);

    if ((fp = file_open(path, "rb"))) {
        file_seek(fp, 0, SEEK_END);
        kf->size = file_tell(fp);
        file_seek(fp, 0, SEEK_SET);

        kf->buf = malloc(kf->size);

        file_read(fp, kf->buf, kf->size);

        file_close(fp);

        return kf;
    }

    return NULL;
}

void configfile_close(CONFIGFILE *kf)
{
    DEBUG(DBG_CONFIGFILE, "configfile closed (0x%08x)\n", kf);

    X_FREE(kf->buf);
    X_FREE(kf);
}
