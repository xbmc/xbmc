/*
 * Preset.cpp
 *
 *  Created on: Aug 5, 2008
 *      Author: struktured
 */

#include "Preset.hpp"

Preset::~Preset() {}

Preset::Preset(const std::string & presetName, const std::string & presetAuthor):
	_name(presetName), _author(presetAuthor) {}

void Preset::setName(const std::string & value) { _name = value; }

const std::string & Preset::name() const { return _name; }

void Preset::setAuthor(const std::string & value) { _author = value; }

const std::string & Preset::author() const { return _author; }


