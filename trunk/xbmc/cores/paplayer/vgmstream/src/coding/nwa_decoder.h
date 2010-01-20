/* originally from nwatowav.cc 2007.7.28 version, which read: */
/*
 * Copyright 2001-2007  jagarl / Kazunori Ueno <jagarl@creator.club.ne.jp>
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * このプログラムの作者は jagarl です。
 *
 * このプログラム、及びコンパイルによって生成したバイナリは
 * プログラムを変更する、しないにかかわらず再配布可能です。
 * その際、上記 Copyright 表示を保持するなどの条件は課しま
 * せん。対応が面倒なのでバグ報告を除き、メールで連絡をする
 * などの必要もありません。ソースの一部を流用することを含め、
 * ご自由にお使いください。
 *
 * THIS SOFTWARE IS PROVIDED BY KAZUNORI 'jagarl' UENO ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL KAZUNORI UENO BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 */

#ifndef _NWA_DECODER_H
#define _NWA_DECODER_H

#include "../streamfile.h"

typedef struct NWAData_s
{
    int channels;
    int bps;						/* bits per sample */
    int freq;						/* samples per second */
    int complevel;				/* compression level */
    int blocks;					/* block count */
    int datasize;					/* all data size */
    int compdatasize;				/* compressed data size */
    int samplecount;				/* all samples */
    int blocksize;				/* samples per block */
    int restsize;					/* samples of the last block */

    int curblock;
    off_t *offsets;

    STREAMFILE *file;

    /* temporarily store samples */
    sample *buffer;
    sample *buffer_readpos;
    int samples_in_buffer;
} NWAData;

NWAData *open_nwa(STREAMFILE *streamFile, const char *filename);
void close_nwa(NWAData *nwa);
void reset_nwa(NWAData *nwa);
void seek_nwa(NWAData *nwa, int32_t seekpos);

#endif
