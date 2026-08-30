/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>

namespace XFILE
{
//
// HDMV navigation command - 12 bytes:
//
//   byte 0:  [operandCount:3][group:2][subGroup:3]
//   byte 1:  [immediate1:1][immediate2:1][reserved:2][branchOption:4]
//   byte 2:  [reserved:4][compareOption:4]
//   byte 3:  [reserved:3][setOption:5]
//   bytes 4-7:   destination operand (big endian)
//   bytes 8-11:  source operand (big endian)
//
// The nibble carrying the opcode depends on the group, so a command cannot be recognised by
// matching its whole first word - the group has to be decoded first.
//
// The same commands appear in MovieObject.bdmv objects and behind the buttons of an interactive
// graphics menu, so both parsers decode them with this.
//
constexpr unsigned int NAVIGATION_COMMAND_SIZE = 12;

enum class HDMV_GROUP : uint8_t
{
  BRANCH = 0,
  COMPARE,
  SET
};

enum class HDMV_BRANCH_SUBGROUP : uint8_t
{
  GOTO = 0,
  JUMP,
  PLAY
};

enum class HDMV_JUMP_OPTION : uint8_t
{
  JUMP_OBJECT = 0,
  JUMP_TITLE,
  CALL_OBJECT,
  CALL_TITLE,
  RESUME
};

enum class HDMV_PLAY_OPTION : uint8_t
{
  PLAY_PLAYLIST = 0,
  PLAY_PLAYLIST_PLAYITEM,
  PLAY_PLAYLIST_PLAYMARK,
  TERMINATE_PLAYLIST,
  LINK_PLAYITEM,
  LINK_PLAYMARK
};

enum class HDMV_SET_SUBGROUP : uint8_t
{
  SET = 0,
  SETSYSTEM
};

//! The operations of the SET sub-group
enum class HDMV_SET_OPERATION : uint8_t
{
  MOVE = 0x01,
  SWAP,
  ADD,
  SUB,
  MUL,
  DIV,
  MOD,
  RND,
  AND,
  OR,
  XOR,
  BITSET,
  BITCLR,
  SHL,
  SHR
};

// An operand is either an immediate value or a register reference. A reference with the top bit set
// is a player status register - the player's own state, which is not known here. The rest are
// general purpose registers.
constexpr uint32_t REGISTER_IS_PSR = 0x80000000;
constexpr uint32_t REGISTER_NUMBER_MASK = 0x00000FFF;
constexpr uint32_t PSR_NUMBER_MASK = 0x0000007F;

constexpr bool IsPlayerStatusRegister(uint32_t operand)
{
  return (operand & REGISTER_IS_PSR) != 0;
}

constexpr bool IsValidRegister(uint32_t operand)
{
  return IsPlayerStatusRegister(operand) ? (operand & ~(REGISTER_IS_PSR | PSR_NUMBER_MASK)) == 0
                                         : (operand & ~REGISTER_NUMBER_MASK) == 0;
}

//! The register an operand names. Only meaningful where IsValidRegister.
constexpr unsigned int RegisterNumber(uint32_t operand)
{
  return operand & (IsPlayerStatusRegister(operand) ? PSR_NUMBER_MASK : REGISTER_NUMBER_MASK);
}

struct NavigationCommand
{
  //! How many of the two operands the command actually carries. One it does not carry is not
  //! there to be read - the bytes it would occupy say nothing.
  uint8_t operandCount{0};

  HDMV_GROUP group{HDMV_GROUP::BRANCH};
  uint8_t subGroup{0};
  uint8_t option{0};
  bool immediateDestination{false};
  bool immediateSource{false};
  uint32_t destination{0};
  uint32_t source{0};

  bool HasDestination() const { return operandCount > 0; }
  bool HasSource() const { return operandCount > 1; }

  bool IsPlayPlaylist() const
  {
    return group == HDMV_GROUP::BRANCH &&
           subGroup == static_cast<uint8_t>(HDMV_BRANCH_SUBGROUP::PLAY) &&
           (option == static_cast<uint8_t>(HDMV_PLAY_OPTION::PLAY_PLAYLIST) ||
            option == static_cast<uint8_t>(HDMV_PLAY_OPTION::PLAY_PLAYLIST_PLAYITEM) ||
            option == static_cast<uint8_t>(HDMV_PLAY_OPTION::PLAY_PLAYLIST_PLAYMARK));
  }

  bool IsPlayPlaylistAtPlayItem() const
  {
    return IsPlayPlaylist() &&
           option == static_cast<uint8_t>(HDMV_PLAY_OPTION::PLAY_PLAYLIST_PLAYITEM);
  }

  bool IsPlayPlaylistAtPlayMark() const
  {
    return IsPlayPlaylist() &&
           option == static_cast<uint8_t>(HDMV_PLAY_OPTION::PLAY_PLAYLIST_PLAYMARK);
  }

  bool IsLinkPlayItem() const
  {
    return group == HDMV_GROUP::BRANCH &&
           subGroup == static_cast<uint8_t>(HDMV_BRANCH_SUBGROUP::PLAY) &&
           option == static_cast<uint8_t>(HDMV_PLAY_OPTION::LINK_PLAYITEM);
  }

  bool IsLinkPlayMark() const
  {
    return group == HDMV_GROUP::BRANCH &&
           subGroup == static_cast<uint8_t>(HDMV_BRANCH_SUBGROUP::PLAY) &&
           option == static_cast<uint8_t>(HDMV_PLAY_OPTION::LINK_PLAYMARK);
  }

  bool IsJumpTitle() const
  {
    return group == HDMV_GROUP::BRANCH &&
           subGroup == static_cast<uint8_t>(HDMV_BRANCH_SUBGROUP::JUMP) &&
           (option == static_cast<uint8_t>(HDMV_JUMP_OPTION::JUMP_TITLE) ||
            option == static_cast<uint8_t>(HDMV_JUMP_OPTION::CALL_TITLE));
  }

  /*! \brief Whether the command branches to another movie object rather than a title. */
  bool IsJumpObject() const
  {
    return group == HDMV_GROUP::BRANCH &&
           subGroup == static_cast<uint8_t>(HDMV_BRANCH_SUBGROUP::JUMP) &&
           (option == static_cast<uint8_t>(HDMV_JUMP_OPTION::JUMP_OBJECT) ||
            option == static_cast<uint8_t>(HDMV_JUMP_OPTION::CALL_OBJECT));
  }

  bool IsSetRegister() const
  {
    return group == HDMV_GROUP::SET && subGroup == static_cast<uint8_t>(HDMV_SET_SUBGROUP::SET);
  }
};

/*! \brief Decode one 12 byte navigation command. */
inline NavigationCommand DecodeNavigationCommand(const std::byte* command)
{
  const auto byte = [command](unsigned int offset)
  { return std::to_integer<uint8_t>(command[offset]); };
  const auto dword = [command](unsigned int offset)
  {
    return std::to_integer<uint32_t>(command[offset + 3]) |
           std::to_integer<uint32_t>(command[offset + 2]) << 8 |
           std::to_integer<uint32_t>(command[offset + 1]) << 16 |
           std::to_integer<uint32_t>(command[offset]) << 24;
  };

  NavigationCommand decoded;
  decoded.operandCount = (byte(0) >> 5) & 0x07;
  decoded.group = static_cast<HDMV_GROUP>((byte(0) >> 3) & 0x03);
  decoded.subGroup = byte(0) & 0x07;
  decoded.immediateDestination = (byte(1) & 0x80) != 0;
  decoded.immediateSource = (byte(1) & 0x40) != 0;
  switch (decoded.group)
  {
    case HDMV_GROUP::BRANCH:
      decoded.option = byte(1) & 0x0F;
      break;
    case HDMV_GROUP::COMPARE:
      decoded.option = byte(2) & 0x0F;
      break;
    case HDMV_GROUP::SET:
      decoded.option = byte(3) & 0x1F;
      break;
  }
  decoded.destination = dword(4);
  decoded.source = dword(8);
  return decoded;
}

/*!
 \brief The general purpose registers of one command sequence, tracked while walking it.

 Playlists and titles are frequently reached indirectly - the sequence moves a number into a
 register and then plays or jumps to the register. Following the moves resolves those, where
 matching a single preceding instruction does not, since the value can arrive through a chain of
 register to register moves.

 This is a linear walk rather than an interpreter, so a register written on both sides of a branch
 holds whichever value was written last. Anything not written by a plain move is left unknown, so
 an unresolved operand is reported as such rather than guessed.
 */
class CRegisterFile
{
public:
  //! \brief Start from values set elsewhere - by a menu button, or by the object that called this one
  void Seed(const std::map<unsigned int, uint32_t>& values) { m_values = values; }

  //! Apply a command's effect on the registers. Anything but a SET/SET leaves them alone.
  void Apply(const NavigationCommand& command)
  {
    if (!command.IsSetRegister())
      return;

    // Every operation of the sub-group takes two operands. One carrying fewer is malformed, and
    // whatever it leaves in the destination cannot be known.
    if (!command.HasSource())
    {
      Store(command.immediateDestination, command.destination, std::nullopt);
      return;
    }

    const std::optional<uint32_t> destination{
        Resolve(command.immediateDestination, command.destination)};
    const std::optional<uint32_t> source{Resolve(command.immediateSource, command.source)};

    // A swap writes both registers. Leaving the source holding what it held before would be wrong
    // rather than merely unknown, so it is written too.
    if (command.option == static_cast<uint8_t>(HDMV_SET_OPERATION::SWAP))
    {
      Store(command.immediateDestination, command.destination, source);
      Store(command.immediateSource, command.source, destination);
      return;
    }

    Store(command.immediateDestination, command.destination,
          Evaluate(command.option, destination, source));
  }

  //! \brief The value of an operand, or nullopt when it is not known.
  std::optional<uint32_t> Resolve(bool immediate, uint32_t operand) const
  {
    if (immediate)
      return operand;

    // A reference naming no register reads nothing, and a player status register holds state this
    // is in no position to know
    if (!IsValidRegister(operand) || IsPlayerStatusRegister(operand))
      return std::nullopt;

    // Only registers a write has been seen to make are known - the rest hold their power-on value
    const auto it = m_values.find(RegisterNumber(operand));
    return it != m_values.end() ? std::optional<uint32_t>{it->second} : std::nullopt;
  }

  //! \brief The command's destination operand, or nullopt where it does not carry one.
  std::optional<uint32_t> ResolveDestination(const NavigationCommand& command) const
  {
    return command.HasDestination()
               ? Resolve(command.immediateDestination, command.destination)
               : std::nullopt;
  }

  /*! \brief Every register this sequence has been seen to set to a known value. */
  const std::map<unsigned int, uint32_t>& GetValues() const { return m_values; }

private:
  //! \brief The result of an operation, or nullopt where it cannot be known.
  static std::optional<uint32_t> Evaluate(uint8_t option,
                                          const std::optional<uint32_t>& destination,
                                          const std::optional<uint32_t>& source)
  {
    // A move is the one operation that does not read the destination first
    if (option == static_cast<uint8_t>(HDMV_SET_OPERATION::MOVE))
      return source;

    if (!destination || !source)
      return std::nullopt;

    const uint32_t d{*destination};
    const uint32_t s{*source};
    constexpr uint32_t SATURATED = 0xFFFFFFFF;

    switch (static_cast<HDMV_SET_OPERATION>(option))
    {
      case HDMV_SET_OPERATION::ADD:
        return static_cast<uint32_t>(
            std::min<uint64_t>(static_cast<uint64_t>(d) + s, SATURATED));
      case HDMV_SET_OPERATION::SUB:
        return d > s ? d - s : 0;
      case HDMV_SET_OPERATION::MUL:
        return static_cast<uint32_t>(
            std::min<uint64_t>(static_cast<uint64_t>(d) * s, SATURATED));
      case HDMV_SET_OPERATION::DIV:
        return s > 0 ? d / s : SATURATED;
      case HDMV_SET_OPERATION::MOD:
        return s > 0 ? d % s : SATURATED;
      case HDMV_SET_OPERATION::AND:
        return d & s;
      case HDMV_SET_OPERATION::OR:
        return d | s;
      case HDMV_SET_OPERATION::XOR:
        return d ^ s;
      case HDMV_SET_OPERATION::BITSET:
        return s < 32 ? std::optional<uint32_t>{d | (1U << s)} : std::nullopt;
      case HDMV_SET_OPERATION::BITCLR:
        return s < 32 ? std::optional<uint32_t>{d & ~(1U << s)} : std::nullopt;
      case HDMV_SET_OPERATION::SHL:
        return s < 32 ? std::optional<uint32_t>{d << s} : std::nullopt;
      case HDMV_SET_OPERATION::SHR:
        return s < 32 ? std::optional<uint32_t>{d >> s} : std::nullopt;

      // A random number is a different one each time the disc is played, so nothing can be said
      // about what it will be
      case HDMV_SET_OPERATION::RND:
      default:
        return std::nullopt;
    }
  }

  //! Write a register, or forget it where the value is not known. Immediates and player status
  //! registers are not written - the player owns the latter.
  void Store(bool immediate, uint32_t operand, const std::optional<uint32_t>& value)
  {
    if (immediate || !IsValidRegister(operand) || IsPlayerStatusRegister(operand))
      return;

    const unsigned int reg{RegisterNumber(operand)};
    if (value)
      m_values[reg] = *value;
    else
      m_values.erase(reg);
  }

  // Sparse - a command sequence touches a handful of the 4096 general purpose registers
  std::map<unsigned int, uint32_t> m_values;
};
} // namespace XFILE
