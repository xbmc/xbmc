#pragma once

#include <string>
#include <sstream>
#include <stdlib.h>

template <class T>
inline std::string intToStr(T i)
{
	std::stringstream stream;
	stream << i;
	return stream.str();
}

inline bool strToBool(const std::string& str)
{
	return str == "true" || atoi(str.c_str()) != 0;
}
