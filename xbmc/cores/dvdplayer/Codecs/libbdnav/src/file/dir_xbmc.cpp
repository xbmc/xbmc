
#define _LINUX

#include "FileSystem/Directory.h"
#include "FileItem.h"
#include "Util.h"

using namespace XFILE;

extern "C"
{
#include "dir.h"

struct xbmcdir {
    CFileItemList* items;
    int position;
};

DIR_H *dir_open_xbmc(const char* dirname);
void dir_close_xbmc(DIR_H *dir);
int dir_read_xbmc(DIR_H *dir, DIRENT *ent);

void dir_close_xbmc(DIR_H *dir)
{
    if (dir) {
        delete (CFileItemList*)(((xbmcdir*)dir->internal)->items);
        free(dir->internal);
    }

    free(dir);
}

int dir_read_xbmc(DIR_H *dir, DIRENT *entry)
{
    xbmcdir* pDir = (xbmcdir*)dir->internal;

    if (pDir->position == pDir->items->Size()) {
        return 1;
    }
    strncpy(entry->d_name, CUtil::GetFileName(pDir->items->Get(pDir->position++)->m_strPath).c_str(), 256);
    return 0;
}

DIR_H *dir_open_xbmc(const char* dirname)
{
    DIR_H *dir = (DIR_H*)malloc(sizeof(DIR_H));

    dir->close = dir_close_xbmc;
    dir->read = dir_read_xbmc;

    dir->internal = malloc(sizeof(xbmcdir));
    ((xbmcdir*)dir->internal)->items = new CFileItemList;
    ((xbmcdir*)dir->internal)->position = 0;

    if (CDirectory::GetDirectory(dirname,*((xbmcdir*)dir->internal)->items))
        return dir;

    delete ((xbmcdir*)dir->internal)->items;
    free(dir->internal);
    free(dir);

    return NULL;
}
}
