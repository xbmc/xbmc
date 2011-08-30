#include <Foundation/Foundation.h>

#include "StandardDirs.h"

std::string StandardDirs::applicationSupportFolderPath()
{
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,
	                   NSUserDomainMask,
	                   true /* expand tildes */);

	for (unsigned int i=0; i < [paths count]; i++)
	{
		NSString* path = [paths objectAtIndex:i];
		return std::string([path UTF8String]);
	}
	return std::string();
}

