#pragma once
#include "utils/StdString.h"
#include <boost/functional/hash.hpp>


struct CStdStringHash
  : std::unary_function<CStdString, size_t>
{
  std::size_t operator()(const CStdString& str) const
  {
    size_t seed = 0;
    for (int i = 0; i < str.GetLength(); ++i)
      boost::hash_combine(seed, str.at(i));
    return seed;
  }
};
