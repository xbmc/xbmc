/*
 * MythXmlCommand.h
 *
 *  Created on: Oct 7, 2010
 *      Author: tafypz
 */

#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_MYTHXMLCOMMAND_H_
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_MYTHXMLCOMMAND_H_


#include "MythXmlCommandResult.h"
#include "MythXmlCommandParameters.h"

#include "../StdString.h"

/*! \class MythXmlCommand
 \brief The base class for all MythXML Commands.
 This class will be subclassed by all MythXML commands; it provides a few basic services:
   - creation of the request url (complete with parameters).
   - execution of the created request.
 */
class MythXmlCommand {
public:
	MythXmlCommand();
	virtual ~MythXmlCommand();
	/*! \brief Execute the command with the given request parameters.
		\param hostname the mythtv backend hostname to connect to.
		\param port the mythtv backend port to connect to.
		\param params the parameters specific to the command.
		\param result the result instance to use to handle the data returned by the request.
		\param timeout the timeout value for this request.
	*/
	void execute(const CStdString& hostname, int port, const MythXmlCommandParameters& params, MythXmlCommandResult& result, int timeout);
protected:
	/*! \brief The MythXML Command to use.
		\return The MythXML Command to use.
	 */
	virtual const CStdString& getCommand() const = 0;
private:
	/*! \brief creates the url to use to issue the request.
		\param hostname the mythtv backend hostname to connect to.
		\param port the mythtv backend port to connect to.
		\param params the parameters specific to the command.
	*/
	CStdString createRequestUrl(const CStdString& hostname, int port, const MythXmlCommandParameters& params);
};

#endif /* XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_MYTHXMLCOMMAND_H_ */
