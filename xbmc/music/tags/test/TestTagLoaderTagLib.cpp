/*
 *      Copyright (C) 2012-2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "gtest/gtest.h"
#include "music/tags/TagLoaderTagLib.h"
#include "music/tags/MusicInfoTag.h"
#include <taglib/tpropertymap.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/apetag.h>
#include <taglib/xiphcomment.h>
#include <taglib/id3v1genres.h>
#include <taglib/asftag.h>
#include <taglib/mp4tag.h>

using namespace TagLib;
using namespace MUSIC_INFO;

template <typename T>
class TestTagParser : public ::testing::Test, public CTagLoaderTagLib {
   public:
     T value_;
};


typedef ::testing::Types<ID3v2::Tag, ID3v1::Tag, ASF::Tag, APE::Tag, Ogg::XiphComment, MP4::Tag> TagTypes;
TYPED_TEST_CASE(TestTagParser, TagTypes);

TYPED_TEST(TestTagParser, ParsesBasicTag) {
  // Create a basic tag
  TypeParam *tg  = &this->value_;
  // Configure a basic tag..
  tg->setTitle ("title");
  tg->setArtist ("artist");
  tg->setAlbum ("album");
  tg->setComment("comment");
  tg->setGenre("Jazz");
  tg->setYear (1985);
  tg->setTrack (2);

  CMusicInfoTag tag;
  EXPECT_TRUE(CTagLoaderTagLib::ParseTag<TypeParam>(tg, NULL, tag));

  EXPECT_EQ(1985, tag.GetYear());
  EXPECT_EQ(2, tag.GetTrackNumber());
  EXPECT_EQ(1u, tag.GetArtist().size());
  if (!tag.GetArtist().empty()) EXPECT_EQ("artist", tag.GetArtist().front());
  EXPECT_EQ("album", tag.GetAlbum());
  EXPECT_EQ("comment", tag.GetComment());
  EXPECT_EQ(1u, tag.GetGenre().size());
  if (!tag.GetGenre().empty()) EXPECT_EQ("Jazz", tag.GetGenre().front());
  EXPECT_EQ("title", tag.GetTitle());
}


TYPED_TEST(TestTagParser, HandleNullTag) {
  // A Null tag should not parse, and not break us either
  CMusicInfoTag tag;
  EXPECT_FALSE(CTagLoaderTagLib::ParseTag<TypeParam>(NULL, NULL, tag));
}

template<typename T, size_t N>
T * end(T (&ra)[N]) {
      return ra + N;
}

const char *tags[] = { "APIC", "ASPI", "COMM", "COMR", "ENCR", "EQU2",
  "ETCO", "GEOB", "GRID", "LINK", "MCDI", "MLLT", "OWNE", "PRIV", "PCNT",
  "POPM", "POSS", "RBUF", "RVA2", "RVRB", "SEEK", "SIGN", "SYLT",
  "SYTC", "TALB", "TBPM", "TCOM", "TCON", "TCOP", "TDEN", "TDLY", "TDOR",
  "TDRC", "TDRL", "TDTG", "TENC", "TEXT", "TFLT", "TIPL", "TIT1", "TIT2",
  "TIT3", "TKEY", "TLAN", "TLEN", "TMCL", "TMED", "TMOO", "TOAL", "TOFN",
  "TOLY", "TOPE", "TOWN", "TPE1", "TPE2", "TPE3", "TPE4", "TPOS", "TPRO",
  "TPUB", "TRCK", "TRSN", "TRSO", "TSOA", "TSOP", "TSOT", "TSRC", "TSSE",
  "TSST", "TXXX", "UFID", "USER", "USLT", "WCOM", "WCOP", "WOAF", "WOAR",
  "WOAS", "WORS", "WPAY", "WPUB", "WXXX",  "ARTIST", "ARTISTS",
  "ALBUMARTIST" , "ALBUM ARTIST", "ALBUMARTISTS" , "ALBUM ARTISTS", "ALBUM",
  "TITLE", "TRACKNUMBER" "TRACK", "DISCNUMBER" "DISC", "YEAR", "GENRE",
  "COMMENT", "CUESHEET", "ENCODEDBY", "COMPILATION", "LYRICS",
  "REPLAYGAIN_TRACK_GAIN", "REPLAYGAIN_ALBUM_GAIN", "REPLAYGAIN_TRACK_PEAK",
  "REPLAYGAIN_ALBUM_PEAK", "MUSICBRAINZ_ARTISTID",
  "MUSICBRAINZ_ALBUMARTISTID", "RATING", "MUSICBRAINZ_ALBUMARTIST",
  "MUSICBRAINZ_ALBUMID", "MUSICBRAINZ_TRACKID", "METADATA_BLOCK_PICTURE",
  "COVERART"
};


// This test exposes a bug in taglib library (#670) so for now we will not run it for all tag types
// See https://github.com/taglib/taglib/issues/670 for details.
typedef ::testing::Types<ID3v2::Tag, ID3v1::Tag, ASF::Tag, APE::Tag, Ogg::XiphComment> EmptyPropertiesTagTypes;
template <typename T>
class EmptyTagParser : public ::testing::Test, public CTagLoaderTagLib {
  public:
    T value_;
};
TYPED_TEST_CASE(EmptyTagParser, EmptyPropertiesTagTypes);

TYPED_TEST(EmptyTagParser, EmptyProperties) {
  TypeParam *tg  = &this->value_;
  CMusicInfoTag tag;
  PropertyMap props;
  int tagcount = end(tags) - tags;
  for(int i = 0; i < tagcount; i++) {
    props.insert(tags[i], StringList());
  }

  // Even though all the properties are empty, we shouldn't
  // crash
  EXPECT_TRUE(CTagLoaderTagLib::ParseTag<TypeParam>(tg, NULL, tag));
}



TYPED_TEST(TestTagParser, FooProperties) {
  TypeParam *tg  = &this->value_;
  CMusicInfoTag tag;
  PropertyMap props;
  int tagcount = end(tags) - tags;
  for(int i = 0; i < tagcount; i++) {
    props.insert(tags[i], String("foo"));
  }
  tg->setProperties(props);

  EXPECT_TRUE(CTagLoaderTagLib::ParseTag<TypeParam>(tg, NULL, tag));
  EXPECT_EQ(0, tag.GetYear());
  EXPECT_EQ(0, tag.GetTrackNumber());
  EXPECT_EQ(1u, tag.GetArtist().size());
  if (!tag.GetArtist().empty()) EXPECT_EQ("foo", tag.GetArtist().front());
  EXPECT_EQ("foo", tag.GetAlbum());
  EXPECT_EQ("foo", tag.GetComment());
  if (!tag.GetGenre().empty()) EXPECT_EQ("foo", tag.GetGenre().front());
  EXPECT_EQ("foo", tag.GetTitle());
}

class TestCTagLoaderTagLib : public ::testing::Test, public CTagLoaderTagLib {};
TEST_F(TestCTagLoaderTagLib, SetGenre)
{
  CMusicInfoTag tag, tag2;
  const char *genre_nr[] = {"0", "2", "4"};
  const char *names[] = { "Jazz", "Funk", "Ska" };
  std::vector<std::string> genres(genre_nr, end(genre_nr));
  std::vector<std::string> named_genre(names, end(names));

  CTagLoaderTagLib::SetGenre(tag, genres);
  EXPECT_EQ(3u, tag.GetGenre().size());
  EXPECT_EQ("Blues", tag.GetGenre()[0]);
  EXPECT_EQ("Country", tag.GetGenre()[1]);
  EXPECT_EQ("Disco", tag.GetGenre()[2]);

  CTagLoaderTagLib::SetGenre(tag2, named_genre);
  EXPECT_EQ(3u, tag2.GetGenre().size());
  for(int i = 0; i < 3; i++)
    EXPECT_EQ(names[i], tag2.GetGenre()[i]);

}

TEST(TestTagLoaderTagLib, SplitMBID)
{
  CTagLoaderTagLib lib;

  // SplitMBID should return the vector if it's empty or longer than 1
  std::vector<std::string> values;
  EXPECT_TRUE(lib.SplitMBID(values).empty());

  values.push_back("1");
  values.push_back("2");
  EXPECT_EQ(values, lib.SplitMBID(values));

  // length 1 and invalid should return empty
  values.clear();
  values.push_back("invalid");
  EXPECT_TRUE(lib.SplitMBID(values).empty());

  // length 1 and valid should return the valid id
  values.clear();
  values.push_back("0383dadf-2a4e-4d10-a46a-e9e041da8eb3");
  EXPECT_EQ(lib.SplitMBID(values), values);

  // case shouldn't matter
  values.clear();
  values.push_back("0383DaDf-2A4e-4d10-a46a-e9e041da8eb3");
  EXPECT_EQ(lib.SplitMBID(values).size(), 1u);
  EXPECT_STREQ(lib.SplitMBID(values)[0].c_str(), "0383dadf-2a4e-4d10-a46a-e9e041da8eb3");

  // valid with some stuff off the end or start should return valid
  values.clear();
  values.push_back("foo0383dadf-2a4e-4d10-a46a-e9e041da8eb3 blah");
  EXPECT_EQ(lib.SplitMBID(values).size(), 1u);
  EXPECT_STREQ(lib.SplitMBID(values)[0].c_str(), "0383dadf-2a4e-4d10-a46a-e9e041da8eb3");

  // two valid with various separators
  values.clear();
  values.push_back("0383dadf-2a4e-4d10-a46a-e9e041da8eb3;53b106e7-0cc6-42cc-ac95-ed8d30a3a98e");
  std::vector<std::string> result = lib.SplitMBID(values);
  EXPECT_EQ(result.size(), 2u);
  EXPECT_STREQ(result[0].c_str(), "0383dadf-2a4e-4d10-a46a-e9e041da8eb3");
  EXPECT_STREQ(result[1].c_str(), "53b106e7-0cc6-42cc-ac95-ed8d30a3a98e");

  values.clear();
  values.push_back("0383dadf-2a4e-4d10-a46a-e9e041da8eb3/53b106e7-0cc6-42cc-ac95-ed8d30a3a98e");
  result = lib.SplitMBID(values);
  EXPECT_EQ(result.size(), 2u);
  EXPECT_STREQ(result[0].c_str(), "0383dadf-2a4e-4d10-a46a-e9e041da8eb3");
  EXPECT_STREQ(result[1].c_str(), "53b106e7-0cc6-42cc-ac95-ed8d30a3a98e");

  values.clear();
  values.push_back("0383dadf-2a4e-4d10-a46a-e9e041da8eb3 / 53b106e7-0cc6-42cc-ac95-ed8d30a3a98e; ");
  result = lib.SplitMBID(values);
  EXPECT_EQ(result.size(), 2u);
  EXPECT_STREQ(result[0].c_str(), "0383dadf-2a4e-4d10-a46a-e9e041da8eb3");
  EXPECT_STREQ(result[1].c_str(), "53b106e7-0cc6-42cc-ac95-ed8d30a3a98e");
}
