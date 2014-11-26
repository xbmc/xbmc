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
    std::string chunk = data.substr(i, 16);
    chunks.push_back(chunk);
    if (chunk.length() < 16)
      break;
    
    i += 16;
  }

  return chunks;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string CPlexAES::encrypt(const std::string &data)
{
  std::vector<std::string> chunks = chunkData(data);
  
  std::string outData;
  BOOST_FOREACH(const std::string& chunk, chunks)
  {
    unsigned char buffer[16];
    memset(buffer, '\0', 16);
    
    if (aes_encrypt((const unsigned char*)chunk.c_str(), buffer, &m_encryptCtx) == EXIT_FAILURE)
    {
      CLog::Log(LOGWARNING, "CPlexAES::encrypt failed to encrypt data...");
      return "";
    }
    
    outData.append((char*)buffer, 16);
  }

  return outData;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string CPlexAES::decrypt(const std::string &data)
{
  std::vector<std::string> chunks = chunkData(data);
  
  std::string outData;
  BOOST_FOREACH(const std::string& chunk, chunks)
  {
    unsigned char buffer[16];
    memset(buffer, '\0', 16);
    
    if (aes_decrypt((const unsigned char*)chunk.c_str(), buffer, &m_decryptCtx) == EXIT_FAILURE)
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
    
    return decrypt(Base64::Decode(outData));
  }
  
  return "";
}