// -*- C++ -*-
// $Id$

// id3lib: a C++ library for creating and manipulating id3v1/v2 tags
// Copyright 1999, 2000  Scott Thomas Haug
// Copyright 2002 Thijmen Klok (thijmen@id3lib.org)

// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
// License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// The id3lib authors encourage improvements and optimisations to be sent to
// the id3lib coordinator.  Please see the README file for details on where to
// send such submissions.  See the AUTHORS file for a list of people who have
// contributed to id3lib.  See the ChangeLog file for a list of changes to
// id3lib.  These files are distributed with id3lib at
// http://download.sourceforge.net/id3lib/

#ifndef _ID3LIB_HELPERS_H_
#define _ID3LIB_HELPERS_H_

#if defined(__BORLANDC__)
// due to a bug in borland it sometimes still wants mfc compatibility even when you disable it
#  if defined(_MSC_VER)
#    undef _MSC_VER
#  endif
#  if defined(__MFC_COMPAT__)
#    undef __MFC_COMPAT__
#  endif
#endif

#include "id3/id3lib_strings.h"
#include "id3/globals.h" //has <stdlib.h> "id3/sized_types.h"

class ID3_TagImpl;
class ID3_Frame;

namespace dami
{
  namespace id3
  {
    namespace v2
    {
      ID3_C_EXPORT String     getString(const ID3_Frame*, ID3_FieldID);
      ID3_C_EXPORT String     getStringAtIndex(const ID3_Frame*, ID3_FieldID, size_t);

      ID3_C_EXPORT String     getFrameText(const ID3_TagImpl&, ID3_FrameID);
      ID3_C_EXPORT ID3_Frame* setFrameText(ID3_TagImpl&, ID3_FrameID, String);
      ID3_C_EXPORT size_t     removeFrames(ID3_TagImpl&, ID3_FrameID);

      ID3_C_EXPORT ID3_Frame* hasArtist(const ID3_TagImpl&);
      ID3_C_EXPORT String     getArtist(const ID3_TagImpl&);
      ID3_C_EXPORT ID3_Frame* setArtist(ID3_TagImpl&, String);
      ID3_C_EXPORT size_t     removeArtists(ID3_TagImpl&);

      ID3_C_EXPORT ID3_Frame* hasAlbum(const ID3_TagImpl&);
      ID3_C_EXPORT String     getAlbum(const ID3_TagImpl&);
      ID3_C_EXPORT ID3_Frame* setAlbum(ID3_TagImpl&, String);
      ID3_C_EXPORT size_t     removeAlbums(ID3_TagImpl&);

      ID3_C_EXPORT ID3_Frame* hasTitle(const ID3_TagImpl&);
      ID3_C_EXPORT String     getTitle(const ID3_TagImpl&);
      ID3_C_EXPORT ID3_Frame* setTitle(ID3_TagImpl&, String);
      ID3_C_EXPORT size_t     removeTitles(ID3_TagImpl&);

      ID3_C_EXPORT ID3_Frame* hasYear(const ID3_TagImpl&);
      ID3_C_EXPORT String     getYear(const ID3_TagImpl&);
      ID3_C_EXPORT ID3_Frame* setYear(ID3_TagImpl&, String);
      ID3_C_EXPORT size_t     removeYears(ID3_TagImpl&);

      ID3_C_EXPORT ID3_Frame* hasV1Comment(const ID3_TagImpl&);
      //      ID3_C_EXPORT ID3_Frame* hasComment(const ID3_TagImpl&, String desc);
      ID3_C_EXPORT ID3_Frame* hasComment(const ID3_TagImpl&);
      ID3_C_EXPORT String     getComment(const ID3_TagImpl&, String desc);
      ID3_C_EXPORT String     getV1Comment(const ID3_TagImpl&);
      ID3_C_EXPORT ID3_Frame* setComment(ID3_TagImpl&, String, String, String);
      ID3_C_EXPORT size_t     removeComments(ID3_TagImpl&, String);
      ID3_C_EXPORT size_t     removeAllComments(ID3_TagImpl&);

      ID3_C_EXPORT ID3_Frame* hasTrack(const ID3_TagImpl&);
      ID3_C_EXPORT String     getTrack(const ID3_TagImpl&);
      ID3_C_EXPORT size_t     getTrackNum(const ID3_TagImpl&);
      ID3_C_EXPORT ID3_Frame* setTrack(ID3_TagImpl&, uchar ucTrack, uchar ucTotal);
      ID3_C_EXPORT size_t     removeTracks(ID3_TagImpl&);

      ID3_C_EXPORT ID3_Frame* hasGenre(const ID3_TagImpl&);
      ID3_C_EXPORT String     getGenre(const ID3_TagImpl&);
      ID3_C_EXPORT size_t     getGenreNum(const ID3_TagImpl&);
      ID3_C_EXPORT ID3_Frame* setGenre(ID3_TagImpl&, size_t ucGenre);
      ID3_C_EXPORT size_t     removeGenres(ID3_TagImpl&);

      ID3_C_EXPORT ID3_Frame* hasLyrics(const ID3_TagImpl&);
      ID3_C_EXPORT String     getLyrics(const ID3_TagImpl&);
      ID3_C_EXPORT ID3_Frame* setLyrics(ID3_TagImpl&, String, String, String);
      ID3_C_EXPORT size_t     removeLyrics(ID3_TagImpl&);

      ID3_C_EXPORT String     getLyricist(const ID3_TagImpl&);
      ID3_C_EXPORT ID3_Frame* setLyricist(ID3_TagImpl&, String);
      ID3_C_EXPORT size_t     removeLyricists(ID3_TagImpl&);

      ID3_C_EXPORT ID3_Frame* hasSyncLyrics(const ID3_TagImpl&, String lang, String desc);
      ID3_C_EXPORT ID3_Frame* setSyncLyrics(ID3_TagImpl&, BString, ID3_TimeStampFormat,
                               String, String, ID3_ContentType);
      ID3_C_EXPORT BString    getSyncLyrics(const ID3_TagImpl& tag, String lang, String desc);
    };
  };
};

#endif /* _ID3LIB_HELPERS_H_ */

