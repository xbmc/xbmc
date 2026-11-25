/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "Util.h"
#include "dbwrappers/test/DatabaseTestUtils.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "test/TestUtils.h"
#include "utils/Artwork.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoManagerTypes.h"

#include <string>
#include <utility>

#include <gtest/gtest.h>

/*!
 * \brief A fixture holding a private video database in special://temp/, and a folder there for the
 * files a test makes.
 *
 * Connect() rather than Open(), so this brings up the video schema alone rather than every
 * database Kodi has by way of CDatabaseManager. Each test gets a database of its own, named
 * after it, so no test reads another's rows.
 */
class VideoDatabaseTestBase : public ::testing::Test
{
protected:
  VideoDatabaseTestBase(std::string namePrefix, std::string pathPrefix)
    : m_namePrefix(std::move(namePrefix)),
      m_pathPrefix(std::move(pathPrefix))
  {
  }

  void SetUp() override
  {
    m_name = StringUtils::Format("{}{}", m_namePrefix,
                                 ::testing::UnitTest::GetInstance()->current_test_info()->name());
    m_root = CSpecialProtocol::TranslatePath("special://temp/" + m_name + "/");
    XFILE::CDirectory::RemoveRecursive(m_root);

    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
              m_db.Connect(m_name, TestDatabaseSettings(), true))
        << "could not create a video database for this test";
  }

  void TearDown() override
  {
    m_db.Close();
    XFILE::CFile::Delete(TestDatabaseSettings().host + m_name + ".db");
    XFILE::CDirectory::RemoveRecursive(m_root);
  }

  int AddTestFile(const std::string& name)
  {
    const int idFile{m_db.AddFile(m_pathPrefix + name, m_pathPrefix)};
    EXPECT_GE(idFile, 0);
    return idFile;
  }

  //! \return the folder \p name in \p parent, ending in a separator, made unless \p onDisk is false
  static std::string Folder(const std::string& parent, const std::string& name, bool onDisk = true)
  {
    std::string folder{URIUtils::AddFileToFolder(parent, name)};
    URIUtils::AddSlashAtEnd(folder);
    if (onDisk)
    {
      EXPECT_TRUE(CUtil::CreateDirectoryEx(folder));
    }
    return folder;
  }

  //! \return the empty file \p name made in \p folder
  static std::string File(const std::string& folder, const std::string& name)
  {
    const std::string path{URIUtils::AddFileToFolder(folder, name)};
    EXPECT_TRUE(WriteTextFile(path, ""));
    return path;
  }

  /*!
   * \brief Adds \p path to the movie as an asset of the type named \p name, or by default of
   * "Extended" for a version and "Trailer" for an extra.
   * \return the asset's file id, -1 if it was not added
   */
  int AddAsset(int idMovie,
               const std::string& path,
               VideoAssetType type = VideoAssetType::VERSION,
               const std::string& name = "",
               VideoAssetTypeOwner owner = VideoAssetTypeOwner::USER,
               const KODI::ART::Artwork& art = {})
  {
    CFileItem item{path, false};
    item.SetArt(art);
    const std::string typeName{!name.empty()                     ? name
                               : type == VideoAssetType::VERSION ? "Extended"
                                                                 : "Trailer"};
    const int idType{m_db.AddVideoVersionType(typeName, owner, type)};
    if (!m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, idType, type, item))
      return -1;
    return m_db.AddFile(path);
  }

  CVideoDatabase m_db;
  //! A folder of the test's own in special://temp/, which goes with the test
  std::string m_root;

private:
  const std::string m_namePrefix;
  const std::string m_pathPrefix;
  std::string m_name;
};
