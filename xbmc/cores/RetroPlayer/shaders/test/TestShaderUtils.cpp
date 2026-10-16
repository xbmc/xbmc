/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/shaders/ShaderUtils.h"

#include <gtest/gtest.h>

using namespace KODI::SHADER;

TEST(TestShaderUtils, StripParameterPragmas)
{
  EXPECT_EQ(CShaderUtils::StripParameterPragmas("#pragma parameter A \"A\" 1.0 0.0 2.0 1.0\n"
                                                "  #pragma parameter B \"B\" 1.0 0.0 2.0 1.0\n"
                                                "uniform float A;\n"
                                                "#pragma parameter C \"C\" 1.0 0.0 2.0 1.0"),
            "uniform float A;\n");
}

TEST(TestShaderUtils, StripParameterPragmasKeepsComments)
{
  const std::string source = "#ifdef PARAMETER_UNIFORM // understands #pragma parameters\n"
                             "uniform float A;\n"
                             "//#pragma parameter B \"B\" 1.0 0.0 2.0 1.0\n"
                             "uniform float B;\n";

  EXPECT_EQ(CShaderUtils::StripParameterPragmas(source), source);
}
