#ifndef _CSTRNODE_
#define _CSTRNODE_


#include "nodeClass.h"
#include "UtilStr.h"


class CStrNode : public nodeClass, public UtilStr {
					
	public:
								CStrNode( nodeClass* inParent, char* inStr );
								CStrNode( nodeClass* inParent );
								
		enum 					{ sClassID = 'StrN' 									};


		virtual void			ReadFrom( CEgIStream* inStream );
		virtual void			WriteTo( CEgOStream* inStream );

		
};





#endif







