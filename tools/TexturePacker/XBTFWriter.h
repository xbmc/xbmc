#ifndef XBTFWRITER_H_
#define XBTFWRITER_H_

#include <string>
#include <stdio.h>
#include "XBTF.h"

class CXBTFWriter
{
public:
  CXBTFWriter(CXBTF& xbtf, const std::string outputFile);
  bool Create();
  bool Close();
  bool AppendContent(unsigned char const* data, size_t length);
  bool UpdateHeader();

private:
  CXBTF& m_xbtf;
  std::string m_outputFile;
  FILE* m_file;
  FILE* m_tempFile;
};

#endif
