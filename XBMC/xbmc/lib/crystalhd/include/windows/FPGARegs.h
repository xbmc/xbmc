/********************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: fpgareg.h
 *
 *  Description: Hardware register offsets..
 *
 *  AU
 *
 *  HISTORY:
 *
 *******************************************************************/
#ifndef _INCLUDE_FPGA_REGS_
#define _INCLUDE_FPGA_REGS_


#ifndef _LINK_COMPATIBLE_FPGA_

/*
 * The FPGA that we are going to use is not 
 * compatible with link. Use the OLD Offsets
 */

//
// Interrupt Register Definitions
//
#define INTERRUPT_STATUS_REG				0x200
#define INTERRUPT_SET_REG					0x204
#define INTERRUPT_CLEAR_REG					0x208
#define INTERRUPT_MASK_STS_REG				0x20C
#define INTERRUPT_MASK_SET_REG				0x210
#define INTERRUPT_MASK_CLEAR_REG			0x214


//
// Interrupt Register Definitions
//
#define TX_FIRST_DEC_L_ADDR_L0				0x300
#define TX_FIRST_DESC_U_ADDR_L0				0x304
#define TX_FIRST_DEC_L_ADDR_L1				0x308
#define TX_FIRST_DESC_U_ADDR_L1				0x30C
#define TX_DESC_LIST_CTRL_STS				0x310
#define TX_DMA_ERROR_STS					0x318
#define TX_DMA_CUR_DESC_L_ADDR_L0			0x31C
#define TX_DMA_CUR_DESC_U_ADDR_L0			0x320
#define TX_DMA_CUR_BYTE_CNT_REM_L0			0x324

#define TX_DMA_CUR_DESC_L_ADDR_L1			0x328
#define TX_DMA_CUR_DESC_U_ADDR_L1			0x32C
#define TX_DMA_CUR_BYTE_CNT_REM_L1			0x330

#define RX_Y_DMA_FIRST_DESC_L_ADDR_L0		0x334
#define RX_Y_DMA_FIRST_DESC_U_ADDR_L0		0x338
#define RX_Y_DMA_FIRST_DESC_L_ADDR_L1		0x33C
#define RX_Y_DMA_FIRST_DESC_U_ADDR_L1		0x340
#define RX_Y_DMA_DESC_LIST_CTRL_STS			0x344

#define RX_Y_DMA_ERROR_STS					0x34C
#define RX_Y_DMA_CUR_DESC_L_ADDR_L0			0x350
#define RX_Y_DMA_CUR_DESC_U_ADDR_L0			0x354
#define RX_Y_DMA_CUR_BYTE_CNT_REM_L0		0x358

#define RX_Y_DMA_CUR_DESC_L_ADDR_L1			0x35C
#define RX_Y_DMA_CUR_DESC_U_ADDR_L1			0x360
#define RX_Y_DMA_CUR_BYTE_CNT_REM_L1		0x364

#define RX_UV_DMA_FIRST_DESC_L_ADDR_L0		0x368
#define RX_UV_DMA_FIRST_DESC_U_ADDR_L0		0x36C
#define RX_UV_DMA_FIRST_DESC_L_ADDR_L1		0x370	
#define RX_UV_DMA_FIRST_DESC_U_ADDR_L1		0x374
#define RX_UV_DMA_DESC_LIST_CTRL_STS		0x378

#define RX_UV_DMA_ERROR_STS					0x37C
#define RX_UV_DMA_CUR_DESC_L_ADDR_L0		0x380
#define RX_UV_DMA_CUR_DESC_U_ADDR_L0		0x384
#define RX_UV_DMA_CUR_BYTE_CNT_REM_L0		0x388

#define RX_UV_DMA_CUR_DESC_L_ADDR_L1		0x38C
#define RX_UV_DMA_CUR_DESC_U_ADDR_L1		0x390
#define RX_UV_DMA_CUR_BYTE_CNT_REM_L1		0x394

#define PCI_DMA_DEBUG_OPTIONS_REG			0x398
#define PCI_EPROM_CONTROL					0x400
#define PCI_GLOBAL_CONTROL					0x404
#define PCI_INT_STS_MUX_CONTROL				0x408
#define PCI_INT_STS_REG						0x40C

#else

#ifndef __LINUX_USER__
#include "link\macfile.h"
#include "link\bc_defines.h"
#endif

/*
 * The FPGA that we are going to use is 
 * compatible with link. Use the NEW Offsets.
 */
 
//
// Interrupt Register Definitions
//

#define INTERRUPT_STATUS_REG				0x700
#define INTERRUPT_SET_REG					0x704
#define INTERRUPT_CLEAR_REG					0x708
#define INTERRUPT_MASK_STS_REG				0x70C
#define INTERRUPT_MASK_SET_REG				0x710
#define INTERRUPT_MASK_CLEAR_REG			0x714


//
// Interrupt Register Definitions
//
#define TX_FIRST_DEC_L_ADDR_L0				0xC00
#define TX_FIRST_DESC_U_ADDR_L0				0xC04
#define TX_FIRST_DEC_L_ADDR_L1				0xC08
#define TX_FIRST_DESC_U_ADDR_L1				0xC0C
#define TX_DESC_LIST_CTRL_STS				0xC10
#define TX_DMA_ERROR_STS					0xC18
#define TX_DMA_CUR_DESC_L_ADDR_L0			0xC1C
#define TX_DMA_CUR_DESC_U_ADDR_L0			0xC20
#define TX_DMA_CUR_BYTE_CNT_REM_L0			0xC24

#define TX_DMA_CUR_DESC_L_ADDR_L1			0xC28
#define TX_DMA_CUR_DESC_U_ADDR_L1			0xC2C
#define TX_DMA_CUR_BYTE_CNT_REM_L1			0xC30

#define RX_Y_DMA_FIRST_DESC_L_ADDR_L0		0xC34
#define RX_Y_DMA_FIRST_DESC_U_ADDR_L0		0xC38
#define RX_Y_DMA_FIRST_DESC_L_ADDR_L1		0xC3C
#define RX_Y_DMA_FIRST_DESC_U_ADDR_L1		0xC40
#define RX_Y_DMA_DESC_LIST_CTRL_STS			0xC44

#define RX_Y_DMA_ERROR_STS					0xC4C
#define RX_Y_DMA_CUR_DESC_L_ADDR_L0			0xC50
#define RX_Y_DMA_CUR_DESC_U_ADDR_L0			0xC54
#define RX_Y_DMA_CUR_BYTE_CNT_REM_L0		0xC58

#define RX_Y_DMA_CUR_DESC_L_ADDR_L1			0xC5C
#define RX_Y_DMA_CUR_DESC_U_ADDR_L1			0xC60
#define RX_Y_DMA_CUR_BYTE_CNT_REM_L1		0xC64

#define RX_UV_DMA_FIRST_DESC_L_ADDR_L0		0xC68
#define RX_UV_DMA_FIRST_DESC_U_ADDR_L0		0xC6C
#define RX_UV_DMA_FIRST_DESC_L_ADDR_L1		0xC70	
#define RX_UV_DMA_FIRST_DESC_U_ADDR_L1		0xC74
#define RX_UV_DMA_DESC_LIST_CTRL_STS		0xC78

#define RX_UV_DMA_ERROR_STS					0xC7C
#define RX_UV_DMA_CUR_DESC_L_ADDR_L0		0xC80
#define RX_UV_DMA_CUR_DESC_U_ADDR_L0		0xC84
#define RX_UV_DMA_CUR_BYTE_CNT_REM_L0		0xC88

#define RX_UV_DMA_CUR_DESC_L_ADDR_L1		0xC8C
#define RX_UV_DMA_CUR_DESC_U_ADDR_L1		0xC90
#define RX_UV_DMA_CUR_BYTE_CNT_REM_L1		0xC94

#define PCI_DMA_DEBUG_OPTIONS_REG			0xC98
#define PCI_EPROM_CONTROL					0x800
#define PCI_GLOBAL_CONTROL					0xD00
#define PCI_INT_STS_REG						0xD04
#define PCI_INT_STS_MUX_CONTROL				0xD08
#define PCI_CONFIG_FPGA_GLOBAL_RESET		0x50 // Offset 0x50 in PCI Config Space

/* EPROM BSC Device Regsiter Map */
#define EPROM_CONTROL_BASE					0x800
#define EPROM_CHIP_ADDR_REG					0x800
#define EPROM_WRITE_DATA0_REG				0x804
#define EPROM_WRITE_DATA1_REG				0x808
#define EPROM_WRITE_DATA2_REG				0x80C
#define EPROM_WRITE_DATA3_REG				0x810
#define EPROM_WRITE_DATA4_REG				0x814
#define EPROM_WRITE_DATA5_REG				0x818
#define EPROM_WRITE_DATA6_REG				0x81C
#define EPROM_WRITE_DATA7_REG				0x820
#define EPROM_BYTE_CNTR_REG					0x824
#define EPROM_CONTROL_REG					0x828
#define EPROM_RW_ENABLE_INTERRUPT_REG		0x82C
#define EPROM_READ_DATA0_REG				0x830
#define EPROM_READ_DATA1_REG				0x834
#define EPROM_READ_DATA2_REG				0x838
#define EPROM_READ_DATA3_REG				0x83C
#define EPROM_READ_DATA4_REG				0x840
#define EPROM_READ_DATA5_REG				0x844
#define EPROM_READ_DATA6_REG				0x848
#define EPROM_READ_DATA7_REG				0x84C
#define EPROM_CONTROL_HIGH_REG				0x850

#define EPROM_SLAVE_ADDRESS_VAL				0xA0
#define EPROM_TOTAL_SIZE					0x4000
#define EPROM_TRANSFER_MAX_SIZE				0x04
//#define EPROM_WR_PAGE_SIZE					0x40
/*Modified the page to 32 bytes inorder to support all sizes of 
 eproms 32K, 64K and 128K. 64k and 128K have a page size of 
 64 bytes.*/
#define EPROM_WR_PAGE_SIZE					0x20

/* Byte Counter Register fields */
#define EPROM_BC_CNT_REG1_FIELD				0x0F

/* RW Enable and Interrupt Register fields */
#define EPROM_NOACK_PRESENT					0x04
#define EPROM_NO_STOP_SEQ					0x10
#define EPROM_NO_START_SEQ					0x20
#define EPROM_INTR_PRESENT					0x02
#define EPROM_OP_ENABLE						0x01
#define EPROM_RESET_ENABLE_REG				0x00

/* Control Register fields */
#define EPROM_CNTRL_REG_INIT_VAL			0x50
#define EPROM_CNTRL_REG_WO_FORMAT			0x00
#define EPROM_CNTRL_REG_RO_FORMAT			0x01

/* FPGA Copy Engine */
#define BC_FW_COPY_ENG_CNTRL_REG_OFFSET		0x380000
#define BC_FPGA_COPY_DRAM_OFFSET_REG_OFFSET	0xD08
#define BC_FPGA_COPY_INT_BIT				7
#define BC_FPGA_COPY_EN						0x80000000
#define BC_FPGA_COPY_REG_OFFSET				0xD10
#define BC_FWIMG_ST_ADDR					0x00000000
#define MAX_BIN_FILE_SZ						0x300000

/* Additional Link Specific Registers And Offset
 * Which are there only in Link Hardware, most of the registers are defined in the macfile.h
 */
#define MISC_PERST_CLOCK_CTRL				0xE9C			/* Clock Control Register */
#define PCIE_PWR_MGMT						0x500
#define PCIE_CLK_REQ_REG					0xDC
#endif

#define LOW_ADDR_DESC_VALID_BIT			0x01
#define DMA_START_BIT					0x01
#define ASPM_L1_ENABLE					(BC_BIT(27))
#define	PCI_CLK_REQ_ENABLE				(BC_BIT(8))

//
// Interrupt Status Register definitions
//
typedef union _INTR_STS_REG_{
	
	struct {
		ULONG	L0TxDMADone:1;		
		ULONG	L0TxDMAErr:1;		
		ULONG	L0YRxDMADone:1;		
		ULONG	L0YRxDMAErr:1;		
		ULONG	L0UVRxDMADone:1;		
		ULONG	L0UVRxDMAErr:1;		
		ULONG	PCI_Err:1;		
		ULONG	Reserved1:1;
		ULONG	L1TxDMADone:1;		
		ULONG	L1TxDMAErr:1;		
		ULONG	L1YRxDMADone:1;		
		ULONG	L1YRxDMAErr:1;		
		ULONG	L1UVRxDMADone:1;		
		ULONG	L1UVRxDMAErr:1;	
		ULONG   Reserved2:18;
	};

	ULONG	WholeReg;
}INTR_STS_REG;

typedef union _INTR_MASK_REG_{
	
	struct {
		ULONG	MaskTxDMADone:1;		
		ULONG	MaskTxDMAErr:1;		
		ULONG	MaskRxDMADone:1;		
		ULONG	MaskRxDMAErr:1;		
		ULONG	MaskPCIE_Error:1;		
		ULONG	MaskPCIE_RBusMastErr:1;		
		ULONG	MaskPCIE_RGRBridge:1;		
		ULONG	Reserved:25;
	};

	ULONG	WholeReg;
}INTR_MASK_REG;

typedef union _DESC_LOW_ADDR_REG_{
	
	struct {
		ULONG	ListValid:1;
		ULONG	Reserved:4;
		ULONG   LowAddr:27;
	};

	ULONG	WholeReg;
}DESC_LOW_ADDR_REG;


typedef union _RX_DMA_ERR__STS_REG_{
	
	struct {
		ULONG	L0DataParity:1;		//B0
		ULONG	L0FifoFull:1;		//B1
		ULONG   L0DataAbort:1;		//B2
		ULONG	L1DataParity:1;		//B3
		ULONG	L1FifoFull:1;		//B4
		ULONG	L1DataAbort:1;		//B5
		ULONG	L0DescParity:1;		//B6
		ULONG	L0DescAbort:1;		//B7
		ULONG	L1DescParity:1;		//B8
		ULONG	L1DescAbort:1;		//B9
		ULONG	L0OverRun:1;		//B10
		ULONG	L0UnderRun:1;		//B11
		ULONG	L1OverRun:1;		//B12
		ULONG	L1UnderRun:1;		//B13
		ULONG   Reserved:18;		//B14-31
	};

	ULONG	WholeReg;
}DMA_ERR_STS_REG;


/* Link Hardware specific definitions */

typedef union _LINK_MISC_PERST_DECODER_CTRL_
{

	struct{
		ULONG	BCM7412Rst:1;				/* 1 -> BCM7412 is held in reset. Reset value 1.*/
		ULONG	Reserved0:3;				/* Reserved.No Effect*/
		ULONG	STOPBCM7412Clk:1;			/* 1 ->Stops branch of 27MHz clk used to clk BCM7412*/
		ULONG	Reserved1:27;				/* Reseved. No Effect*/			
	};

	ULONG	WholeReg;

} LINK_MISC_PERST_DECODER_CTRL;

typedef union _LINK_MISC_PERST_CLOCK_CTRL_
{

	struct{
		ULONG	SEL_ALT_CLK:1;				/* When set, selects a 6.75MHz clock as the source of core_clk */
		ULONG	STOP_CORE_CLK:1;			/* When set, stops the branch of core_clk that is not needed for low power operation */
		ULONG	PLL_PWRDOWN:1;				/* When set, powers down the main PLL. The alternate clock bit should be set
												to select an alternate clock before setting this bit.*/
		ULONG	Reserved0:5;				/* Reserved */
		ULONG	PLL_MULT:8;					/* This setting controls the multiplier for the PLL. */
		ULONG	PLL_DIV:4;					/* This setting controls the divider for the PLL. */
		ULONG	Reserved1:12;				/* Reserved */
	};

	ULONG	WholeReg;

} LINK_MISC_PERST_CLOCK_CTRL;

#endif
