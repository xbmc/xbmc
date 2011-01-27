/*
 * This file is part of libbluray
 * Copyright (C) 2009-2010  Obliter0n
 * Copyright (C) 2009-2010  John Stebbins
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef BD_FILESYSTEM_H_
#define BD_FILESYSTEM_H_

#include <stdint.h>

/*
 * file access
 */

typedef struct bd_file_s BD_FILE_H;
struct bd_file_s
{
    void* internal;
    void (*close)(BD_FILE_H *file);
    int64_t (*seek)(BD_FILE_H *file, int64_t offset, int32_t origin);
    int64_t (*tell)(BD_FILE_H *file);
    int (*eof)(BD_FILE_H *file);
    int64_t (*read)(BD_FILE_H *file, uint8_t *buf, int64_t size);
    int64_t (*write)(BD_FILE_H *file, const uint8_t *buf, int64_t size);
};

/*
 * directory access
 */

// Our dirent struct only contains the parts we care about.
typedef struct
{
    char    d_name[256];
} BD_DIRENT;

typedef struct bd_dir_s BD_DIR_H;
struct bd_dir_s
{
    void* internal;
    void (*close)(BD_DIR_H *dir);
    int (*read)(BD_DIR_H *dir, BD_DIRENT *entry);
};

typedef BD_FILE_H* (*BD_FILE_OPEN)(const char* filename, const char *mode);
typedef BD_DIR_H* (*BD_DIR_OPEN) (const char* dirname);

/**
 *
 *  Register function pointer that will be used to open a file
 *
 * @param p function pointer
 * @return previous function pointer registered
 */
BD_FILE_OPEN bd_register_file(BD_FILE_OPEN p);

/**
 *
 *  Register function pointer that will be used to open a directory
 *
 * @param p function pointer
 * @return previous function pointer registered
 */
BD_DIR_OPEN bd_register_dir(BD_DIR_OPEN p);


#endif /* BD_FILESYSTEM_H_ */
