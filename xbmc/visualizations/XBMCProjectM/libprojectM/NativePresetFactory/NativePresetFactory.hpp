//
// C++ Interface: NativePresetFactory
//
// Description: 
//
//
// Author: Carmelo Piccione <carmelo.piccione@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef __NATIVE_PRESET_FACTORY_HPP
#define __NATIVE_PRESET_FACTORY_HPP

#include <memory>
#include "PresetFactory.hpp"

class PresetLibrary;

class NativePresetFactory : public PresetFactory {

public:

 NativePresetFactory();

 virtual ~NativePresetFactory();

 virtual std::auto_ptr<Preset> allocate(const std::string & url, const std::string & name = std::string(), 
	const std::string & author = std::string());

 virtual std::string supportedExtensions() const { return "so"; }

private:
	PresetLibrary * loadLibrary(const std::string & url);
	typedef std::map<std::string, PresetLibrary*> PresetLibraryMap;
	PresetLibraryMap _libraries;
	
};

#endif
