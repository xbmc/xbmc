#ifndef BASE64_H_
#define BASE64_H_

#include <string>
#include <iostream>

static const std::string base64_chars = 
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789+/";

class CBase64
{
public:
	CBase64();
	virtual ~CBase64();
	
	static std::string Decode(std::string const& strEncodedString);
	static std::string Encode(unsigned char const* bytes_to_encode, unsigned int uiLength);
	
	static inline bool IsBase64(unsigned char c) {
	  return (isalnum(c) || (c == '+') || (c == '/'));
	}
	
	static bool test() {
	  const std::string s = "This is a long string\n that is used\n to test Base64 encoding" ;

	  std::string encoded = Encode(reinterpret_cast<const unsigned char*>(s.c_str()), s.length());
	  std::string decoded = Decode(encoded);

	  std::cout << "encoded: " << encoded << std::endl;
	  std::cout << "decoded: " << decoded << std::endl;
	  
	  return (s == decoded);
	}
	
	
};

#endif /*BASE64_H_*/
