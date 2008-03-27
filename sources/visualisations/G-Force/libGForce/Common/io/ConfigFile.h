#ifndef _ConfigFile_
#define _ConfigFile_

class CEgFileSpec;
class ArgList;

class ConfigFile {

	public:
		static bool				Load( const CEgFileSpec* inSpec, ArgList& outArgs );
};

#endif

