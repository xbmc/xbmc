#pragma once

#include <string>
#include <sstream>

inline std::string intToStr(int i)
{
	std::stringstream stream;
	stream << i;
	return stream.str();
}


