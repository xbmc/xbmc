#ifndef _XMLCHOICE_H_
#define _XMLCHOICE_H_

#define USE_RAPIDXML

#ifndef USE_RAPIDXML
	#include "XBMCTinyXML.h"
	#define XML_ELEMENT TiXmlElement
	#define XML_ATTRIBUTE TiXmlAttribute
#else
	#include "rapidxml.hpp"
	#define XML_ELEMENT xml_node<>
	#define XML_ATTRIBUTE xml_attribute<>
	
#endif

#endif /* _XMLCHOICE_H_*/