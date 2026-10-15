/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/shaders/gl/ShaderUtilsGL.h"

#include <string>

#include <gtest/gtest.h>

using namespace KODI::SHADER;

TEST(TestShaderUtilsGL, DropsTheProfileFromTheVersion)
{
  std::string source = "#version 330 core\nvoid main() {}\n";

  EXPECT_EQ(CShaderUtilsGL::GetGLSLVersion(source), "#version 330\n");
  EXPECT_EQ(source, "\nvoid main() {}\n");
}

TEST(TestShaderUtilsGL, KeepsACommentOnTheVersionLine)
{
  std::string source = "#version 330 core // note\nvoid main() {}\n";

  EXPECT_EQ(CShaderUtilsGL::GetGLSLVersion(source), "#version 330\n");
  EXPECT_EQ(source, " // note\nvoid main() {}\n");
}

TEST(TestShaderUtilsGL, KeepsABlockCommentStartedOnTheVersionLine)
{
  std::string source = "#version 330 /* comment\ncontinued */\nvoid main() {}\n";

  EXPECT_EQ(CShaderUtilsGL::GetGLSLVersion(source), "#version 330\n");
  EXPECT_EQ(source, " /* comment\ncontinued */\nvoid main() {}\n");
}

TEST(TestShaderUtilsGL, KeepsAVersionWithoutAProfile)
{
  std::string source = "#version 120\nvoid main() {}\n";

  EXPECT_EQ(CShaderUtilsGL::GetGLSLVersion(source), "#version 120\n");
  EXPECT_EQ(source, "\nvoid main() {}\n");
}
