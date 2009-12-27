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
#ifndef __PRESET_FACTORY_MANAGER_HPP
#define __PRESET_FACTORY_MANAGER_HPP
#include "PresetFactory.hpp"

/// A simple exception class to strongly type all preset factory related issues
class PresetFactoryException : public std::exception
{
	public:
		inline PresetFactoryException(const std::string & message) : _message(message) {}
		virtual ~PresetFactoryException() throw() {}
		const std::string & message() const { return _message; } 

	private:	
		std::string _message;
};

/// A manager of preset factories
class PresetFactoryManager {

	public:
		PresetFactoryManager();
		~PresetFactoryManager();

		/// Initializes the manager with mesh sizes specified
		/// \param gx the width of the mesh
		/// \param gy the height of the mesh
		/// \note This must be called once before any other methods
		void initialize(int gx, int gy);
		
		/// Requests a factory given a preset extension type
		/// \param extension a string denoting the preset suffix type
		/// \throws PresetFactoryException if the extension is unhandled
		/// \returns a valid preset factory associated with the extension
		PresetFactory & factory(const std::string & extension);

		/// Tests if an extension has been registered with a factory
		/// \param extension the file name extension to verify
		/// \returns true if a factory exists, false otherwise
		bool extensionHandled(const std::string & extension) const;

	private:
		int _gx, _gy;				
		mutable std::map<std::string, PresetFactory *> _factoryMap;
		mutable std::vector<PresetFactory *> _factoryList;
		void registerFactory(const std::string & extension, PresetFactory * factory);

};
#endif
