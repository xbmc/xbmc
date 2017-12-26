#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <system_error>

namespace KODI
{
namespace PLATFORM
{
namespace FILESYSTEM
{
enum class file_type
{
  none,      //!< The type of the file has not been determined or an error occurred while trying to
             //!< determine the type.
  not_found, //!< Pseudo-type indicating the file was not found.
  regular,   //!< Regular file
  directory, //!< Directory file
  symlink,   //!< Symbolic link file
  block,     //!< Block special file
  character, //!< Character special file
  fifo,      //!< FIFO or pipe special file
  socket,    //!< Socket file
  unknown    //!< The file exists but the type could not be determined
};

enum class perms : int
{
  none = 0,           //!<		There are no permissions set for the file.
  owner_read = 0400,  //!<	S_IRUSR	Read permission, owner
  owner_write = 0200, //!<	S_IWUSR	Write permission, owner
  owner_exec = 0100,  //!<	S_IXUSR	Execute / search permission, owner
  owner_all = 0700,   //!<	S_IRWXU	Read, write, execute / search by owner; owner_read |
                      //!<owner_write | owner_exec
  group_read = 040,   //!<	S_IRGRP	Read permission, group
  group_write = 020,  //!<	S_IWGRP	Write permission, group
  group_exec = 010,   //!<	S_IXGRP	Execute / search permission, group
  group_all = 070,   //!<	S_IRWXG	Read, write, execute / search by group; group_read | group_write |
                     //!<group_exec
  others_read = 04,  //!<	S_IROTH	Read permission, others
  others_write = 02, //!<	S_IWOTH	Write permission, others
  others_exec = 01,  //!<	S_IXOTH	Execute / search permission, others
  others_all = 07, //!<	S_IRWXO	Read, write, execute / search by others; others_read | others_write
                   //!<| others_exec
  all = 0777,      //!<		owner_all | group_all | others_all
  set_uid = 04000, //!<	S_ISUID	Set - user - ID on execution
  set_gid = 02000, //!<	S_ISGID	Set - group - ID on execution
  sticky_bit = 01000, //!<	S_ISVTX	Operating system dependent.
  mask = 07777,       //!<		all | set_uid | set_gid | sticky_bit
  unknown = 0xFFFF,   //!< The permissions are not known, such as when a file_status object is
                      // created without specifying the permissions
};

constexpr inline perms operator&(perms X, perms Y)
{
  return static_cast<perms>(static_cast<int>(X) & static_cast<int>(Y));
}

constexpr inline perms operator|(perms X, perms Y)
{
  return static_cast<perms>(static_cast<int>(X) | static_cast<int>(Y));
}

constexpr inline perms operator^(perms X, perms Y)
{
  return static_cast<perms>(static_cast<int>(X) ^ static_cast<int>(Y));
}

constexpr inline perms operator~(perms X)
{
  return static_cast<perms>(~static_cast<int>(X));
}

inline perms &operator&=(perms &X, perms Y)
{
  X = X & Y;
  return X;
}

inline perms &operator|=(perms &X, perms Y)
{
  X = X | Y;
  return X;
}

inline perms &operator^=(perms &X, perms Y)
{
  X = X ^ Y;
  return X;
}

class filesystem_error : public std::system_error
{
public:
  filesystem_error(const std::string &what_arg, std::error_code ec);
  filesystem_error(const std::string &what_arg, const std::string &p1, std::error_code ec);
  filesystem_error(const std::string &what_arg,
                   const std::string &p1,
                   const std::string &p2,
                   std::error_code ec);

  const std::string &path1() const noexcept;
  const std::string &path2() const noexcept;
  const char *what() const noexcept override;

private:
  std::string m_what;
  std::string m_path1;
  std::string m_path2;
};

class file_status
{
public:
  file_status() noexcept;
  explicit file_status(file_type ft) noexcept;
  explicit file_status(file_type ft, perms prms) noexcept;

  file_status(const file_status &) noexcept;
  file_status(file_status &&) noexcept;
  ~file_status();

  file_status &operator=(const file_status &) noexcept;
  file_status &operator=(file_status &&) noexcept;

  void type(file_type ft) noexcept;
  void permissions(perms prms) noexcept;
  file_type type() const noexcept;
  perms permissions() const noexcept;

private:
  file_type m_type;
  perms m_perms;
};

struct space_info
{
  std::uintmax_t capacity;
  std::uintmax_t free;
  std::uintmax_t available;
};

space_info space(const std::string &path, std::error_code &ec);
}
}
}