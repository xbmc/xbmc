/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/posix/filesystem/SMBFile.h"

#include <gtest/gtest.h>

using namespace XFILE;

TEST(TestSMBFileRecovery, ReconnectableReadErrors)
{
#ifdef ENETRESET
  EXPECT_TRUE(SMBFileRecovery::IsReconnectableReadError(ENETRESET));
#endif
  EXPECT_TRUE(SMBFileRecovery::IsReconnectableReadError(ECONNRESET));
  EXPECT_TRUE(SMBFileRecovery::IsReconnectableReadError(ECONNABORTED));
  EXPECT_TRUE(SMBFileRecovery::IsReconnectableReadError(ENOTCONN));
  EXPECT_TRUE(SMBFileRecovery::IsReconnectableReadError(EPIPE));
  EXPECT_TRUE(SMBFileRecovery::IsReconnectableReadError(ETIMEDOUT));

  EXPECT_FALSE(SMBFileRecovery::IsReconnectableReadError(0));
  EXPECT_FALSE(SMBFileRecovery::IsReconnectableReadError(EACCES));
  EXPECT_FALSE(SMBFileRecovery::IsReconnectableReadError(ENOENT));
  EXPECT_FALSE(SMBFileRecovery::IsReconnectableReadError(EINVAL));
  EXPECT_FALSE(SMBFileRecovery::IsReconnectableReadError(EIO));
}

TEST(TestSMBFileRecovery, ZeroBytesBeforeKnownFileEndIsInvalidEof)
{
  EXPECT_FALSE(SMBFileRecovery::IsValidEof(4095, 4096));
}

TEST(TestSMBFileRecovery, ZeroBytesAtOrBeyondKnownFileEndIsValidEof)
{
  EXPECT_TRUE(SMBFileRecovery::IsValidEof(4096, 4096));
  EXPECT_TRUE(SMBFileRecovery::IsValidEof(8192, 4096));
}
