/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <openssl/evp.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "StringUtils.h"

namespace KODI
{
namespace UTILITY
{

/**
 * Utility class for calculating message digests/hashes, currently using OpenSSL
 */
class CDigest
{
public:
  enum class Type
  {
    MD5,
    SHA1,
    SHA256,
    SHA512,
    INVALID
  };

  /**
   * Convert type enumeration value to lower-case string representation
   */
  static std::string TypeToString(Type type);
  /**
   * Convert digest type string representation to enumeration value
   */
  static Type TypeFromString(std::string const& type);

  /**
   * Create a digest calculation object
   */
  CDigest(Type type);
  /**
   * Update digest with data
   *
   * Cannot be called after \ref Finalize has been called
   */
  void Update(std::string const& data);
  /**
   * Update digest with data
   *
   * Cannot be called after \ref Finalize has been called
   */
  void Update(void const* data, std::size_t size);
  /**
   * Finalize and return the digest
   *
   * The digest object cannot be used any more after this function
   * has been called.
   *
   * \return digest value as string in lower-case hexadecimal notation
   */
  std::string Finalize();
  /**
   * Finalize and return the digest
   *
   * The digest object cannot be used any more after this
   * function has been called.
   *
   * \return digest value as binary std::string
   */
  std::string FinalizeRaw();

  /**
   * Calculate message digest
   */
  static std::string Calculate(Type type, std::string const& data);
  /**
   * Calculate message digest
   */
  static std::string Calculate(Type type, void const* data, std::size_t size);

private:
  struct MdCtxDeleter
  {
    void operator()(EVP_MD_CTX* context);
  };

  bool m_finalized{false};
  std::unique_ptr<EVP_MD_CTX, MdCtxDeleter> m_context;
  EVP_MD const* m_md;
};

struct TypedDigest
{
  CDigest::Type type{CDigest::Type::INVALID};
  std::string value;

  TypedDigest()
  {}

  TypedDigest(CDigest::Type type, std::string const& value)
  : type(type), value(value)
  {}

  bool Empty() const
  {
    return (type == CDigest::Type::INVALID || value.empty());
  }
};

inline bool operator==(TypedDigest const& left, TypedDigest const& right)
{
  if (left.type != right.type)
  {
    throw std::logic_error("Cannot compare digests of different type");
  }
  return StringUtils::EqualsNoCase(left.value, right.value);
}

inline bool operator!=(TypedDigest const& left, TypedDigest const& right)
{
  return !(left == right);
}

std::ostream& operator<<(std::ostream& os, TypedDigest const& digest);

}
}
