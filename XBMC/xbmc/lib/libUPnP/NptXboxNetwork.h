/*****************************************************************
|
|   Neptune - Network :: Winsock Implementation
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   static initializer
+---------------------------------------------------------------------*/
class NPT_WinsockSystem {
public:
    static NPT_WinsockSystem Initializer;
private:
    NPT_WinsockSystem();
    ~NPT_WinsockSystem();
};
