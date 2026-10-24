/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "pictures/MotionPhoto.h"
#include "test/TestUtils.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace KODI;

namespace
{
// An MP4 starts with an "ftyp" box
const std::string CLIP = std::string(3, '\0') + "\x18" + "ftypisom" + "the clip's frames";

std::string MakeJpeg(const std::string& xmp, const std::string& clip)
{
  return "\xFF\xD8<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">" + xmp + "</x:xmpmeta>\xFF\xD9" + clip;
}

void WriteFile(const std::string& path, const std::string& contents)
{
  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(path, true));
  file.Write(contents.data(), contents.size());
}

std::string Extract(const std::string& contents)
{
  XFILE::CFile* file = XBMC_CREATETEMPFILE(".jpg");
  EXPECT_NE(file, nullptr);
  if (!file)
    return {};
  file->Write(contents.data(), contents.size());
  file->Close();

  const std::string clipPath = PICTURE::ExtractMotionPhotoClip(XBMC_TEMPFILEPATH(file));
  XBMC_DELETETEMPFILE(file);

  std::string clip;
  XFILE::CFile clipFile;
  if (!clipPath.empty() && clipFile.Open(clipPath))
  {
    clip.resize(static_cast<size_t>(clipFile.GetLength()));
    clipFile.Read(clip.data(), clip.size());
  }
  return clip;
}
} // namespace

TEST(TestMotionPhoto, GroupLivePhotos)
{
  const std::string folder = "special://temp/livephotos/";
  ASSERT_TRUE(XFILE::CDirectory::Create(folder));
  // Apple's maker note with a content identifier, as in a Live Photo's still
  const std::string makerNote = std::string("Apple iOS\0\0\x01MM\0\x01", 16) +
                                std::string("\0\x11\0\x02\0\0\0\x25\0\0\0\x20", 12);
  WriteFile(folder + "IMG_0001.JPG", MakeJpeg(makerNote, ""));
  WriteFile(folder + "movie.jpg", MakeJpeg("", ""));

  CFileItemList items;
  for (const char* name :
       {"IMG_0001.JPG", "IMG_0001.MOV", "movie.jpg", "movie.mov", "IMG_0003.mov"})
    items.Add(std::make_shared<CFileItem>(folder + name, false));

  PICTURE::GroupLivePhotos(items);
  XFILE::CDirectory::RemoveRecursive(folder);

  ASSERT_EQ(items.Size(), 4);
  EXPECT_EQ(items[0]->GetProperty("motionclip").asString(), folder + "IMG_0001.MOV");
  EXPECT_FALSE(items[1]->HasProperty("motionclip"));
  EXPECT_EQ(items[2]->GetPath(), folder + "movie.mov");
  EXPECT_EQ(items[3]->GetPath(), folder + "IMG_0003.mov");
}

TEST(TestMotionPhoto, MotionPhoto)
{
  for (const std::string prefix : {"GCamera", "Camera"})
  {
    const std::string xmp = "<rdf:Description " + prefix +
                            ":MotionPhoto=\"1\"><Container:Directory><rdf:Seq>"
                            "<rdf:li><Container:Item Item:Semantic=\"Primary\" "
                            "Item:Mime=\"image/jpeg\"/></rdf:li><rdf:li><Container:Item "
                            "Item:Mime=\"video/mp4\" Item:Semantic=\"MotionPhoto\" Item:Length=\"" +
                            std::to_string(CLIP.size()) +
                            "\" Item:Padding=\"0\"/></rdf:li></rdf:Seq></Container:Directory>"
                            "</rdf:Description>";
    EXPECT_EQ(Extract(MakeJpeg(xmp, CLIP)), CLIP) << prefix;
    PICTURE::ClearMotionPhotoClips();
  }
}

TEST(TestMotionPhoto, MicroVideo)
{
  const std::string xmp =
      "GCamera:MicroVideo=\"1\" GCamera:MicroVideoOffset=\"" + std::to_string(CLIP.size()) + "\"";
  EXPECT_EQ(Extract(MakeJpeg(xmp, CLIP)), CLIP);
  PICTURE::ClearMotionPhotoClips();
}

TEST(TestMotionPhoto, StillPhoto)
{
  EXPECT_EQ(Extract(MakeJpeg("<rdf:RDF/>", "")), "");
  EXPECT_EQ(Extract(MakeJpeg("GCamera:MicroVideo=\"1\" GCamera:MicroVideoOffset=\"12\"",
                             "not an mp4 box")),
            "");
}
