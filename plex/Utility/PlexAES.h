#ifndef __PLEX_AES_H__
#define __PLEX_AES_H__

#include "Third-Party/aes/aes.h"
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexAES
{
public:
  CPlexAES(const std::string& key) : m_key(key)
  {
    aes_init();
    
    std::string realKey = key;
    if (realKey.length() > 32)
      realKey = key.substr(0, 32);
      
    aes_encrypt_key((const unsigned char*)realKey.c_str(), realKey.length(), &m_encryptCtx);
    aes_decrypt_key((const unsigned char*)realKey.c_str(), realKey.length(), &m_decryptCtx);
  }
  
  std::string encrypt(const std::string& data);
  std::string decrypt(const std::string& data);
  
  std::string decryptFile(const std::string& url);

private:
  std::vector<std::string> chunkData(const std::string& data);
  
  std::string m_key;
  aes_encrypt_ctx m_encryptCtx;
  aes_decrypt_ctx m_decryptCtx;
};

#endif