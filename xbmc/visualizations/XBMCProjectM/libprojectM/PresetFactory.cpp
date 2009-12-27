#include "PresetFactory.hpp"

const std::string PresetFactory::IDLE_PRESET_PROTOCOL("idle");

std::string PresetFactory::protocol(const std::string & url, std::string & path) {

#ifdef __APPLE__
	// NOTE: Brian changed this from url.find_first_of to url.find, since presumably we want to find the first occurence of
	// :// and not the first occurence of any colon or forward slash.  At least that fixed a bug in the Mac OS X build.
	std::size_t pos = url.find("://");
#else
	std::size_t pos = url.find_first_of("://");
#endif
	if (pos == std::string::npos)
		return std::string();
	else {
		path = url.substr(pos + 3, url.length());
		std::cout << "[PresetFactory] path is " << path << std::endl;
		std::cout << "[PresetFactory] url is " << url << std::endl;
		return url.substr(0, pos);
	}

}

