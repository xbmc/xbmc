/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/shaders/ShaderUtils.h"

#include <string>

#include <gtest/gtest.h>

using namespace KODI::SHADER;

TEST(TestShaderUtils, StripsAParameterLine)
{
  const std::string source = "#pragma parameter GAMMA \"Gamma\" 2.2 1.0 3.0 0.1\n"
                             "uniform float GAMMA;\n";

  EXPECT_EQ(CShaderUtils::StripParameterPragmas(source), "uniform float GAMMA;\n");
}

TEST(TestShaderUtils, StripsAnIndentedParameterLine)
{
  const std::string source = "  #pragma parameter GAMMA \"Gamma\" 2.2 1.0 3.0 0.1\n"
                             "uniform float GAMMA;\n";

  EXPECT_EQ(CShaderUtils::StripParameterPragmas(source), "uniform float GAMMA;\n");
}

TEST(TestShaderUtils, StripsAParameterOnTheLastLine)
{
  const std::string source = "uniform float GAMMA;\n"
                             "#pragma parameter GAMMA \"Gamma\" 2.2 1.0 3.0 0.1";

  EXPECT_EQ(CShaderUtils::StripParameterPragmas(source), "uniform float GAMMA;\n");
}

TEST(TestShaderUtils, KeepsAPragmaMentionedInAComment)
{
  const std::string source = "#ifdef PARAMETER_UNIFORM // Defined if #pragma parameters work.\n"
                             "uniform float GAMMA;\n"
                             "#endif\n";

  EXPECT_EQ(CShaderUtils::StripParameterPragmas(source), source);
}

TEST(TestShaderUtils, KeepsACommentedOutParameter)
{
  const std::string source = "//#pragma parameter GAMMA \"Gamma\" 2.2 1.0 3.0 0.1\n"
                             "uniform float GAMMA;\n";

  EXPECT_EQ(CShaderUtils::StripParameterPragmas(source), source);
}
