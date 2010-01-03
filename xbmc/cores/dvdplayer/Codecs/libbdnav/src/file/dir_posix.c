#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <dirent.h>

#include "dir.h"
#include "../util/macro.h"
#include "../util/logging.h"

DIR_H *dir_open_posix(const char* dirname);
void dir_close_posix(DIR_H *dir);
int dir_read_posix(DIR_H *dir, DIRENT *ent);

void dir_close_posix(DIR_H *dir)
{
    if (dir) {
        closedir((DIR *)dir->internal);

        DEBUG(DBG_DIR, "Closed POSIX dir (0x%08x)\n", dir);

        X_FREE(dir);
    }
}

int dir_read_posix(DIR_H *dir, DIRENT *entry)
{
    struct dirent e, *p_e;
	int result;

    result = readdir_r((DIR*)dir->internal, &e, &p_e);
    if (result) {
        return -result;
    } else if (p_e == NULL) {
        return 1;
    }
    strncpy(entry->d_name, e.d_name, 256);
    return 0;
}

DIR_H *dir_open_posix(const char* dirname)
{
    DIR *dp = NULL;
    DIR_H *dir = malloc(sizeof(DIR_H));

    DEBUG(DBG_CONFIGFILE, "Opening POSIX dir %s... (0x%08x)\n", dirname, dir);
    dir->close = dir_close_posix;
    dir->read = dir_read_posix;

    if ((dp = opendir(dirname))) {
        dir->internal = dp;

        return dir;
    }

    DEBUG(DBG_DIR, "Error opening dir! (0x%08x)\n", dir);

    X_FREE(dir);

    return NULL;
}
