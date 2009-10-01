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

#include <stdlib.h>
#include "nwa_decoder.h"

/* can serve up 8 bits at a time */
static int
getbits (STREAMFILE *file, off_t *offset, int *shift, int bits)
{
	int ret;
    if (*shift > 8)
    {
        ++*offset;
        *shift -= 8;
    }
    ret = read_16bitLE(*offset,file) >> *shift;
    *shift += bits;
    return ret & ((1 << bits) - 1);	/* mask */
}

NWAData *
open_nwa (STREAMFILE * streamFile, const char *filename)
{
    int i;
    NWAData * const nwa = malloc(sizeof(NWAData));
    if (!nwa) goto fail;

    nwa->channels = read_16bitLE(0x00,streamFile);
    nwa->bps = read_16bitLE(0x02,streamFile);
    nwa->freq = read_32bitLE(0x04,streamFile);
    nwa->complevel = read_32bitLE(0x08,streamFile);
    nwa->blocks = read_32bitLE(0x10,streamFile);
    nwa->datasize = read_32bitLE(0x14,streamFile);
    nwa->compdatasize = read_32bitLE(0x18,streamFile);
    nwa->samplecount = read_32bitLE(0x1c,streamFile);
    nwa->blocksize = read_32bitLE(0x20,streamFile);
    nwa->restsize = read_32bitLE(0x24,streamFile);
    nwa->offsets = NULL;
    nwa->buffer = NULL;
    nwa->buffer_readpos = NULL;
    nwa->file = NULL;

    /* PCM not handled here */
    if (nwa->complevel < 0 || nwa->complevel > 5) goto fail;

    if (nwa->channels != 1 && nwa->channels != 2) goto fail;

    if (nwa->bps != 8 && nwa->bps != 16) goto fail;

    if (nwa->blocks <= 0) goto fail;

    if (nwa->compdatasize == 0
            || get_streamfile_size(streamFile) != nwa->compdatasize) goto fail;

    if (nwa->datasize != nwa->samplecount * (nwa->bps/8)) goto fail;

    if (nwa->samplecount !=
            (nwa->blocks-1) * nwa->blocksize + nwa->restsize) goto fail;

    nwa->offsets = malloc(sizeof(off_t)*nwa->blocks);
    if (!nwa->offsets) goto fail;

    for (i = 0; i < nwa->blocks; i++)
    {
        int32_t o = read_32bitLE(0x2c+i*4,streamFile);
        if (o < 0) goto fail;
        nwa->offsets[i] = o;
    }

    if (nwa->offsets[nwa->blocks-1] >= nwa->compdatasize) goto fail;

    if (nwa->restsize > nwa->blocksize) nwa->buffer =
        malloc(sizeof(sample)*nwa->restsize);
    else nwa->buffer =
        malloc(sizeof(sample)*nwa->blocksize);
    if (nwa->buffer == NULL) goto fail;

    nwa->buffer_readpos = nwa->buffer;

    nwa->samples_in_buffer = 0;

    nwa->curblock = 0;

    /* if we got this far, it's probably safe */
    nwa->file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
    if (!nwa->file) goto fail;

    return nwa;
fail:
    if (nwa)
    {
        close_nwa(nwa);
        free(nwa);
    }

    return NULL;
}

void
close_nwa(NWAData * nwa)
{
    if (!nwa) return;

    if (nwa->offsets)
        free (nwa->offsets);
    nwa->offsets = NULL;
    if (nwa->buffer)
        free (nwa->buffer);
    nwa->buffer = NULL;
    if (nwa->file)
        close_streamfile (nwa->file);
    nwa->file = NULL;
    free(nwa);
}

void
reset_nwa(NWAData *nwa)
{
    nwa->curblock = 0;
    nwa->buffer_readpos = nwa->buffer;
    nwa->samples_in_buffer = 0;
}

static int use_runlength(NWAData *nwa)
{
    if (nwa->channels == 2 && nwa->bps == 16 && nwa->complevel == 2)
    {
        /* sw2 */
        return 0;
    }
    if (nwa->complevel == 5)
    {
        if (nwa->channels == 2) return 0; /* BGM*.nwa in Little Busters! */
        return 1; /* Tomoyo After (.nwk koe file) */
    }
    return 0;
}

static void
nwa_decode_block(NWAData *nwa)
{
    /* 今回読み込む／デコードするデータの大きさを得る */
    int curblocksize, curcompsize;
    if (nwa->curblock != nwa->blocks - 1)
    {
        curblocksize = nwa->blocksize * (nwa->bps / 8);
        curcompsize = nwa->offsets[nwa->curblock + 1] - nwa->offsets[nwa->curblock];
    }
    else
    {
        curblocksize = nwa->restsize * (nwa->bps / 8);
        curcompsize = nwa->blocksize * (nwa->bps / 8) * 2;
    }

    nwa->samples_in_buffer = 0;
    nwa->buffer_readpos = nwa->buffer;

    {
        sample d[2];
        int i;
        int shift = 0;
        off_t offset = nwa->offsets[nwa->curblock];
        int dsize = curblocksize / (nwa->bps / 8);
        int flip_flag = 0;			/* stereo 用 */
        int runlength = 0;

        /* read initial sample value */
        for (i=0;i<nwa->channels;i++)
        {
            if (nwa->bps == 8) { d[i] = read_8bit(offset,nwa->file); }
            else							/* bps == 16 */
            {
                d[i] = read_16bitLE(offset,nwa->file);
                offset += 2;
            }
        }

        for (i = 0; i < dsize; i++)
        {
            if (runlength == 0)
            {						/* コピーループ中でないならデータ読み込み */
                int type = getbits(nwa->file, &offset, &shift, 3);
                /* type により分岐：0, 1-6, 7 */
                if (type == 7)
                {
                    /* 7 : 大きな差分 */
                    /* RunLength() 有効時（CompLevel==5, 音声ファイル) では無効 */
                    if (getbits(nwa->file, &offset, &shift, 1) == 1)
                    {
                        d[flip_flag] = 0;	/* 未使用 */
                    }
                    else
                    {
                        int BITS, SHIFT;
                        if (nwa->complevel >= 3)
                        {
                            BITS = 8;
                            SHIFT = 9;
                        }
                        else
                        {
                            BITS = 8 - nwa->complevel;
                            SHIFT = 2 + 7 + nwa->complevel;
                        }
						{
							const int MASK1 = (1 << (BITS - 1));
							const int MASK2 = (1 << (BITS - 1)) - 1;
							int b = getbits(nwa->file, &offset, &shift, BITS);
							if (b & MASK1)
								d[flip_flag] -= (b & MASK2) << SHIFT;
							else
								d[flip_flag] += (b & MASK2) << SHIFT;
						}
                    }
                }
                else if (type != 0)
                {
                    /* 1-6 : 通常の差分 */
                    int BITS, SHIFT;
                    if (nwa->complevel >= 3)
                    {
                        BITS = nwa->complevel + 3;
                        SHIFT = 1 + type;
                    }
                    else
                    {
                        BITS = 5 - nwa->complevel;
                        SHIFT = 2 + type + nwa->complevel;
                    }
					{
						const int MASK1 = (1 << (BITS - 1));
						const int MASK2 = (1 << (BITS - 1)) - 1;
						int b = getbits(nwa->file, &offset, &shift, BITS);
						if (b & MASK1)
							d[flip_flag] -= (b & MASK2) << SHIFT;
						else
	                        d[flip_flag] += (b & MASK2) << SHIFT;
					}
                }
                else
                {					/* type == 0 */
                    /* ランレングス圧縮なしの場合はなにもしない */
                    if (use_runlength(nwa))
                    {
                        /* ランレングス圧縮ありの場合 */
                        runlength = getbits(nwa->file, &offset, &shift, 1);
                        if (runlength == 1)
                        {
                            runlength = getbits(nwa->file, &offset, &shift, 2);
                            if (runlength == 3)
                            {
                                runlength = getbits(nwa->file, &offset, &shift, 8);
                            }
                        }
                    }
                }
            }
            else
            {
                runlength--;
            }
            if (nwa->bps == 8)
            {
                nwa->buffer[i] = d[flip_flag]*0x100;
            }
            else
            {
                nwa->buffer[i] = d[flip_flag];
            }
            nwa->samples_in_buffer++;
            if (nwa->channels == 2)
                flip_flag ^= 1;			/* channel 切り替え */
        }
    }

    nwa->curblock++;
    return;
}

void
seek_nwa(NWAData *nwa, int32_t seekpos)
{
    int dest_block = seekpos/(nwa->blocksize/nwa->channels);
    int32_t remainder = seekpos%(nwa->blocksize/nwa->channels);

    nwa->curblock = dest_block;

    nwa_decode_block(nwa);

    nwa->buffer_readpos = nwa->buffer + remainder*nwa->channels;
    nwa->samples_in_buffer -= remainder*nwa->channels;
}

/* interface to vgmstream */
void
decode_nwa(NWAData *nwa, sample *outbuf,
        int32_t samples_to_do)
{
    while (samples_to_do > 0)
    {
        int32_t samples_to_read;

        if (nwa->samples_in_buffer > 0)
        {
            samples_to_read = nwa->samples_in_buffer/nwa->channels;
            if (samples_to_read > samples_to_do)
                samples_to_read = samples_to_do;

            memcpy(outbuf,nwa->buffer_readpos,
                    sizeof(sample)*samples_to_read*nwa->channels);

            nwa->buffer_readpos += samples_to_read*nwa->channels;
            nwa->samples_in_buffer -= samples_to_read*nwa->channels;
            outbuf += samples_to_read*nwa->channels;
            samples_to_do -= samples_to_read;
        }
        else
        {
            nwa_decode_block(nwa);
        }
    }
}
