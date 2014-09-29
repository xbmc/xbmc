/*-
 * Copyright 2003-2005 Colin Percival
 * Copyright 2012 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include "bspatch.h"

int64_t bspatch_offtin(uint8_t* buf)
{
  int64_t y;

  y = buf[7] & 0x7F;
  y = y * 256;
  y += buf[6];
  y = y * 256;
  y += buf[5];
  y = y * 256;
  y += buf[4];
  y = y * 256;
  y += buf[3];
  y = y * 256;
  y += buf[2];
  y = y * 256;
  y += buf[1];
  y = y * 256;
  y += buf[0];

  if (buf[7] & 0x80)
    y = -y;

  return y;
}

int bspatch(const uint8_t* old, int64_t oldsize, uint8_t* newdata, int64_t newsize,
            struct bspatch_stream* stream)
{
  uint8_t buf[8];
  int64_t oldpos, newpos;
  int64_t ctrl[3];
  int64_t i;

  oldpos = 0;
  newpos = 0;
  while (newpos < newsize)
  {
    /* Read control data */
    for (i = 0; i <= 2; i++)
    {
      if (stream->read(stream, buf, 8))
        return -1;
      ctrl[i] = bspatch_offtin(buf);
    };

    /* Sanity-check */
    if (newpos + ctrl[0] > newsize)
      return -1;

    /* Read diff string */
    if (stream->read(stream, newdata + newpos, ctrl[0]))
      return -1;

    /* Add old data to diff string */
    for (i = 0; i < ctrl[0]; i++)
      if ((oldpos + i >= 0) && (oldpos + i < oldsize))
        newdata[newpos + i] += old[oldpos + i];

    /* Adjust pointers */
    newpos += ctrl[0];
    oldpos += ctrl[0];

    /* Sanity-check */
    if (newpos + ctrl[1] > newsize)
      return -1;

    /* Read extra string */
    if (stream->read(stream, newdata + newpos, ctrl[1]))
      return -1;

    /* Adjust pointers */
    newpos += ctrl[1];
    oldpos += ctrl[2];
  };

  return 0;
}
