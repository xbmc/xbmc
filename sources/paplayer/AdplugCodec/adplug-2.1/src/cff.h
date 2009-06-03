/*
  AdPlug - Replayer for many OPL2/OPL3 audio file formats.
  Copyright (C) 1999 - 2006 Simon Peter <dn.tlp@gmx.net>, et al.

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

  cff.h - BoomTracker loader by Riven the Mage <riven@ok.ru>
*/

#include "protrack.h"

class CcffLoader: public CmodPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  CcffLoader(Copl *newopl) : CmodPlayer(newopl) { };

  bool	load(const std::string &filename, const CFileProvider &fp);
  void	rewind(int subsong);

  std::string		gettype();
  std::string		gettitle();
  std::string		getauthor();
  std::string		getinstrument(unsigned int n);
  unsigned int	getinstruments();

 private:

  class cff_unpacker
    {
    public:

      long unpack(unsigned char *ibuf, unsigned char *obuf);

    private:

      unsigned long get_code();
      void translate_code(unsigned long code, unsigned char *string);

      void cleanup();
      int startup();

      void expand_dictionary(unsigned char *string);

      unsigned char *input;
      unsigned char *output;

      long output_length;

      unsigned char code_length;

      unsigned long bits_buffer;
      unsigned int bits_left;

      unsigned char *heap;
      unsigned char **dictionary;

      unsigned int heap_length;
      unsigned int dictionary_length;

      unsigned long old_code,new_code;

      unsigned char the_string[256];
    };

  struct cff_header
  {
    char	id[16];
    unsigned char	version;
    unsigned short	size;
    unsigned char	packed;
    unsigned char	reserved[12];
  } header;

  struct cff_instrument
  {
    unsigned char	data[12];
    char		name[21];
  } instruments[47];

  char	song_title[20];
  char	song_author[20];

  struct cff_event
  {
    unsigned char	byte0;
    unsigned char	byte1;
    unsigned char	byte2;
  };
};
