/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/shaders/gles/ShaderUtilsGLES.h"

#include <string>

#include <gtest/gtest.h>

using namespace KODI::SHADER;

TEST(TestShaderUtilsGLES, DropsTheProfileFromTheVersion)
{
  std::string source = "#version 330 core\nvoid main() {}\n";

  EXPECT_EQ(CShaderUtilsGLES::GetGLSLVersion(source), "#version 310 es\n");
  EXPECT_EQ(source, "\nvoid main() {}\n");
}

TEST(TestShaderUtilsGLES, KeepsACommentOnTheVersionLine)
{
  std::string source = "#version 120 /* comment\ncontinued */\nvoid main() {}\n";

  EXPECT_EQ(CShaderUtilsGLES::GetGLSLVersion(source), "#version 100\n");
  EXPECT_EQ(source, " /* comment\ncontinued */\nvoid main() {}\n");
}

TEST(TestShaderUtilsGLES, KeepsAnOpenGLESVersion)
{
  std::string source = "#version 300 es\nvoid main() {}\n";

  EXPECT_EQ(CShaderUtilsGLES::GetGLSLVersion(source), "#version 300 es\n");
  EXPECT_EQ(source, "\nvoid main() {}\n");
}

TEST(TestShaderUtilsGLES, TranslatesAVersionWithoutAProfile)
{
  std::string source = "#version 120\nvoid main() {}\n";

  EXPECT_EQ(CShaderUtilsGLES::GetGLSLVersion(source), "#version 100\n");
  EXPECT_EQ(source, "\nvoid main() {}\n");
}
