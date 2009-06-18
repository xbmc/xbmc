/*
  Adplug - Replayer for many OPL2/OPL3 audio file formats.
  Copyright (C) 1999 - 2004, 2006 Simon Peter, <dn.tlp@gmx.net>, et al.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  dmo.cpp - TwinTeam loader by Riven the Mage <riven@ok.ru>
*/
/*
  NOTES:
  Panning is ignored.

  A WORD ist 16 bits, a DWORD is 32 bits and a BYTE is 8 bits in this context.
*/

#include <string.h>
#include <binstr.h>

#include "dmo.h"
#include "debug.h"

#define LOWORD(l) ((l) & 0xffff)
#define HIWORD(l) ((l) >> 16)
#define LOBYTE(w) ((w) & 0xff)
#define HIBYTE(w) ((w) >> 8)

#define ARRAY_AS_DWORD(a, i) \
((a[i + 3] << 24) + (a[i + 2] << 16) + (a[i + 1] << 8) + a[i])
#define ARRAY_AS_WORD(a, i)	((a[i + 1] << 8) + a[i])

#define CHARP_AS_WORD(p)	(((*(p + 1)) << 8) + (*p))

/* -------- Public Methods -------------------------------- */

CPlayer *CdmoLoader::factory(Copl *newopl)
{
  return new CdmoLoader(newopl);
}

bool CdmoLoader::load(const std::string &filename, const CFileProvider &fp)
{
  int i,j;
  binistream *f;

  // check header
  dmo_unpacker *unpacker = new dmo_unpacker;
  unsigned char chkhdr[16];

  if(!fp.extension(filename, ".dmo")) return false;
  f = fp.open(filename); if(!f) return false;

  f->readString((char *)chkhdr, 16);

  if (!unpacker->decrypt(chkhdr, 16))
    {
      delete unpacker;
      fp.close(f);
      return false;
    }

  // get file size
  long packed_length = fp.filesize(f);
  f->seek(0);

  unsigned char *packed_module = new unsigned char [packed_length];

  // load file
  f->readString((char *)packed_module, packed_length);
  fp.close(f);

  // decrypt
  unpacker->decrypt(packed_module,packed_length);

  long unpacked_length = 0x2000 * ARRAY_AS_WORD(packed_module, 12);
  unsigned char *module = new unsigned char [unpacked_length];

  // unpack
  if (!unpacker->unpack(packed_module+12,module,unpacked_length))
    {
      delete unpacker;
      delete [] packed_module;
      delete [] module;
      return false;
    }

  delete unpacker;
  delete [] packed_module;

  // "TwinTeam" - signed ?
  if (memcmp(module,"TwinTeam Module File""\x0D\x0A",22))
    {
      delete module;
      return false;
    }

  // load header
  binisstream	uf(module, unpacked_length);
  uf.setFlag(binio::BigEndian, false); uf.setFlag(binio::FloatIEEE);

  memset(&header,0,sizeof(s3mheader));

  uf.ignore(22);				// ignore DMO header ID string
  uf.readString(header.name, 28);

  uf.ignore(2);				// _unk_1
  header.ordnum  = uf.readInt(2);
  header.insnum  = uf.readInt(2);
  header.patnum  = uf.readInt(2);
  uf.ignore(2);				// _unk_2
  header.is      = uf.readInt(2);
  header.it      = uf.readInt(2);

  memset(header.chanset,0xFF,32);

  for (i=0;i<9;i++)
    header.chanset[i] = 0x10 + i;

  uf.ignore(32);				// ignore panning settings for all 32 channels

  // load orders
  for(i = 0; i < 256; i++) orders[i] = uf.readInt(1);

  orders[header.ordnum] = 0xFF;

  // load pattern lengths
  unsigned short my_patlen[100];
  for(i = 0; i < 100; i++) my_patlen[i] = uf.readInt(2);

  // load instruments
  for (i = 0; i < header.insnum; i++)
    {
      memset(&inst[i],0,sizeof(s3minst));

      uf.readString(inst[i].name, 28);

      inst[i].volume = uf.readInt(1);
      inst[i].dsk    = uf.readInt(1);
      inst[i].c2spd  = uf.readInt(4);
      inst[i].type   = uf.readInt(1);
      inst[i].d00    = uf.readInt(1);
      inst[i].d01    = uf.readInt(1);
      inst[i].d02    = uf.readInt(1);
      inst[i].d03    = uf.readInt(1);
      inst[i].d04    = uf.readInt(1);
      inst[i].d05    = uf.readInt(1);
      inst[i].d06    = uf.readInt(1);
      inst[i].d07    = uf.readInt(1);
      inst[i].d08    = uf.readInt(1);
      inst[i].d09    = uf.readInt(1);
      inst[i].d0a    = uf.readInt(1);
      /*
       * Originally, riven sets d0b = d0a and ignores 1 byte in the
       * stream, but i guess this was a typo, so i read it here.
       */
      inst[i].d0b    = uf.readInt(1);
    }

  // load patterns
  for (i = 0; i < header.patnum; i++) {
    long cur_pos = uf.pos();

    for (j = 0; j < 64; j++) {
      while (1) {
	unsigned char token = uf.readInt(1);

	if (!token)
	  break;

	unsigned char chan = token & 31;

	// note + instrument ?
	if (token & 32) {
	  unsigned char bufbyte = uf.readInt(1);

	  pattern[i][j][chan].note = bufbyte & 15;
	  pattern[i][j][chan].oct = bufbyte >> 4;
	  pattern[i][j][chan].instrument = uf.readInt(1);
	}

	// volume ?
	if (token & 64)
	  pattern[i][j][chan].volume = uf.readInt(1);

	// command ?
	if (token & 128) {
	  pattern[i][j][chan].command = uf.readInt(1);
	  pattern[i][j][chan].info = uf.readInt(1);
	}
      }
    }

    uf.seek(cur_pos + my_patlen[i]);
  }

  delete [] module;
  rewind(0);
  return true;
}

std::string CdmoLoader::gettype()
{
  return std::string("TwinTeam (packed S3M)");
}

std::string CdmoLoader::getauthor()
{
  /*
    All available .DMO modules written by one composer. And because all .DMO
    stuff was lost due to hd crash (TwinTeam guys said this), there are
    never(?) be another.
  */
  return std::string("Benjamin GERARDIN");
}

/* -------- Private Methods ------------------------------- */

unsigned short CdmoLoader::dmo_unpacker::brand(unsigned short range)
{
  unsigned short ax,bx,cx,dx;

  ax = LOWORD(bseed);
  bx = HIWORD(bseed);
  cx = ax;
  ax = LOWORD(cx * 0x8405);
  dx = HIWORD(cx * 0x8405);
  cx <<= 3;
  cx = (((HIBYTE(cx) + LOBYTE(cx)) & 0xFF) << 8) + LOBYTE(cx);
  dx += cx;
  dx += bx;
  bx <<= 2;
  dx += bx;
  dx = (((HIBYTE(dx) + LOBYTE(bx)) & 0xFF) << 8) + LOBYTE(dx);
  bx <<= 5;
  dx = (((HIBYTE(dx) + LOBYTE(bx)) & 0xFF) << 8) + LOBYTE(dx);
  ax += 1;
  if (!ax) dx += 1;

  // leave it that way or amd64 might get it wrong
  bseed = dx;
  bseed <<= 16;
  bseed += ax;

  return HIWORD(HIWORD(LOWORD(bseed) * range) + HIWORD(bseed) * range);
}

bool CdmoLoader::dmo_unpacker::decrypt(unsigned char *buf, long len)
{
  unsigned long seed = 0;
  int i;

  bseed = ARRAY_AS_DWORD(buf, 0);

  for (i=0; i < ARRAY_AS_WORD(buf, 4) + 1; i++)
    seed += brand(0xffff);

  bseed = seed ^ ARRAY_AS_DWORD(buf, 6);

  if (ARRAY_AS_WORD(buf, 10) != brand(0xffff))
    return false;

  for (i=0;i<(len-12);i++)
    buf[12+i] ^= brand(0x100);

  buf[len - 2] = buf[len - 1] = 0;

  return true;
}

short CdmoLoader::dmo_unpacker::unpack_block(unsigned char *ibuf, long ilen, unsigned char *obuf)
{
  unsigned char code,par1,par2;
  unsigned short ax,bx,cx;

  unsigned char *ipos = ibuf;
  unsigned char *opos = obuf;

  // LZ77 child
  while (ipos - ibuf < ilen)
    {
      code = *ipos++;

      // 00xxxxxx: copy (xxxxxx + 1) bytes
      if ((code >> 6) == 0)
	{
	  cx = (code & 0x3F) + 1;

	  if(opos + cx >= oend)
	    return -1;

	  for (int i=0;i<cx;i++)
	    *opos++ = *ipos++;

	  continue;
	}

      // 01xxxxxx xxxyyyyy: copy (Y + 3) bytes from (X + 1)
      if ((code >> 6) == 1)
	{
	  par1 = *ipos++;

	  ax = ((code & 0x3F) << 3) + ((par1 & 0xE0) >> 5) + 1;
	  cx = (par1 & 0x1F) + 3;

	  if(opos + cx >= oend)
	    return -1;

	  for(int i=0;i<cx;i++)
	    *opos++ = *(opos - ax);

	  continue;
	}

      // 10xxxxxx xyyyzzzz: copy (Y + 3) bytes from (X + 1); copy Z bytes
      if ((code >> 6) == 2)
	{
	  int i;

	  par1 = *ipos++;

	  ax = ((code & 0x3F) << 1) + (par1 >> 7) + 1;
	  cx = ((par1 & 0x70) >> 4) + 3;
	  bx = par1 & 0x0F;

	  if(opos + bx + cx >= oend)
	    return -1;

	  for(i=0;i<cx;i++)
	    *opos++ = *(opos - ax);

	  for (i=0;i<bx;i++)
	    *opos++ = *ipos++;

	  continue;
	}

      // 11xxxxxx xxxxxxxy yyyyzzzz: copy (Y + 4) from X; copy Z bytes
      if ((code >> 6) == 3)
	{
	  int i;

	  par1 = *ipos++;
	  par2 = *ipos++;

	  bx = ((code & 0x3F) << 7) + (par1 >> 1);
	  cx = ((par1 & 0x01) << 4) + (par2 >> 4) + 4;
	  ax = par2 & 0x0F;

	  if(opos + ax + cx >= oend)
	    return -1;

	  for(i=0;i<cx;i++)
	    *opos++ = *(opos - bx);

	  for (i=0;i<ax;i++)
	    *opos++ = *ipos++;

	  continue;
	}
    }

  return opos - obuf;
}

long CdmoLoader::dmo_unpacker::unpack(unsigned char *ibuf, unsigned char *obuf,
				      unsigned long outputsize)
{
  long			olen = 0;
  unsigned short	block_count = CHARP_AS_WORD(ibuf);

  ibuf += 2;
  unsigned char *block_length = ibuf;
  ibuf += 2 * block_count;

  oend = obuf + outputsize;

  for (int i=0;i<block_count;i++)
    {
      unsigned short bul = CHARP_AS_WORD(ibuf);

      if(unpack_block(ibuf + 2,CHARP_AS_WORD(block_length) - 2,obuf) != bul)
	return 0;

      obuf += bul;
      olen += bul;

      ibuf += CHARP_AS_WORD(block_length);
      block_length += 2;
    }

  return olen;
}
