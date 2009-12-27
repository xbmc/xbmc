/*
 * UserTexture.cpp
 *
 *  Created on: Jul 16, 2008
 *      Author: pete
 */

#include "UserTexture.hpp"

UserTexture::UserTexture(std::string qualifiedName): qname(qualifiedName)
{

	if (qualifiedName.substr(0,3) == "fc_")
	{
		name = qualifiedName.substr(3);
		bilinear = true;
		wrap = false;
	}
	else if (qualifiedName.substr(0,3) == "fw_")
	{
		name = qualifiedName.substr(3);
		bilinear = true;
		wrap = true;
	}
	else if (qualifiedName.substr(0,3) == "pc_")
	{
		name = qualifiedName.substr(3);
		bilinear = false;
		wrap = false;
	}
	else if (qualifiedName.substr(0,3) == "pw_")
	{
		name = qualifiedName.substr(3);
		bilinear = false;
		wrap = true;
	}
	else
	{
		name = qualifiedName;
		bilinear = true;
		wrap = true;
	}

	texsizeDefined = false;
}

UserTexture::~UserTexture()
{
	// TODO Auto-generated destructor stub
}
