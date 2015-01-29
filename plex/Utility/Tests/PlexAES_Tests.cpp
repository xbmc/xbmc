#include "PlexTest.h"
#include "PlexAES.h"

const char* key = "ff0a27dc-338c-4e8a-9590-0dddb5f0cbfe";

TEST(PlexAES, encryptDecryptSmallStr)
{
  CPlexAES aes(key);
  CStdString encrypted = aes.encrypt("foo");

  EXPECT_STREQ("foo", aes.decrypt(encrypted).c_str());
}

TEST(PlexAES, encryptDecryptToken)
{
  CPlexAES aes(key);
  CStdString token("AAABzKSNM6RAFqPxJDuG");
  CStdString encrypted = aes.encrypt(token);

  EXPECT_EQ(encrypted.length(), 32);

  CStdString decrypted = aes.decrypt(encrypted);
  EXPECT_STREQ(token, decrypted);
}
