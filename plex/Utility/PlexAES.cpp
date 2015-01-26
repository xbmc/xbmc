#include "PlexAES.h"
#include "log.h"
#include "File.h"
#include "Base64.h"

#include <boost/foreach.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<std::string> CPlexAES::chunkData(const std::string& data)
{
  std::vector<std::string> chunks;
  int i = 0;

  while (true)
  {
    std::string chunk = data.substr(i, AES_BLOCK_SIZE);
    chunks.push_back(chunk);
    if (chunk.length() < AES_BLOCK_SIZE)
      break;

    i += AES_BLOCK_SIZE;
  }

  return chunks;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string CPlexAES::encrypt(const std::string &data)
{
  if (data.empty()) {
    return "";
  }

  unsigned char block[AES_BLOCK_SIZE], buffer[AES_BLOCK_SIZE];
  std::string outData;

  BOOST_FOREACH(const std::string& chunk, chunkData(data))
  {
    strncpy((char *)&block[0], chunk.c_str(), chunk.length());
    if (aes_encrypt(block, buffer, &m_encryptCtx) == EXIT_FAILURE)
    {
      CLog::Log(LOGWARNING, "CPlexAES::encrypt failed to encrypt data...");
      return "";
    }

    outData.append((const char*)buffer, chunk.length());
  }

  return outData;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string CPlexAES::decrypt(const std::string &data)
{
  if (data.empty()) {
    return "";
  }

  std::string outData;
  unsigned char block[AES_BLOCK_SIZE], buffer[AES_BLOCK_SIZE];

  BOOST_FOREACH(const std::string& chunk, chunkData(data))
  {
    strncpy((char *)&block[0], chunk.c_str(), chunk.length());
    if (aes_decrypt(block, buffer, &m_decryptCtx) == EXIT_FAILURE)
    {
      CLog::Log(LOGWARNING, "CPlexAES::decrypt failed to decrypt data...");
      return "";
    }

    outData.append((const char*)buffer, chunk.length());
  }

  return outData;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string CPlexAES::decryptFile(const std::string &url)
{
  XFILE::CFile file;

  if (file.Open(url))
  {
    char buffer[4096];
    std::string outData;
    while (int r = file.Read(buffer, 4096))
    {
      outData.append(buffer, r);
      if (r < 4096)
        break;
    }
    file.Close();

    return decrypt(Base64::Decode(outData));
  }

  return "";
}
