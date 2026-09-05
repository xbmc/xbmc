/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <filesystem>
#include <fstream>
#include <random>
#include <string>

#include <Platinum/Source/Devices/MediaServer/PltFileMediaServer.h>
#include <gtest/gtest.h>

namespace
{

std::string PathToUtf8(const std::filesystem::path& path)
{
  const std::u8string value = path.u8string();
  return {reinterpret_cast<const char*>(value.data()), value.size()};
}

#if defined(_WIN32)
std::string GenericPathToUtf8(const std::filesystem::path& path)
{
  const std::u8string value = path.generic_u8string();
  return {reinterpret_cast<const char*>(value.data()), value.size()};
}
#endif

std::filesystem::path PathFromUtf8(const NPT_String& path)
{
  const char8_t* begin = reinterpret_cast<const char8_t*>(path.GetChars());
  return std::filesystem::path(std::u8string(begin, begin + path.GetLength()));
}

class TestFileMediaServerDelegate : public PLT_FileMediaServerDelegate
{
public:
  explicit TestFileMediaServerDelegate(const std::filesystem::path& root)
    : PLT_FileMediaServerDelegate("/", PathToUtf8(root).c_str())
  {
  }

  using PLT_FileMediaServerDelegate::ProcessFileRequest;

  NPT_Result GetObjectPath(const char* objectId, NPT_String& path)
  {
    return GetFilePath(objectId, path);
  }

  bool CanBuildBrowseObject(const std::filesystem::path& path)
  {
    NPT_HttpUrl url;
    NPT_HttpRequest request(url, NPT_HTTP_METHOD_GET);
    PLT_HttpRequestContext context(request);
    PLT_MediaObjectReference object(BuildFromFilePath(PathToUtf8(path).c_str(), context));
    return !object.IsNull();
  }

  NPT_Result GetBrowseResourceUri(const std::filesystem::path& path, NPT_String& resourceUri)
  {
    NPT_HttpUrl url;
    NPT_HttpRequest request(url, NPT_HTTP_METHOD_GET);
    NPT_SocketAddress localAddress(NPT_IpAddress(127, 0, 0, 1), 1234);
    NPT_HttpRequestContext httpContext(&localAddress, nullptr);
    PLT_HttpRequestContext context(request, httpContext);
    PLT_MediaObjectReference object(BuildFromFilePath(PathToUtf8(path).c_str(), context));
    if (object.IsNull() || object->m_Resources.GetItemCount() == 0)
      return NPT_FAILURE;

    resourceUri = object->m_Resources[0].m_Uri;
    return NPT_SUCCESS;
  }

  NPT_Result OnUpdateObject(PLT_ActionReference&,
                            const char*,
                            NPT_Map<NPT_String, NPT_String>&,
                            NPT_Map<NPT_String, NPT_String>&,
                            const PLT_HttpRequestContext&) override
  {
    return NPT_ERROR_NOT_IMPLEMENTED;
  }
};

struct RequestResult
{
  NPT_Result result;
  NPT_HttpStatusCode status;
  std::string body;
  std::string mimeType;
  std::string contentFeatures;
};

class TestUPnPFileMediaServer : public testing::Test
{
protected:
  void SetUp() override
  {
    std::random_device random;
    m_base = std::filesystem::temp_directory_path() /
             ("kodi-upnp-file-server-" + std::to_string(random()) + std::to_string(random()));
    m_root = m_base / "root";
    m_root2 = m_base / "root2";

    std::filesystem::create_directories(m_root / "nested");
    std::filesystem::create_directories(m_root2);
    WriteFile(m_root / "movie..part.mkv", "normal");
    WriteFile(m_root / "..hidden", "hidden");
    WriteFile(m_root / "nested" / "song.flac", "nested");
    WriteFile(m_root2 / "outside.txt", "outside");
    WriteFile(m_base / "outside.txt", "outside");
  }

  void TearDown() override
  {
    std::error_code error;
    std::filesystem::remove_all(m_base, error);
  }

  static void WriteFile(const std::filesystem::path& path, const char* contents)
  {
    std::ofstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    stream << contents;
    ASSERT_TRUE(stream.good());
  }

  RequestResult Request(const char* rawPath) { return Request(m_root, rawPath); }

  RequestResult Request(const std::filesystem::path& root,
                        const char* rawPath,
                        bool requestContentFeatures = false)
  {
    TestFileMediaServerDelegate delegate(root);
    NPT_HttpUrl url;
    EXPECT_EQ(NPT_SUCCESS, url.SetPath(rawPath, true));
    NPT_HttpRequest request(url, NPT_HTTP_METHOD_GET);
    if (requestContentFeatures)
      request.GetHeaders().SetHeader("getcontentFeatures.dlna.org", "1");
    NPT_HttpRequestContext context;
    NPT_HttpResponse response(200, "OK");
    response.SetEntity(new NPT_HttpEntity());

    RequestResult requestResult{delegate.ProcessFileRequest(request, context, response),
                                response.GetStatusCode(),
                                {},
                                {},
                                {}};
    requestResult.mimeType = response.GetEntity()->GetContentType().GetChars();
    const NPT_String* contentFeatures =
        response.GetHeaders().GetHeaderValue("ContentFeatures.DLNA.ORG");
    if (contentFeatures)
      requestResult.contentFeatures = contentFeatures->GetChars();
    if (requestResult.result == NPT_SUCCESS && requestResult.status == 200)
    {
      NPT_DataBuffer data;
      EXPECT_EQ(NPT_SUCCESS, response.GetEntity()->Load(data));
      requestResult.body.assign(reinterpret_cast<const char*>(data.GetData()), data.GetDataSize());
    }
    return requestResult;
  }

  void ExpectRejected(const char* rawPath)
  {
    const RequestResult result = Request(rawPath);
    EXPECT_EQ(NPT_SUCCESS, result.result);
    EXPECT_EQ(404, result.status);
  }

  void ExpectServeFileRejected(const char* rawPath)
  {
    const RequestResult result = Request(rawPath);
    EXPECT_EQ(NPT_ERROR_NO_SUCH_ITEM, result.result);
    EXPECT_EQ(200, result.status);
  }

  std::filesystem::path m_base;
  std::filesystem::path m_root;
  std::filesystem::path m_root2;
};

TEST_F(TestUPnPFileMediaServer, ServesNormalFileBelowRoot)
{
  const RequestResult result = Request("/%25/movie..part.mkv");

  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("normal", result.body);
}

TEST_F(TestUPnPFileMediaServer, ServesFilenameBeginningWithTwoDots)
{
  const RequestResult result = Request("/%25/..hidden");

  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("hidden", result.body);
}

TEST_F(TestUPnPFileMediaServer, ServesNestedFileBelowRoot)
{
  const RequestResult result = Request("/%25/nested/song.flac");

  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("nested", result.body);
}

TEST_F(TestUPnPFileMediaServer, TrimsRedundantSeparatorsFromNonRoot)
{
  std::filesystem::path root = m_root;
  root += std::filesystem::path::preferred_separator;
  root += std::filesystem::path::preferred_separator;

  const RequestResult result = Request(root, "/%25/movie..part.mkv");
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("normal", result.body);
}

TEST_F(TestUPnPFileMediaServer, RoundTripsBrowseResourceUriFromFilesystemRoot)
{
  const std::filesystem::path filesystemRoot = m_root.root_path();
  const std::filesystem::path file = m_root / "nested" / "song.flac";
  const std::string relativePath = PathToUtf8(file.lexically_relative(filesystemRoot));
  const NPT_String encodedPath =
      NPT_Uri::PercentEncode(relativePath.c_str(), NPT_Uri::PathCharsToEncode);
  TestFileMediaServerDelegate delegate(filesystemRoot);
  NPT_String resourceUri;

  ASSERT_EQ(NPT_SUCCESS, delegate.GetBrowseResourceUri(file, resourceUri));
  NPT_HttpUrl url(resourceUri.GetChars());
  EXPECT_NE(NPT_STRING_SEARCH_FAILED, url.GetPath().Find(encodedPath));

  const RequestResult result = Request(filesystemRoot, url.GetPath());
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("nested", result.body);
}

#if defined(_WIN32)
TEST_F(TestUPnPFileMediaServer, PreservesWindowsDriveRoot)
{
  const std::filesystem::path driveRoot = m_root.root_path();
  ASSERT_FALSE(driveRoot.empty());
  ASSERT_TRUE(driveRoot.is_absolute());

  const std::filesystem::path file = m_root / "nested" / "song.flac";
  const std::string relativePath = GenericPathToUtf8(file.lexically_relative(driveRoot));
  const NPT_String encodedPath =
      NPT_Uri::PercentEncode(relativePath.c_str(), NPT_Uri::PathCharsToEncode);
  const std::string rawPath = "/%25/" + std::string(encodedPath.GetChars());
  const RequestResult result = Request(driveRoot, rawPath.c_str());

  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("nested", result.body);
}
#endif

TEST_F(TestUPnPFileMediaServer, UsesRequestedPathForMimeTypeOfInRootSymlink)
{
#if defined(_WIN32)
  GTEST_SKIP() << "Windows reparse points are intentionally rejected";
#else
  WriteFile(m_root / "extensionless-target", "symlink");
  std::error_code error;
  std::filesystem::create_symlink("extensionless-target", m_root / "movie.mp4", error);
  if (error)
    GTEST_SKIP() << "Cannot create file symlink: " << error.message();

  const RequestResult result = Request(m_root, "/%25/movie.mp4", true);
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("symlink", result.body);
  EXPECT_EQ("video/mp4", result.mimeType);
  EXPECT_NE(std::string::npos, result.contentFeatures.find("MPEG4_P2_SP_AAC"));
#endif
}

TEST_F(TestUPnPFileMediaServer, AcceptsCaseVariantInRootSymlinkOnCaseInsensitiveFilesystem)
{
#if defined(_WIN32)
  GTEST_SKIP() << "Windows reparse points are intentionally rejected";
#else
  const std::filesystem::path alternateRoot = m_root.parent_path() / "ROOT";
  std::error_code error;
  if (!std::filesystem::equivalent(m_root, alternateRoot, error) || error)
    GTEST_SKIP() << "Filesystem is case-sensitive";

  WriteFile(m_root / "case-target", "case");
  std::filesystem::create_symlink(alternateRoot / "case-target", m_root / "case-link", error);
  if (error)
    GTEST_SKIP() << "Cannot create file symlink: " << error.message();

  const RequestResult result = Request("/%25/case-link");
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("case", result.body);
#endif
}

TEST_F(TestUPnPFileMediaServer, RejectsTraversalPaths)
{
  ExpectRejected("/%25/../outside.txt");
  ExpectRejected("/%25/..\\outside.txt");
  ExpectRejected("/%25/%2e%2e%2foutside.txt");
  ExpectRejected("/%25/%2e%2e%5coutside.txt");
  ExpectRejected("/%25/.%2e/outside.txt");
  ExpectRejected("/%25/nested/../../outside.txt");
  ExpectRejected("/%/../outside.txt");
}

TEST_F(TestUPnPFileMediaServer, RejectsAbsolutePaths)
{
  ExpectRejected("/%25//etc/passwd");
#if defined(_WIN32)
  ExpectRejected("/%25/C:%5cWindows%5cwin.ini");
#endif
  ExpectRejected("/%25/%5c%5cserver%5cshare%5cfile");
}

TEST_F(TestUPnPFileMediaServer, RejectsRootPrefixCollision)
{
  ExpectRejected("/%25/../root2/outside.txt");
}

TEST_F(TestUPnPFileMediaServer, DoesNotDecodePercentEncodingTwice)
{
  WriteFile(m_root / "%2e%2e%2fpercent.txt", "percent");

  const RequestResult result = Request("/%25/%252e%252e%252fpercent.txt");
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("percent", result.body);
}

TEST_F(TestUPnPFileMediaServer, RejectsMalformedPathEncodingWithoutThrowing)
{
  EXPECT_NO_THROW(ExpectServeFileRejected("/%25/%ff"));
  EXPECT_NO_THROW(ExpectServeFileRejected("/%25/%c3%28"));
}

TEST_F(TestUPnPFileMediaServer, PreservesServeFileFailureSemantics)
{
  ExpectServeFileRejected("/%25/missing-file");
}

TEST_F(TestUPnPFileMediaServer, RejectsSymlinkEscape)
{
  std::error_code error;
  std::filesystem::create_directory_symlink(m_root2, m_root / "outside-link", error);
  if (error)
    GTEST_SKIP() << "Cannot create directory symlink: " << error.message();

  ExpectServeFileRejected("/%25/outside-link/outside.txt");
}

TEST_F(TestUPnPFileMediaServer, ResolvesNormalBrowseObjectIdsBelowRoot)
{
  TestFileMediaServerDelegate delegate(m_root);
  NPT_String path;

  ASSERT_EQ(NPT_SUCCESS, delegate.GetObjectPath("0/nested/song.flac", path));
  EXPECT_TRUE(std::filesystem::equivalent(PathFromUtf8(path), m_root / "nested" / "song.flac"));
}

TEST_F(TestUPnPFileMediaServer, RejectsBrowseObjectIdTraversal)
{
  TestFileMediaServerDelegate delegate(m_root);
  NPT_String path;

  EXPECT_TRUE(NPT_FAILED(delegate.GetObjectPath("0/../outside.txt", path)));
  EXPECT_TRUE(NPT_FAILED(delegate.GetObjectPath("0/..\\outside.txt", path)));
  EXPECT_TRUE(NPT_FAILED(delegate.GetObjectPath("0/nested/../../outside.txt", path)));
  EXPECT_TRUE(NPT_FAILED(delegate.GetObjectPath("0/../root2/outside.txt", path)));
}

TEST_F(TestUPnPFileMediaServer, RejectsMalformedBrowseObjectIdWithoutThrowing)
{
  TestFileMediaServerDelegate delegate(m_root);
  NPT_String path;
  const char malformedObjectId[] = {'0', '/', static_cast<char>(0xff), '\0'};

  EXPECT_NO_THROW(EXPECT_TRUE(NPT_FAILED(delegate.GetObjectPath(malformedObjectId, path))));
}

TEST_F(TestUPnPFileMediaServer, DoesNotUrlDecodeBrowseObjectIds)
{
  WriteFile(m_root / "%2e%2e", "percent-object-id");
  TestFileMediaServerDelegate delegate(m_root);
  NPT_String path;

  ASSERT_EQ(NPT_SUCCESS, delegate.GetObjectPath("0/%2e%2e", path));
  EXPECT_TRUE(std::filesystem::equivalent(PathFromUtf8(path), m_root / "%2e%2e"));
}

TEST_F(TestUPnPFileMediaServer, RejectsBrowseObjectIdSymlinkEscape)
{
  std::error_code error;
  std::filesystem::create_directory_symlink(m_root2, m_root / "browse-link", error);
  if (error)
    GTEST_SKIP() << "Cannot create directory symlink: " << error.message();

  TestFileMediaServerDelegate delegate(m_root);
  NPT_String path;
  EXPECT_TRUE(NPT_FAILED(delegate.GetObjectPath("0/browse-link/outside.txt", path)));
  EXPECT_FALSE(delegate.CanBuildBrowseObject(m_root / "browse-link"));
}

} // namespace
