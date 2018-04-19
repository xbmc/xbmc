/*
 *      Copyright (C) 2018 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Digest.h"
#include "StringUtils.h"

#include <openssl/evp.h>

#include <array>
#include <stdexcept>

namespace KODI
{
namespace UTILITY
{

namespace
{

EVP_MD const * TypeToEVPMD(CDigest::Type type)
{
  switch (type)
  {
    case CDigest::Type::MD5:
      return EVP_md5();
    case CDigest::Type::SHA1:
      return EVP_sha1();
    case CDigest::Type::SHA256:
      return EVP_sha256();
    case CDigest::Type::SHA512:
      return EVP_sha512();
    default:
      throw std::invalid_argument("Unknown digest type");
  }
}

}

std::ostream& operator<<(std::ostream& os, TypedDigest const& digest)
{
  return os << "{" << CDigest::TypeToString(digest.type) << "}" << digest.value;
}

std::string CDigest::TypeToString(Type type)
{
  switch (type)
  {
    case Type::MD5:
      return "md5";
    case Type::SHA1:
      return "sha1";
    case Type::SHA256:
      return "sha256";
    case Type::SHA512:
      return "sha512";
    case Type::INVALID:
      return "invalid";
    default:
      throw std::invalid_argument("Unknown digest type");
  }
}

CDigest::Type CDigest::TypeFromString(std::string const& type)
{
  std::string typeLower{type};
  StringUtils::ToLower(typeLower);
  if (type == "md5")
  {
    return Type::MD5;
  }
  else if (type == "sha1")
  {
    return Type::SHA1;
  }
  else if (type == "sha256")
  {
    return Type::SHA256;
  }
  else if (type == "sha512")
  {
    return Type::SHA512;
  }
  else
  {
    throw std::invalid_argument(std::string("Unknown digest type \"") + type + "\"");
  }
}

void CDigest::MdCtxDeleter::operator()(EVP_MD_CTX* context)
{
  EVP_MD_CTX_destroy(context);
}

CDigest::CDigest(Type type)
: m_context{EVP_MD_CTX_create()}, m_md(TypeToEVPMD(type))
{
  if (1 != EVP_DigestInit_ex(m_context.get(), m_md, nullptr))
  {
    throw std::runtime_error("EVP_DigestInit_ex failed");
  }
}

void CDigest::Update(std::string const& data)
{
  Update(data.c_str(), data.size());
}

void CDigest::Update(void const* data, std::size_t size)
{
  if (m_finalized)
  {
    throw std::logic_error("Finalized digest cannot be updated any more");
  }

  if (1 != EVP_DigestUpdate(m_context.get(), data, size))
  {
    throw std::runtime_error("EVP_DigestUpdate failed");
  }
}

std::string CDigest::FinalizeRaw()
{
  if (m_finalized)
  {
    throw std::logic_error("Digest can only be finalized once");
  }

  m_finalized = true;

  std::array<unsigned char, 64> digest;
  std::size_t size = EVP_MD_size(m_md);
  if (size > digest.size())
  {
    throw std::runtime_error("Digest unexpectedly long");
  }
  if (1 != EVP_DigestFinal_ex(m_context.get(), digest.data(), nullptr))
  {
    throw std::runtime_error("EVP_DigestFinal_ex failed");
  }
  return {reinterpret_cast<char*> (digest.data()), size};
}

std::string CDigest::Finalize()
{
  return StringUtils::ToHexadecimal(FinalizeRaw());
}

std::string CDigest::Calculate(Type type, std::string const& data)
{
  CDigest digest{type};
  digest.Update(data);
  return digest.Finalize();
}

std::string CDigest::Calculate(Type type, void const* data, std::size_t size)
{
  CDigest digest{type};
  digest.Update(data, size);
  return digest.Finalize();
}

}
}
