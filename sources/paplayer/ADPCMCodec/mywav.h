/*
  MyWAV 0.1.1
  by Luigi Auriemma
  e-mail: aluigi@autistici.org
  web:    aluigi.org

    Copyright 2005,2006 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl.txt
*/

#include <string.h>

//#include <stdint.h>
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;

#include <stdio.h>

/*
the functions return ever 0 if success, other values (-1) if error
note that these functions have been written with compatibility in mind
so don't worry if you see useless instructions
*/



typedef struct {
    uint8_t     id[4];
    uint32_t    size;
} mywav_chunk;

typedef struct {
    int16_t     wFormatTag;
    uint16_t    wChannels;
    uint32_t    dwSamplesPerSec;
    uint32_t    dwAvgBytesPerSec;
    uint16_t    wBlockAlign;
    uint16_t    wBitsPerSample;
} mywav_fmtchunk;



    /* FILE WRITING */

    // 8 bit
int mywav_fwi08(FILE *fd, int num) {
    if(fputc((num      ) & 0xff, fd) < 0) return(-1);
    return(0);
}



    // 16 bit
int mywav_fwi16(FILE *fd, int num) {
    if(fputc((num      ) & 0xff, fd) < 0) return(-1);
    if(fputc((num >>  8) & 0xff, fd) < 0) return(-1);
    return(0);
}



    // 32 bit
int mywav_fwi32(FILE *fd, int num) {
    if(fputc((num      ) & 0xff, fd) < 0) return(-1);
    if(fputc((num >>  8) & 0xff, fd) < 0) return(-1);
    if(fputc((num >> 16) & 0xff, fd) < 0) return(-1);
    if(fputc((num >> 24) & 0xff, fd) < 0) return(-1);
    return(0);
}



    // data
int mywav_fwmem(FILE *fd, uint8_t *mem, int size) {
    if(size) {
        if(fwrite(mem, size, 1, fd) != 1) return(-1);
    }
    return(0);
}



    // chunk
int mywav_fwchunk(FILE *fd, mywav_chunk *chunk) {
    if(mywav_fwmem(fd, chunk->id, 4)) return(-1);
    if(mywav_fwi32(fd, chunk->size))  return(-1);
    return(0);
}



  // fmtchunk
int mywav_fwfmtchunk(FILE *fd, mywav_fmtchunk *fmtchunk) {
    if(mywav_fwi16(fd, fmtchunk->wFormatTag))       return(-1);
    if(mywav_fwi16(fd, fmtchunk->wChannels))        return(-1);
    if(mywav_fwi32(fd, fmtchunk->dwSamplesPerSec))  return(-1);
    if(mywav_fwi32(fd, fmtchunk->dwAvgBytesPerSec)) return(-1);
    if(mywav_fwi16(fd, fmtchunk->wBlockAlign))      return(-1);
    if(mywav_fwi16(fd, fmtchunk->wBitsPerSample))   return(-1);
    return(0);
}



    /* FILE READING */

    // 8 bit
int mywav_fri08(FILE *fd, uint8_t *num) {
    if(fread(num, 1, 1, fd) != 1)  return(-1);
    return(0);
}



    // 16 bit
int mywav_fri16(FILE *fd, uint16_t *num) {
    uint16_t    ret;
    uint8_t     tmp;

    if(fread(&tmp, 1, 1, fd) != 1) return(-1);  ret = tmp;
    if(fread(&tmp, 1, 1, fd) != 1) return(-1);  ret |= (tmp << 8);
    *num = ret;
    return(0);
}



    // 32 bit
int mywav_fri32(FILE *fd, uint32_t *num) {
    uint32_t    ret;
    uint8_t     tmp;

    if(fread(&tmp, 1, 1, fd) != 1) return(-1);  ret = tmp;
    if(fread(&tmp, 1, 1, fd) != 1) return(-1);  ret |= (tmp << 8);
    if(fread(&tmp, 1, 1, fd) != 1) return(-1);  ret |= (tmp << 16);
    if(fread(&tmp, 1, 1, fd) != 1) return(-1);  ret |= (tmp << 24);
    *num = ret;
    return(0);
}



    // data
int mywav_frmem(FILE *fd, uint8_t *mem, int size) {
    if(size) {
        if(fread(mem, size, 1, fd) != 1) return(-1);
    }
    return(0);
}



    // chunk
int mywav_frchunk(FILE *fd, mywav_chunk *chunk) {
    if(mywav_frmem(fd, (uint8_t *)&chunk->id, 4)) return(-1);
    if(mywav_fri32(fd, (uint32_t *)&chunk->size))  return(-1);
    return(0);
}



  // fmtchunk
int mywav_frfmtchunk(FILE *fd, mywav_fmtchunk *fmtchunk) {
    if(mywav_fri16(fd, (uint16_t *)&fmtchunk->wFormatTag))       return(-1);
    if(mywav_fri16(fd, (uint16_t *)&fmtchunk->wChannels))        return(-1);
    if(mywav_fri32(fd, (uint32_t *)&fmtchunk->dwSamplesPerSec))  return(-1);
    if(mywav_fri32(fd, (uint32_t *)&fmtchunk->dwAvgBytesPerSec)) return(-1);
    if(mywav_fri16(fd, (uint16_t *)&fmtchunk->wBlockAlign))      return(-1);
    if(mywav_fri16(fd, (uint16_t *)&fmtchunk->wBitsPerSample))   return(-1);
    return(0);
}


    /* MYWAV MAIN FUNCTIONS */

int mywav_seekchunk(FILE *fd, uint8_t *find) {
    mywav_chunk chunk;

    if(fseek(fd, sizeof(mywav_chunk) + 4, SEEK_SET) < 0) return(-1);

    while(!mywav_frchunk(fd, &chunk)) {
        if(!memcmp(chunk.id, find, 4)) return(chunk.size);
        if(fseek(fd, chunk.size, SEEK_CUR) < 0) break;
    }
    return(-1);
}



int mywav_data(FILE *fd, mywav_fmtchunk *fmt) {
    mywav_chunk chunk;
    uint8_t     type[4];

    if(mywav_frchunk(fd, &chunk)   < 0) return(-1);
    if(mywav_frmem(fd, type, 4)    < 0) return(-1);
    if(memcmp(type, "WAVE", 4))         return(-1);

    if(mywav_seekchunk(fd, (uint8_t*)"fmt ") < 0) return(-1);
    if(mywav_frfmtchunk(fd, fmt)   < 0) return(-1);

    return(mywav_seekchunk(fd, (uint8_t*)"data"));
}



int mywav_writehead(FILE *fd, mywav_fmtchunk *fmt, uint32_t data_size, uint8_t *more, int morelen) {
    mywav_chunk chunk;

    memcpy(chunk.id, "RIFF", 4);
    chunk.size =
        4                      +
        sizeof(mywav_chunk)    +
        sizeof(mywav_fmtchunk) +
        morelen                +
        sizeof(mywav_chunk)    +
        data_size;

    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    if(mywav_fwmem(fd, (uint8_t*)"WAVE", 4)     < 0) return(-1);

    memcpy(chunk.id, "fmt ", 4);
    chunk.size = sizeof(mywav_fmtchunk) + morelen;
    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    if(mywav_fwfmtchunk(fd, fmt)      < 0) return(-1);
    if(mywav_fwmem(fd, more, morelen) < 0) return(-1);

    memcpy(chunk.id, "data", 4);
    chunk.size = data_size;
    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    return(0);
}

