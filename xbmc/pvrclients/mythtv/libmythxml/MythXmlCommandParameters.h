/*
 * MythXmlCommandParameters.h
 *
 *  Created on: Oct 8, 2010
 *      Author: tafypz
 */

#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_MYTHXMLCOMMANDPARAMETERS_H_
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_MYTHXMLCOMMANDPARAMETERS_H_

#include "DateTime.h"

#include "../StdString.h"

/*!	\class MythXmlCommandParameters
	\brief This is the base abstract class for all MythXML command parameters.
	This class should be subclassed in order to provide parameters to the mythxml commands.
	It has the responsibility to create a list of HTTP parameters to use for the request.
	A subclass called \ref MythXmlEmptyCommandParameters MythXmlEmptyCommandParameters has been provided for convenience
	in case the mythXML command doesn't need parameters.
 */
class MythXmlCommandParameters {
public:
	static CStdString convertTimeToString(const CDateTime& time);

	MythXmlCommandParameters(bool hasParameters);
	virtual ~MythXmlCommandParameters();
	virtual CStdString createParameterString() const = 0;
	inline bool hasParameters() const {return hasParameters_;};
private:
	bool hasParameters_;
};

/*!	\class MythXmlEmptyCommandParameters
	\brief This class is to be used with commands that do not take parameters.
 */
class MythXmlEmptyCommandParameters : public MythXmlCommandParameters {
	MythXmlEmptyCommandParameters();
	virtual ~MythXmlEmptyCommandParameters();
	virtual CStdString createParameterString() const;
};

#endif /* MYTHXMLCOMMANDPARAMETERS_H_ */
