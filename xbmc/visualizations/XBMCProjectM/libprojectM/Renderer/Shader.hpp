/*
 * Shader.hpp
 *
 *  Created on: Jun 29, 2008
 *      Author: pete
 */

#ifndef SHADER_HPP_
#define SHADER_HPP_

#include <string>
#include <map>
#include "UserTexture.hpp"

class Shader
{
public:

    std::map<std::string, UserTexture*> textures;

    bool enabled;

	std::string programSource;

	Shader();
};

#endif /* SHADER_HPP_ */
