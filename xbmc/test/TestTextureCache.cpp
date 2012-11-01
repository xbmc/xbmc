/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "URL.h"
#include "TextureCache.h"

#include "gtest/gtest.h"

TEST(TestTextureCache, GetWrappedImageURL)
{
  typedef struct
  {
    const char *in;
    const char *type;
    const char *options;
    const char *out;
  } testfiles;

  const testfiles test_files[] = {{ "/path/to/image/file.jpg", "", "", "image://%2fpath%2fto%2fimage%2ffile.jpg/" },
                                  { "/path/to/image/file.jpg", "", "size=thumb", "image://%2fpath%2fto%2fimage%2ffile.jpg/transform?size=thumb" },
                                  { "/path/to/video/file.mkv", "video", "", "image://video@%2fpath%2fto%2fvideo%2ffile.mkv/" },
                                  { "/path/to/music/file.mp3", "music", "", "image://music@%2fpath%2fto%2fmusic%2ffile.mp3/" },
                                  { "image://%2fpath%2fto%2fimage%2ffile.jpg/", "", "", "image://%2fpath%2fto%2fimage%2ffile.jpg/" },
                                  { "image://%2fpath%2fto%2fimage%2ffile.jpg/transform?size=thumb", "", "size=thumb", "image://%2fpath%2fto%2fimage%2ffile.jpg/transform?size=thumb" }};

  for (unsigned int i = 0; i < sizeof(test_files) / sizeof(testfiles); i++)
  {
    std::string expected = test_files[i].out;
    std::string out = CTextureCache::GetWrappedImageURL(test_files[i].in, test_files[i].type, test_files[i].options);
    EXPECT_EQ(out, expected);
  }
}
