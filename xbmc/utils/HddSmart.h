#pragma once
#include "thread.h"

static CRITICAL_SECTION m_CriticalSection;
class CHDDSmart : public CThread //CThread
{
  public:
    CHDDSmart();
    ~CHDDSmart();
    
    virtual void OnStartup();
    virtual void OnExit();
    virtual void Process();

    BYTE m_HddSmarValue;
    bool IsRunning();
    bool Start();
    void Stop();
    BYTE GetSmartValue(int SmartREQ);
    bool DelayRequestSmartValue(int SmartREQ , int iDelayTime);

protected:
    
    typedef struct IP_IDE_REG
    {
	    BYTE bFeaturesReg;			  // Used for specifying SMART "commands".
	    BYTE bSectorCountReg;		  // IDE sector count register
	    BYTE bSectorNumberReg;		// IDE sector number register
	    BYTE bCylLowReg;			    // IDE low order cylinder value
	    BYTE bCylHighReg;			    // IDE high order cylinder value
	    BYTE bDriveHeadReg;			  // IDE drive/head register
	    BYTE bCommandReg;			    // Actual IDE command.
	    BYTE bReserved;				    // reserved for future use.  Must be zero.
    };
    typedef IP_IDE_REG* LPIP_IDE_REG;
    typedef struct OP_IDE_REG
    {
	    BYTE bErrorReg;
	    BYTE bSectorCountReg;
	    BYTE bSectorNumberReg;
	    BYTE bCylLowReg;
	    BYTE bCylHighReg;
	    BYTE bDriveHeadReg;
	    BYTE bStatusReg;
    };
    typedef OP_IDE_REG* LPOP_IDE_REG;
    typedef struct ATA_COMMAND_OBJ
    {
	    IP_IDE_REG	IPReg;
	    OP_IDE_REG	OPReg;
	    BYTE		DATA_BUFFER[512];
	    ULONG		DATA_BUFFSIZE;
    };
    typedef ATA_COMMAND_OBJ* LPATA_COMMAND_OBJ;
    typedef	struct	_DRIVEATTRIBUTE 
    {	BYTE	bAttrID;				    // Identifies which attribute
      WORD	wStatusFlags;			  // see bit definitions below
      BYTE	bAttrValue;				  // Current normalized value
      BYTE	bWorstValue;			  // How bad has it ever been?
      BYTE	bRawValue[5];			  // Un-normalized value
      BYTE	bReserved;				  // ...
    } DRIVEATTRIBUTE, *PDRIVEATTRIBUTE, *LPDRIVEATTRIBUTE;
    typedef	struct	_ATTRTHRESHOLD 
    {	BYTE	bAttrID;				    // Identifies which attribute
      BYTE	bWarrantyThreshold;	// Triggering value
      BYTE	bReserved[10];			// ...
    } ATTRTHRESHOLD, *PATTRTHRESHOLD, *LPATTRTHRESHOLD;	

    BOOL SendATACommand(WORD IDEPort, LPATA_COMMAND_OBJ ATACommandObj, UCHAR ReadWrite);
 
 };

extern CHDDSmart* m_hddsmart;