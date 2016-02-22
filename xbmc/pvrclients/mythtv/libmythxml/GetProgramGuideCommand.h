#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETPROGRAMGUIDECOMMAND_H
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETPROGRAMGUIDECOMMAND_H
// 
#include "MythXmlCommand.h"

class GetProgramGuideCommand: public MythXmlCommand {
public:
	GetProgramGuideCommand() {};
	virtual ~GetProgramGuideCommand() {};
protected:
	virtual const CStdString& getCommand() const {
		static CStdString result = "GetProgramGuide";
		return result;
	};
};


#endif // XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETPROGRAMGUIDECOMMAND_H
