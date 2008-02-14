/*****************************************************************
|
|   Neptune - Serial Ports
|
|   (c) 2001-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_SERIAL_PORT_H_
#define _NPT_SERIAL_PORT_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptStreams.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const int NPT_ERROR_NO_SUCH_SERIAL_PORT      = NPT_ERROR_BASE_SERIAL_PORT - 0;
const int NPT_ERROR_SERIAL_PORT_NOT_OPEN     = NPT_ERROR_BASE_SERIAL_PORT - 1;
const int NPT_ERROR_SERIAL_PORT_ALREADY_OPEN = NPT_ERROR_BASE_SERIAL_PORT - 2;
const int NPT_ERROR_SERIAL_PORT_BUSY         = NPT_ERROR_BASE_SERIAL_PORT - 3;

typedef enum {
    NPT_SERIAL_PORT_PARITY_NONE,
    NPT_SERIAL_PORT_PARITY_EVEN,
    NPT_SERIAL_PORT_PARITY_ODD,
    NPT_SERIAL_PORT_PARITY_MARK
} NPT_SerialPortParity;

typedef enum {
    NPT_SERIAL_PORT_STOP_BITS_1,
    NPT_SERIAL_PORT_STOP_BITS_1_5,
    NPT_SERIAL_PORT_STOP_BITS_2
} NPT_SerialPortStopBits;

typedef enum {
    NPT_SERIAL_PORT_FLOW_CONTROL_NONE,
    NPT_SERIAL_PORT_FLOW_CONTROL_HARDWARE,
    NPT_SERIAL_PORT_FLOW_CONTROL_XON_XOFF
} NPT_SerialPortFlowControl;

/*----------------------------------------------------------------------
|   NPT_SerialPortInterface
+---------------------------------------------------------------------*/
class NPT_SerialPortInterface
{
public:
    // constructors and destructor
    virtual ~NPT_SerialPortInterface() {}

    // methods
    virtual NPT_Result Open(unsigned int              speed, 
                            NPT_SerialPortStopBits    stop_bits,
                            NPT_SerialPortFlowControl flow_control,
                            NPT_SerialPortParity      parity) = 0;
    virtual NPT_Result Close() = 0;
    virtual NPT_Result GetInputStream(NPT_InputStreamReference& stream) = 0;
    virtual NPT_Result GetOutputStream(NPT_OutputStreamReference& stream) = 0;
};

/*----------------------------------------------------------------------
|   NPT_SerialPort
+---------------------------------------------------------------------*/
class NPT_SerialPort : public NPT_SerialPortInterface
{
public:
    // constructors and destructor
    NPT_SerialPort(const char* name);
   ~NPT_SerialPort() { delete m_Delegate; }

    // NPT_SerialPortInterface methods
    NPT_Result Open(unsigned int              speed, 
                    NPT_SerialPortStopBits    stop_bits = NPT_SERIAL_PORT_STOP_BITS_1,
                    NPT_SerialPortFlowControl flow_control = NPT_SERIAL_PORT_FLOW_CONTROL_NONE,
                    NPT_SerialPortParity      parity = NPT_SERIAL_PORT_PARITY_NONE) {
        return m_Delegate->Open(speed, stop_bits, flow_control, parity);
    }
    NPT_Result Close() {
        return m_Delegate->Close();
    }
    NPT_Result GetInputStream(NPT_InputStreamReference& stream) {
        return m_Delegate->GetInputStream(stream);
    }
    NPT_Result GetOutputStream(NPT_OutputStreamReference& stream) {
        return m_Delegate->GetOutputStream(stream);
    }

protected:
    // members
    NPT_SerialPortInterface* m_Delegate;
};

#endif // _NPT_SERIAL_PORT_H_ 
