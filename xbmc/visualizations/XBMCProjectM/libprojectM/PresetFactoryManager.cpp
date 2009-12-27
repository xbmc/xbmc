//
// C++ Implementation: PresetFactoryManager
//
// Description: 
//
//
// Author: Carmelo Piccione <carmelo.piccione@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "PresetFactoryManager.hpp"

#ifndef DISABLE_MILKDROP_PRESETS
#include "MilkdropPresetFactory/MilkdropPresetFactory.hpp"
#endif

#ifndef DISABLE_NATIVE_PRESETS
#include "NativePresetFactory/NativePresetFactory.hpp"
#endif

#include <sstream>
PresetFactoryManager::PresetFactoryManager() : _gx(0), _gy(0) {}

PresetFactoryManager::~PresetFactoryManager() {
	for (std::vector<PresetFactory *>::iterator pos = _factoryList.begin(); 
		pos != _factoryList.end(); ++pos) {
		assert(*pos);
		delete(*pos);
	}


}
void PresetFactoryManager::initialize(int gx, int gy) {
	_gx = gx;
	_gy = gy;
	PresetFactory * factory;
	
	#ifndef DISABLE_MILKDROP_PRESETS
	factory = new MilkdropPresetFactory(_gx, _gy);
	registerFactory(factory->supportedExtensions(), factory);		
	#endif
	
	#ifndef DISABLE_NATIVE_PRESETS
	factory = new NativePresetFactory();
	registerFactory(factory->supportedExtensions(), factory);
	#endif
}

// Current behavior if a conflict is occurs is to override the previous request

void PresetFactoryManager::registerFactory(const std::string & extensions, PresetFactory * factory) {
	
	std::stringstream ss(extensions);	
	std::string extension;

	_factoryList.push_back(factory);

	while (ss >> extension) {
		if (_factoryMap.count(extension)) {
			std::cerr << "[PresetFactoryManager] Warning: extension \"" << extension << 
				"\" already has a factory. New factory handler ignored." << std::endl;			
		} else {
			_factoryMap.insert(std::make_pair(extension, factory));			
		}
	}
}

PresetFactory & PresetFactoryManager::factory(const std::string & extension) {

	if (!_factoryMap.count(extension)) {		
		std::ostringstream os;
		os << "No factory associated with \"" << extension << "\"." << std::endl;
		throw PresetFactoryException(os.str());
	}
	return *_factoryMap[extension];
}

bool PresetFactoryManager::extensionHandled(const std::string & extension) const {		
	return _factoryMap.count(extension);
}
