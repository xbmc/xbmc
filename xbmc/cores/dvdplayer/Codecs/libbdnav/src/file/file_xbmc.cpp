#include "FileSystem/File.h"

extern "C"
{
#include "file.h"

using namespace XFILE;

FILE_H *file_open_xbmc(const char* filename, const char *mode);
void file_close_xbmc(FILE_H *file);
int64_t file_seek_xbmc(FILE_H *file, int64_t offset, int32_t origin);
int64_t file_tell_xbmc(FILE_H *file);
int file_eof_xbmc(FILE_H *file);
int file_read_xbmc(FILE_H *file, uint8_t *buf, int64_t size);
int file_write_xbmc(FILE_H *file, uint8_t *buf, int64_t size);

void file_close_xbmc(FILE_H *file)
{
    if (file) {
        delete (CFile*)file->internal;
        free(file);
    }
}

int64_t file_seek_xbmc(FILE_H *file, int64_t offset, int32_t origin)
{
    return ((CFile*)file->internal)->Seek(offset, origin);
}

int64_t file_tell_xbmc(FILE_H *file)
{
    return ((CFile *)file->internal)->GetPosition();
}

int file_eof_xbmc(FILE_H *file)
{
    CFile* pFile = (CFile*)file->internal;
    return pFile->GetPosition()==pFile->GetLength();
}

int file_read_xbmc(FILE_H *file, uint8_t *buf, int64_t size)
{
    return ((CFile*)file->internal)->Read(buf,size);
}

int file_write_xbmc(FILE_H *file, uint8_t *buf, int64_t size)
{
    return ((CFile*)file->internal)->Write(buf,size);
}

FILE_H *file_open_xbmc(const char* filename, const char *mode)
{
    FILE_H *file = (FILE_H*)malloc(sizeof(FILE_H));

    file->close = file_close_xbmc;
    file->seek = file_seek_xbmc;
    file->read = file_read_xbmc;
    file->write = file_write_xbmc;
    file->tell = file_tell_xbmc;
    file->eof = file_eof_xbmc;

    file->internal = (void*)new CFile;
    if (file->internal && ((CFile*)file->internal)->Open(filename)) {
        return file;
    }

    delete (CFile*)file->internal;
    free(file);

    return NULL;
}
}
