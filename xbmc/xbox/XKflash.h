
#ifndef XBOXFLASH_H
#define XBOXFLASH_H

#include <stdio.h>
#include <stdarg.h>

#define	FLASH_STATUS_DQ7			0x80
#define FLASH_STATUS_TOGGLE			0x40
#define FLASH_STATUS_ERROR			0x20
#define FLASH_STATUS_ERASE_TIMER	0x08
#define FLASH_STATUS_TOGGLE_ALT		0x04

#define FLASH_BASE_ADDRESS			0xff000000
#define KERNEL_BASE_ADDRESS			0xF0012CBC

#define _2BL_MAGICNUMBER			0x095fe4
#define _2BL_DECRYPTION_KEY			0xffffffa5

#define	FLASH_UNLOCK_ADDR_1			(FLASH_BASE_ADDRESS+0x5555)
#define	FLASH_UNLOCK_ADDR_2			(FLASH_BASE_ADDRESS+0x2aaa)
#define	FLASH_UNLOCK_ADDR_3			(FLASH_BASE_ADDRESS+0x5555)
#define	FLASH_UNLOCK_ADDR_4			(FLASH_BASE_ADDRESS+0x5555)

#define	FLASH_UNLOCK2_ADDR_1		(FLASH_BASE_ADDRESS+0x5555)
#define	FLASH_UNLOCK2_ADDR_2		(FLASH_BASE_ADDRESS+0x5555)
#define	FLASH_UNLOCK2_ADDR_3		(FLASH_BASE_ADDRESS+0x2aaa)
#define	FLASH_UNLOCK2_ADDR_4		(FLASH_BASE_ADDRESS+0x5555)


#define FLASH_UNLOCK_DATA_1			0xaa
#define FLASH_UNLOCK_DATA_2			0x55
#define FLASH_UNLOCK_DATA_3	        0x90
#define FLASH_UNLOCK_DATA_4	        0xff

#define FLASH_UNLOCK2_DATA_1			0xf0
#define FLASH_UNLOCK2_DATA_2			0xaa
#define FLASH_UNLOCK2_DATA_3	        0x55
#define FLASH_UNLOCK2_DATA_4	        0xf0


#define FLASH_COMMAND_RESET			0xf0
#define FLASH_COMMAND_AUTOSELECT	0x90
#define FLASH_COMMAND_PROGRAM		0xa0
#define FLASH_COMMAND_UNLOCK_BYPASS	0x20
#define FLASH_COMMAND_ERASE			0x80

#define FLASH_COMMAND_ERASE_BLOCK	0x30
#define FLASH_COMMAND_ERASE_ALL		0x10

#define FLASH_BLOCK_COUNT			0x10
#define FLASH_BLOCK_MASK			(FLASH_BLOCK_COUNT-1)

typedef struct fci_s {
	unsigned char mfct;
	unsigned char devc;
	char text[40];
	unsigned long size;
	struct fci_s *next;
} fci_t;

class CXBoxFlash
{
	fci_t *flashinfo;
	void (*UpdateStatus)(WCHAR *line1, WCHAR *line2, unsigned long addr);

public:
	CXBoxFlash()
	{
		flashinfo=NULL;
	}
	
	~CXBoxFlash(void)
	{
	}

	fci_t* AddFCI(BYTE manuf, BYTE code, char *text, unsigned long size)
	{
		fci_t *cur = flashinfo;

		if(cur==NULL) {
			flashinfo = (fci_t*)malloc(sizeof(fci_t));
			cur=flashinfo;
		} else {
			while(cur->next!=NULL) cur=cur->next;
			cur->next = (fci_t*)malloc(sizeof(fci_t));
			cur=cur->next;
		}
		memset(cur, 0, sizeof(fci_t));

		cur->mfct = manuf;
		cur->devc = code;
		strcpy(cur->text,text);
		cur->size = size;

		return cur;
	}

	fci_t* FindFCI(BYTE manuf, BYTE code)
	{
		fci_t *cur = flashinfo;

		while(cur!=NULL) {
			if((cur->mfct==manuf) && (cur->devc==code)) return cur;
			cur=cur->next;
		}
		return NULL;
	}

	inline void Write(DWORD address,BYTE data)
	{
		volatile BYTE *ptr=(BYTE*)address;
		*ptr=data;
	}

	inline BYTE Read(DWORD address)
	{
		volatile BYTE *ptr=(BYTE*)address;
		return *ptr;
	}

void SetReadMode2(void)
{
	// 	Unlock stage 1
	Write(FLASH_UNLOCK_ADDR_1,FLASH_UNLOCK_DATA_1); 
	Write(FLASH_UNLOCK_ADDR_2,FLASH_UNLOCK_DATA_2);
	Write(FLASH_UNLOCK_ADDR_3,FLASH_UNLOCK_DATA_3);
	// Issue the reset
	Write(FLASH_UNLOCK_ADDR_1,FLASH_COMMAND_RESET);

  BYTE dummy=Read(FLASH_BASE_ADDRESS); 
}   
fci_t* CheckID2(void)
{
  BYTE manuf,code;

  Write(FLASH_UNLOCK_ADDR_1,FLASH_UNLOCK_DATA_1);
  Write(FLASH_UNLOCK_ADDR_2,FLASH_UNLOCK_DATA_2);
  Write(FLASH_UNLOCK_ADDR_3,FLASH_UNLOCK_DATA_3);
  Write(FLASH_UNLOCK_ADDR_3,FLASH_UNLOCK_DATA_3);
  Write(FLASH_UNLOCK_ADDR_4,FLASH_UNLOCK_DATA_4); 
  Write(FLASH_UNLOCK_ADDR_1,FLASH_COMMAND_AUTOSELECT);
  manuf=Read(FLASH_BASE_ADDRESS);
  code=Read(FLASH_BASE_ADDRESS+1);

  SetReadMode();
  return FindFCI(manuf,code);
}
fci_t* CheckID(void) 
{
	BYTE manuf,code;
	// 	Unlock stage 1
	Write(FLASH_UNLOCK_ADDR_1,FLASH_UNLOCK_DATA_1);
	Write(FLASH_UNLOCK_ADDR_2,FLASH_UNLOCK_DATA_2);
	// Issue the autroselect
	Write(FLASH_UNLOCK_ADDR_1,FLASH_COMMAND_AUTOSELECT);

	manuf=Read(FLASH_BASE_ADDRESS);
	code=Read(FLASH_BASE_ADDRESS+1);
  
  // All done
	SetReadMode();

	return FindFCI(manuf,code);
}

void SetReadMode(void)
	{
		// 	Unlock stage 1
		Write(FLASH_UNLOCK_ADDR_1,FLASH_UNLOCK_DATA_1); 
		Write(FLASH_UNLOCK_ADDR_2,FLASH_UNLOCK_DATA_2);
		// Issue the reset
		Write(FLASH_UNLOCK_ADDR_1,FLASH_COMMAND_RESET);

		// Leave it in a read mode to avoid any buss contention issues
		BYTE dummy=Read(FLASH_BASE_ADDRESS);
	}

bool EraseBlock(int block)//int block)
	{
		bool retval;

		// 	Unlock stage 1
		Write(FLASH_UNLOCK_ADDR_1,FLASH_UNLOCK_DATA_1);
		Write(FLASH_UNLOCK_ADDR_2,FLASH_UNLOCK_DATA_2);
		// Issue the erase
		Write(FLASH_UNLOCK_ADDR_1,FLASH_COMMAND_ERASE);
		// Unlock stage 2
		Write(FLASH_UNLOCK_ADDR_1,FLASH_UNLOCK_DATA_1);
		Write(FLASH_UNLOCK_ADDR_2,FLASH_UNLOCK_DATA_2);
		// Now set the block
		Write((FLASH_UNLOCK_ADDR_1+(block&FLASH_BLOCK_MASK)),FLASH_COMMAND_ERASE_BLOCK);
		
		// Now poll for a result
		retval=WaitOnToggle();
		
		// All done		
		SetReadMode();

		return retval;
	}
bool EraseDevice(fci_t* fci)
	{
		
		bool retval;
		
		// 	Unlock stage 1
		Write(FLASH_UNLOCK_ADDR_1,FLASH_UNLOCK_DATA_1);
		Write(FLASH_UNLOCK_ADDR_2,FLASH_UNLOCK_DATA_2);
		// Issue the erase
		Write(FLASH_UNLOCK_ADDR_1,FLASH_COMMAND_ERASE);
		// Unlock stage 2
		Write(FLASH_UNLOCK_ADDR_1,FLASH_UNLOCK_DATA_1);
		Write(FLASH_UNLOCK_ADDR_2,FLASH_UNLOCK_DATA_2);
		// Now set the block
		Write(FLASH_UNLOCK_ADDR_1,FLASH_COMMAND_ERASE_ALL);
		
		// Now poll for a result
		retval=WaitOnToggle();

		// All done
		SetReadMode();

		// Check it all 0xFF
		for(DWORD address=FLASH_BASE_ADDRESS;address<(FLASH_BASE_ADDRESS+fci->size);address++)
		{
			if(Read(address)!=0xff)
			{
//				WCHAR tmpW[128];
//				wsprintfW(tmpW, L"Unable to erase 0x%08x", address);
//				UpdateStatus(L"Erase Failure:", tmpW, address);
				return false;
			}
		}

//		return retval;
	}

	bool ProgramDevice(char *filename,fci_t* fci)
	{
		FILE *fp;
		BYTE data;
		volatile BYTE dummy;
		DWORD twiddle=0;
		DWORD address=FLASH_BASE_ADDRESS;
        bool retval;
		long BinFileSize;

		fp=fopen(filename,"rb");
		if (fp==NULL) return false;
        fseek(fp,0L,SEEK_END);
		BinFileSize=ftell(fp);
		fseek(fp,0L,SEEK_SET); 

//		if (BinFileSize!=fci->size) return false;

		while (fread(&data,1,sizeof(BYTE),fp)==1)
		{
//			// Check address bound
//			if(address>=FLASH_BASE_ADDRESS+0x100000)
//			{
//				UpdateStatus(L"Programming Failure:", L"Incorrect Filesize", address);
//				fclose(fp);
//				SetReadMode();
//				return 0;
//			}

			// 	Unlock stage 1
			Write(FLASH_UNLOCK_ADDR_1,FLASH_UNLOCK_DATA_1);
			Write(FLASH_UNLOCK_ADDR_2,FLASH_UNLOCK_DATA_2);
			// Issue the program command
			Write(FLASH_UNLOCK_ADDR_1,FLASH_COMMAND_PROGRAM);
			// Program Byte
			Write(address,data);
            retval=WaitOnToggle();
			// Do the Data polling test
			while(1)
			{
				dummy=Read(address);

				if((data&FLASH_STATUS_DQ7)==(dummy&FLASH_STATUS_DQ7)) break;

				dummy=Read(address);
				if((data&FLASH_STATUS_DQ7)==(dummy&FLASH_STATUS_DQ7))
				{
					break;
				}
				else 
				{
//					wsprintfW(tmpW, L"Write failure at 0x%08x", address);
//					UpdateStatus(L"Programming Failure:", tmpW, address);
					fclose(fp);
					SetReadMode();
					return false;
				}
			}
			Write(FLASH_UNLOCK_ADDR_1,FLASH_COMMAND_RESET);

			dummy=Read(address);
			// Verify the written byte
			if(dummy!=data)
			{
//				wsprintfW(tmpW, L"Verify failure at 0x%08x, Wrote 0x%02x, Read 0x%02x", address, data, dummy);
//				UpdateStatus(L"Programming Failure:", tmpW, address);
				fclose(fp);
				SetReadMode();
				return false;
			}

			// Next byte
			address++;

			// User information
//			if((address&0xffff)==0x0000)
//			{
//				wsprintfW(tmpW, L"Wrote block %02d",(address>>16)&0xff);
//				UpdateStatus(L"Writing...", tmpW, address);
//			}
		}

//		wsprintfW(tmpW, L"Wrote %i bytes", address && 0xffffff);
//		UpdateStatus(L"Verifying...", tmpW, address);

		// Verify written code
		int count=0;
		fseek(fp,0,SEEK_SET);
		address=FLASH_BASE_ADDRESS;
		while (fread(&data,1,sizeof(BYTE),fp)==1)
		{
			if (data!=Read(address))
			{
//				wsprintfW(tmpW, L"Verify failure at 0x%08x, Wrote 0x%02x, Read 0x%02x", address, data, Read(address));
//				UpdateStatus(L"Programming Failure:", tmpW, address);
				if(count++>8) return 0;
			}
			address++;
		}

		fclose(fp);

		if(count) {
//			UpdateStatus(L"Programming Failure:", L"One or more bytes did not write propperly.", FLASH_BASE_ADDRESS);
			return false;
		} else {
//			UpdateStatus(L"Programming Complete:", L"Flash Programming Successful.", address-1);
			return true;
		}
	}

	bool WaitOnToggle(void)
	{
		BYTE last,current;
		last=Read(FLASH_BASE_ADDRESS);
		while(1)
		{
			// We wait for the toggle bit to stop toggling
			current=Read(FLASH_BASE_ADDRESS);
			// Check for an end to toggling
			if((last&FLASH_STATUS_TOGGLE)==(current&FLASH_STATUS_TOGGLE)) break;
			last=current;

			// We're still in command mode so its OK to check for Error condition
			if(current&FLASH_STATUS_ERROR)
			{
				last=Read(FLASH_BASE_ADDRESS);
				current=Read(FLASH_BASE_ADDRESS);
				if((last&FLASH_STATUS_TOGGLE)==(current&FLASH_STATUS_TOGGLE)) break; else return false;
			}
		}
		return true;
	}

};

#endif