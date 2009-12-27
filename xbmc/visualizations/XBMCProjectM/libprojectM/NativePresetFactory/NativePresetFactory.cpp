//
// C++ Implementation: NativePresetFactory
//
// Description: 
//
//
// Author: Carmelo Piccione <carmelo.piccione@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <dlfcn.h>
#include "NativePresetFactory.hpp"

typedef void Handle;
typedef void DestroyFunctor(Preset*);
typedef Preset * CreateFunctor(const char * url);

class LibraryPreset : public Preset {
public:
	LibraryPreset(Preset * preset, DestroyFunctor * destroyFun) : Preset(preset->name(), preset->author()), _internalPreset(preset), _destroyFunctor(destroyFun) {}
	inline Pipeline & pipeline() { return _internalPreset->pipeline(); }
	inline virtual ~LibraryPreset() { _destroyFunctor(_internalPreset); }
	inline void Render(const BeatDetect &music, const PipelineContext &context) {
		return _internalPreset->Render(music, context);
	}
private:
	Preset * _internalPreset;
	DestroyFunctor * _destroyFunctor;
};

class PresetLibrary {

	public:
		PresetLibrary(Handle * h, CreateFunctor * create, DestroyFunctor * destroy) :
			_handle(h), _createFunctor(create), _destroyFunctor(destroy) {}

		Handle * handle() { return _handle; }
		CreateFunctor * createFunctor() { return _createFunctor; }
		DestroyFunctor * destroyFunctor() { return _destroyFunctor; }

		~PresetLibrary() {
			dlclose(handle());
		}

	private:
		Handle * _handle; 
		CreateFunctor * _createFunctor;
		DestroyFunctor * _destroyFunctor;
	
};

NativePresetFactory::NativePresetFactory() {}

NativePresetFactory::~NativePresetFactory() {

for (PresetLibraryMap::iterator pos = _libraries.begin(); pos != _libraries.end(); ++pos) {
	std::cerr << "deleting preset library" << std::endl;
	delete(pos->second);
}


}

PresetLibrary * NativePresetFactory::loadLibrary(const std::string & url) {

    if (_libraries.count(url))
	return _libraries[url];

    // load the preset library
    void* handle = dlopen(url.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "[NativePresetFactory] Cannot load library: " << dlerror() << '\n';
        return 0;
    }

    // reset errors
    dlerror();

    // load the symbols
    CreateFunctor * create = (CreateFunctor*) dlsym(handle, "create");
    const char * dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "[NativePresetFactory] Cannot load symbol create: " << dlsym_error << '\n';
        return 0;
    }

    DestroyFunctor * destroy = (DestroyFunctor*) dlsym(handle, "destroy");
    dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "[NativePresetFactory] Cannot load symbol destroy: " << dlsym_error << '\n';
        return 0;
    }

    std::cerr << "[NativePresetFactory] creating preset library from url " << url << std::endl;

    PresetLibrary * library = new PresetLibrary(handle, create, destroy);

    _libraries.insert(std::make_pair(url, library));
    return library;
}


std::auto_ptr<Preset> NativePresetFactory::allocate
	(const std::string & url, const std::string & name, const std::string & author) {
		
	PresetLibrary * library;
	
	if ((library = loadLibrary(url)) == 0)
		return std::auto_ptr<Preset>(0);
	
	return std::auto_ptr<Preset>(new LibraryPreset
		(library->createFunctor()(url.c_str()), library->destroyFunctor()));
		 
}
