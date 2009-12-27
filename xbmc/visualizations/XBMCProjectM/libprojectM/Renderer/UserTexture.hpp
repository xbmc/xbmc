/*
 * UserTexture.hpp
 *
 *  Created on: Jul 16, 2008
 *      Author: pete
 */

#ifndef USERTEXTURE_HPP_
#define USERTEXTURE_HPP_

#include <string>

class UserTexture
{
public:

	bool wrap;
	bool bilinear;

	bool texsizeDefined;

	int width;
	int height;

	unsigned int texID;

	std::string qname;
	std::string name;

	UserTexture(std::string qualifiedName);
	virtual ~UserTexture();
};

#endif /* USERTEXTURE_HPP_ */
