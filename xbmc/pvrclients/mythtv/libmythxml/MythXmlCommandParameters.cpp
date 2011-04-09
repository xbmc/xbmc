/*
 * MythXmlCommandParameters.cpp
 *
 *  Created on: Oct 8, 2010
 *      Author: tafypz
 */

#include "MythXmlCommandParameters.h"
#include "XBDateTime.h"

MythXmlCommandParameters::MythXmlCommandParameters(bool hasParameters) {
	hasParameters_ = hasParameters;
}

MythXmlCommandParameters::~MythXmlCommandParameters() {
}

CStdString MythXmlCommandParameters::convertTimeToString(const CDateTime& time){
	CStdString result = "%i-%02.2i-%02.2iT%02.2i:%02.2i";
	result.Format(result.c_str(), time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute());
	return result;
}

MythXmlEmptyCommandParameters::MythXmlEmptyCommandParameters() : MythXmlCommandParameters(false){
}

MythXmlEmptyCommandParameters::~MythXmlEmptyCommandParameters(){
}

CStdString MythXmlEmptyCommandParameters::createParameterString() const{
	static CStdString result = "";
	return result;
};
