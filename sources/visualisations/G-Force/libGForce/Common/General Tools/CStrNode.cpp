
#include "CStrNode.h"


CStrNode::CStrNode( nodeClass* inParent, char* inStr ) :
	nodeClass( inParent ),
	UtilStr( inStr ) {
	
	mType = CStrNodeT;
}



CStrNode::CStrNode( nodeClass* inParent ) :
	nodeClass( inParent ) {
	
	mType = CStrNodeT;

}






void CStrNode::ReadFrom( CEgIStream* inStream ) {
	
	
	nodeClass::ReadFrom( inStream );
	UtilStr::ReadFrom( inStream );
}





void CStrNode::WriteTo( CEgOStream* inStream ) {
	
	nodeClass::WriteTo( inStream );
	UtilStr::WriteTo( inStream );
}
	










