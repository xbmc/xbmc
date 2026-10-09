/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/bluray/NavigationCommand.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

using namespace XFILE;

namespace
{
// The 12 bytes of a navigation command, as MovieObject.bdmv stores them. The layout follows
// libbluray's mobj_parse_cmd, against which these expectations were checked.
std::array<std::byte, NAVIGATION_COMMAND_SIZE> MakeCommand(unsigned int operandCount,
                                                           HDMV_GROUP group,
                                                           uint8_t subGroup,
                                                           uint8_t option,
                                                           bool immediateDestination,
                                                           bool immediateSource,
                                                           uint32_t destination,
                                                           uint32_t source)
{
  std::array<std::byte, NAVIGATION_COMMAND_SIZE> command{};
  const auto set{[&command](unsigned int index, unsigned int value)
                 { command[index] = static_cast<std::byte>(value); }};

  set(0, (operandCount & 0x07) << 5 | (static_cast<unsigned int>(group) & 0x03) << 3 |
             (subGroup & 0x07));
  set(1, (immediateDestination ? 0x80 : 0) | (immediateSource ? 0x40 : 0) |
             (group == HDMV_GROUP::BRANCH ? option & 0x0F : 0));
  set(2, group == HDMV_GROUP::COMPARE ? option & 0x0F : 0);
  set(3, group == HDMV_GROUP::SET ? option & 0x1F : 0);
  for (unsigned int i = 0; i < 4; ++i)
  {
    set(4 + i, (destination >> (24 - 8 * i)) & 0xFF);
    set(8 + i, (source >> (24 - 8 * i)) & 0xFF);
  }
  return command;
}

NavigationCommand SetCommand(HDMV_SET_OPERATION operation,
                             uint32_t destination,
                             uint32_t source,
                             bool immediateSource = true,
                             unsigned int operandCount = 2)
{
  const auto bytes{
      MakeCommand(operandCount, HDMV_GROUP::SET, static_cast<uint8_t>(HDMV_SET_SUBGROUP::SET),
                  static_cast<uint8_t>(operation), false, immediateSource, destination, source)};
  return DecodeNavigationCommand(bytes.data());
}

//! The value left in general purpose register 0 after moving seed into it and then operating
std::optional<uint32_t> Operate(HDMV_SET_OPERATION operation, uint32_t seed, uint32_t operand)
{
  CRegisterFile registers;
  registers.Apply(SetCommand(HDMV_SET_OPERATION::MOVE, 0, seed));
  registers.Apply(SetCommand(operation, 0, operand));
  return registers.Resolve(false, 0);
}
} // namespace

TEST(TestNavigationCommand, Decode_FieldsFollowTheBitLayout)
{
  const auto bytes{MakeCommand(
      2, HDMV_GROUP::BRANCH, static_cast<uint8_t>(HDMV_BRANCH_SUBGROUP::PLAY),
      static_cast<uint8_t>(HDMV_PLAY_OPTION::PLAY_PLAYLIST), true, false, 0x12345678, 0x9ABCDEF0)};
  const NavigationCommand command{DecodeNavigationCommand(bytes.data())};

  EXPECT_EQ(command.operandCount, 2);
  EXPECT_EQ(command.group, HDMV_GROUP::BRANCH);
  EXPECT_EQ(command.subGroup, static_cast<uint8_t>(HDMV_BRANCH_SUBGROUP::PLAY));
  EXPECT_TRUE(command.immediateDestination);
  EXPECT_FALSE(command.immediateSource);
  EXPECT_EQ(command.destination, 0x12345678u);
  EXPECT_EQ(command.source, 0x9ABCDEF0u);
  EXPECT_TRUE(command.IsPlayPlaylist());
}

TEST(TestNavigationCommand, Decode_OptionComesFromTheGroupsOwnByte)
{
  // The opcode does not live in the same nibble for every group
  const auto set{MakeCommand(2, HDMV_GROUP::SET, static_cast<uint8_t>(HDMV_SET_SUBGROUP::SET),
                             static_cast<uint8_t>(HDMV_SET_OPERATION::ADD), false, true, 0, 1)};
  EXPECT_EQ(DecodeNavigationCommand(set.data()).option,
            static_cast<uint8_t>(HDMV_SET_OPERATION::ADD));

  const auto compare{MakeCommand(2, HDMV_GROUP::COMPARE, 0, 0x05, false, true, 0, 1)};
  EXPECT_EQ(DecodeNavigationCommand(compare.data()).option, 0x05);
}

TEST(TestNavigationCommand, Registers_OnlyValidReferencesNameARegister)
{
  EXPECT_TRUE(IsValidRegister(0x00000FFF)); // the highest general purpose register
  EXPECT_FALSE(IsValidRegister(0x00001000)); // a bit outside the mask names no register
  EXPECT_TRUE(IsValidRegister(0x8000007F)); // the highest player status register
  EXPECT_FALSE(IsValidRegister(0x80000080));

  EXPECT_FALSE(IsPlayerStatusRegister(0x0000000F));
  EXPECT_TRUE(IsPlayerStatusRegister(0x8000000F));
  EXPECT_EQ(RegisterNumber(0x8000000F), 0x0Fu);
}

TEST(TestNavigationCommand, Resolve_ImmediateAndUnwrittenRegisters)
{
  const CRegisterFile registers;
  EXPECT_EQ(registers.Resolve(true, 42), 42u); // an immediate is its own value
  EXPECT_FALSE(registers.Resolve(false, 7)); // never written, so its value is unknown
  EXPECT_FALSE(registers.Resolve(false, 0x80000004)); // the player owns its own state
  EXPECT_FALSE(registers.Resolve(false, 0x00001000)); // names no register
}

TEST(TestNavigationCommand, Resolve_DestinationHonoursTheOperandCount)
{
  CRegisterFile registers;
  registers.Apply(SetCommand(HDMV_SET_OPERATION::MOVE, 0, 900));

  // Carrying no operands, the bytes a destination would occupy say nothing
  const auto none{
      MakeCommand(0, HDMV_GROUP::BRANCH, static_cast<uint8_t>(HDMV_BRANCH_SUBGROUP::PLAY),
                  static_cast<uint8_t>(HDMV_PLAY_OPTION::PLAY_PLAYLIST), false, false, 0, 0)};
  EXPECT_FALSE(registers.ResolveDestination(DecodeNavigationCommand(none.data())));

  const auto one{
      MakeCommand(1, HDMV_GROUP::BRANCH, static_cast<uint8_t>(HDMV_BRANCH_SUBGROUP::PLAY),
                  static_cast<uint8_t>(HDMV_PLAY_OPTION::PLAY_PLAYLIST), false, false, 0, 0)};
  EXPECT_EQ(registers.ResolveDestination(DecodeNavigationCommand(one.data())), 900u);
}

TEST(TestNavigationCommand, Apply_Move)
{
  CRegisterFile registers;
  registers.Apply(SetCommand(HDMV_SET_OPERATION::MOVE, 3, 800));
  EXPECT_EQ(registers.Resolve(false, 3), 800u);
}

TEST(TestNavigationCommand, Apply_SwapWritesBothRegisters)
{
  CRegisterFile registers;
  registers.Apply(SetCommand(HDMV_SET_OPERATION::MOVE, 1, 100));
  registers.Apply(SetCommand(HDMV_SET_OPERATION::MOVE, 2, 200));

  // Register to register, so the source is not an immediate
  registers.Apply(SetCommand(HDMV_SET_OPERATION::SWAP, 1, 2, false));

  EXPECT_EQ(registers.Resolve(false, 1), 200u);
  EXPECT_EQ(registers.Resolve(false, 2), 100u); // must not keep what it held before the swap
}

TEST(TestNavigationCommand, Apply_ArithmeticFollowsLibbluray)
{
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::ADD, 10, 5), 15u);
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::ADD, 0xFFFFFFFF, 1), 0xFFFFFFFFu); // saturates
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::SUB, 10, 5), 5u);
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::SUB, 5, 10), 0u); // clamps rather than wrapping
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::MUL, 0x10000, 0x10000), 0xFFFFFFFFu); // saturates
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::DIV, 10, 3), 3u);
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::DIV, 10, 0), 0xFFFFFFFFu); // no answer, so libbluray's
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::MOD, 10, 3), 1u);
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::MOD, 10, 0), 0xFFFFFFFFu);
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::AND, 0xF0, 0x3C), 0x30u);
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::OR, 0xF0, 0x0F), 0xFFu);
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::XOR, 0xFF, 0x0F), 0xF0u);
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::BITSET, 0, 4), 0x10u);
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::BITCLR, 0xFF, 4), 0xEFu);
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::SHL, 1, 4), 0x10u);
  EXPECT_EQ(Operate(HDMV_SET_OPERATION::SHR, 0x10, 4), 1u);
}

TEST(TestNavigationCommand, Apply_UnknowableResultsAreForgotten)
{
  // A random number differs every time the disc is played
  EXPECT_FALSE(Operate(HDMV_SET_OPERATION::RND, 10, 5));

  // A shift wider than the register has no defined answer
  EXPECT_FALSE(Operate(HDMV_SET_OPERATION::SHL, 1, 32));

  // Operating on a register that was never written cannot give a known result, and must not leave
  // the earlier value in place
  CRegisterFile registers;
  registers.Apply(SetCommand(HDMV_SET_OPERATION::MOVE, 0, 50));
  registers.Apply(SetCommand(HDMV_SET_OPERATION::ADD, 0, 9, false)); // register 9, never written
  EXPECT_FALSE(registers.Resolve(false, 0));
}

TEST(TestNavigationCommand, Apply_PlayerStatusRegistersAreNotWritten)
{
  CRegisterFile registers;
  registers.Apply(SetCommand(HDMV_SET_OPERATION::MOVE, 0x80000004, 7));
  EXPECT_FALSE(registers.Resolve(false, 0x80000004));
  EXPECT_TRUE(registers.GetValues().empty());
}
