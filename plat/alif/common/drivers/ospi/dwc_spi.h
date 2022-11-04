/* Copyright (C) 2022 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */
typedef struct {
  volatile uint32_t DW_SPI_CTRLR0;	/* 0x0 */
  volatile uint32_t DW_SPI_CTRLR1;	/* 0x4 */
  volatile uint32_t DW_SPI_SSIENR;	/* 0x8 */
  volatile uint32_t DW_SPI_MWCR;	/* 0xC */

  volatile uint32_t DW_SPI_SER;		/* 0x10 */
  volatile uint32_t DW_SPI_BAUDR;	/* 0x14 */
  volatile uint32_t DW_SPI_TXFTLR;	/* 0x18 */
  volatile uint32_t DW_SPI_RXFTLR;	/* 0x1C */

  volatile uint32_t DW_SPI_TXFLR;	/* 0x20 */
  volatile uint32_t DW_SPI_RXFLR;	/* 0x24 */
  volatile uint32_t DW_SPI_SR;		/* 0x28 */
  volatile uint32_t DW_SPI_IMR;		/* 0x2C */

  volatile uint32_t DW_SPI_ISR;		/* 0x30 */
  volatile uint32_t DW_SPI_RISR;	/* 0x34 */
  volatile uint32_t DW_SPI_TXEICR;	/* 0x38 */
  volatile uint32_t DW_SPI_RXOICR;	/* 0x3C */

  volatile uint32_t DW_SPI_RXUICR;	/* 0x40 */
  volatile uint32_t DW_SPI_MSTICR;	/* 0x44 */
  volatile uint32_t DW_SPI_ICR;		/* 0x48 */
  volatile uint32_t DW_SPI_DMACR;	/* 0x4C */

  volatile uint32_t DW_SPI_DMATDLR;	/* 0x50 */
  volatile uint32_t DW_SPI_DMARDLR;	/* 0x54 */
  volatile uint32_t DW_SPI_IDR;		/* 0x58 */
  volatile uint32_t DW_SPI_VERSION;	/* 0x5C */

  volatile uint32_t DW_SPI_DR;		/* 0x60 */
  volatile uint32_t dummy[35];		/* 0x64 .. 0xEC */

  volatile uint32_t DW_SPI_RX_SAMPLE_DLY;	/* 0xF0 */
  volatile uint32_t DW_SPI_CS_CTRLR0;		/* 0xF4 */
  volatile uint32_t DW_SPI_DDR_DRIVE_EDGE;	/* 0xF8 */
  volatile uint32_t DW_SPI_XIP_MODE_BITS;	/* 0xFC */

  volatile uint32_t DW_XIP_INCR_INST;		/* 0x100 */
  volatile uint32_t DW_XIP_WRAP_INST;		/* 0x104 */
  volatile uint32_t DW_XIP_CTRL;		/* 0x108 */
  volatile uint32_t DW_XIP_SER;			/* 0x10C */
  volatile uint32_t DW_XRXOICR;			/* 0x110 */
  volatile uint32_t DW_XIP_CNT_TIME_OUT;	/* 0x114 */

} ssi_regs_t;

typedef struct {
	volatile uint32_t CSPI_CTRL_REG;	/* 0x0 Control register */
	volatile uint32_t CSPI_INTR_REG;	/* 0x4 Interrupts */
	volatile uint32_t CSPI_INTR_MASK_REG;	/* 0x8 Interrupt mask, default to masked */
	volatile uint32_t CSPI_AES_KEY0_REG;	/* 0xc  AES decryption key [31:0]*/
	volatile uint32_t CSPI_AES_KEY1_REG;	/* 0x10 AES decryption key [63:32]*/
	volatile uint32_t CSPI_AES_KEY2_REG;	/* 0x14 AES decryption key [95:64]*/
	volatile uint32_t CSPI_AES_KEY3_REG;	/* 0x18 AES decryption key [127:96]*/
	volatile uint32_t CSPI_TIME_OUT_REG;	/* 0x1C Number of clocks to wait if waiting for DWC_SSI AHB, 'd 1000 */
} cspi_regs_t;

/* Bit fields in CSPI register based on the PPT */
#define CSPI_CTRL_DECRYPT_ENABLE_OFFSET 	0
#define CSPI_CTRL_RESET_SPI_CONTROL_LOGIC	1
#define CSPI_CTRL_XIP_DIRECT_MODE_ENABLE_OFFSET 4
#define CSPI_CTRL_AES_KEY_LOCK_OFFSET		5
#define CSPI_CTRL_AES_KEY_HIDE_OFFSET		6
#define CSPI_INTR_REGS_ERROR_RESP_OFFSET	2
#define CSPI_INTR_SPI_ERROR_RESP_OFFSET 	3


/* Bit fields in CTRLR0 based on DWC_ssi_databook.pdf v1.02a */
#define DWC_SSI_CTRLR0_SPI_HE_OFFSET		24		/* hyperbus enable */
#define DWC_SSI_CTRLR0_SPI_FRF_OFFSET		22		/* spi frame format - std/dual/quad/octal */
#define DWC_SSI_CTRLR0_CFS_OFFSET		16		/* control frame size - Microwire only - ignore */
#define DWC_SSI_CTRLR0_SSTE_OFFSET		14		/* slave select toggle enable */
#define DWC_SSI_CTRLR0_SRL_OFFSET		13		/* shift register loop - ignore */
#define DWC_SSI_CTRLR0_SLV_OE_OFFSET		12
#define DWC_SSI_CTRLR0_TMOD_OFFSET		10
#define DWC_SSI_CTRLR0_TMOD_MASK		(3 << DWC_SSI_CTRLR0_TMOD_OFFSET) /* read/write, read only, write only, eeprom read*/
#define DWC_SSI_CTRLR0_SCPOL_OFFSET		9
#define DWC_SSI_CTRLR0_SCPH_OFFSET		8
#define DWC_SSI_CTRLR0_FRF_OFFSET		6		/* frame format - spi/ssp/microwire */
#define DWC_SSI_CTRLR0_DFS_OFFSET		0

#define DWC_SSI_CTRLR0_IS_MST			0x80000000

#define DWC_SSI_DEFAULTS			(DWC_SSI_CTRLR0_IS_MST|7)	/* Always master, 8-bit data frame size */

#define	SPI_TMOD_TR				0x0		/* xmit & recv */
#define SPI_TMOD_TO				0x1		/* xmit only */
#define SPI_TMOD_RO				0x2		/* recv only */
#define SPI_TMOD_EPROMREAD			0x3		/* eeprom read mode */

/* Bit fields for SPI FRF */
#define SPI_SINGLE				0x0
#define SPI_DUAL				0x1
#define SPI_QUAD				0x2
#define SPI_OCTAL				0x3

/* Bit fields for Frame Format FRF */
#define SPI_FRF_SPI				0x0
#define SPI_FRF_SSP				0x1
#define SPI_FRF_MICROWIRE			0x2

/* Bit fields in CTRLR1 */
#define SPI_NDF_MASK				0x0000ffff	/* GENMASK(15, 0) */

/* Bit fields in SR, 7 bits */
#define SR_MASK					0x7f		/* cover 7 bits */
#define SR_BUSY					(1 << 0)
#define SR_TF_NOT_FULL				(1 << 1)
#define SR_TF_EMPTY				(1 << 2)
#define SR_RF_NOT_EMPT				(1 << 3)
#define SR_RF_FULL				(1 << 4)
#define SR_TX_ERR				(1 << 5)
#define SR_DCOL					(1 << 6)

/* Bit fields in ISR, IMR, RISR, 7 bits */
#define SPI_INT_TXEI				(1 << 0)
#define SPI_INT_TXOI				(1 << 1)
#define SPI_INT_RXUI				(1 << 2)
#define SPI_INT_RXOI				(1 << 3)
#define SPI_INT_RXFI				(1 << 4)
#define SPI_INT_MSTI				(1 << 5)

/* Bit fields in DMACR */
#define SPI_DMA_RDMAE				(1 << 0)
#define SPI_DMA_TDMAE				(1 << 1)

/* Bit fields in SPI_CTRLR0 */
#define DWC_SPI_CTRLR0_CLK_STRETCH_EN_OFFSET 	30
#define DWC_SPI_CTRLR0_XIP_PREFETCH_EN_OFFSET 	29
#define DWC_SPI_CTRLR0_XIP_MBL_OFFSET 		26
#define DWC_SPI_CTRLR0_SPI_RXDS_SIG_EN_OFFSET 	25
#define DWC_SPI_CTRLR0_SPI_DM_EN_OFFSET 	24
#define DWC_SPI_CTRLR0_XIP_CONT_EN_OFFSET 	21
#define DWC_SPI_CTRLR0_XIP_INST_EN_OFFSET 	20
#define DWC_SPI_CTRLR0_XIP_DFS_HC_OFFSET 	19
#define DWC_SPI_CTRLR0_SPI_RXDS_EN_OFFSET 	18
#define DWC_SPI_CTRLR0_INST_DDR_EN_OFFSET	17
#define DWC_SPI_CTRLR0_SPI_DDR_EN_OFFSET 	16
#define DWC_SPI_CTRLR0_WAIT_CYCLES_OFFSET 	11
#define DWC_SPI_CTRLR0_INST_L_OFFSET 		8
#define DWC_SPI_CTRLR0_XIP_MD_EN_OFFSET 	7
#define DWC_SPI_CTRLR0_ADDR_L_OFFSET 		2
#define DWC_SPI_CTRLR0_TRANS_TYPE_OFFSET 	0

#define DWC_SPI_CTRLR0_TRANS_TYPE_MASK	 	3
#define DWC_SPI_TRANS_TYPE_STANDARD	 	0
#define DWC_SPI_TRANS_TYPE_FRF_DEFINED		2	/* CTRLR0.SPI_FRF Defined - Standard/Dual/Quad/Octal */

#define DWC_SPI_DEFAULTS			0
#define DWC_FIFO_SIZE				256
#define SPI_WAIT_RETRIES			5

#define OSPI0_BASE		0xD0000000
#define OSPI1_BASE		0xE0000000

#define GPIO_P1_BASE		(volatile uint32_t *)0x49000000
#define AES0_BASE		(volatile uint32_t *)0x49040000
#define AES1_BASE		(volatile uint32_t *)0x49041000
#define PINMUX_BASE		(volatile uint32_t *)0x71006000


/* defines the Length of Address to be transmitted by Host controller */
#define ADDR_LENGTH_0_BITS	0x0
#define ADDR_LENGTH_8_BITS	0x2
#define ADDR_LENGTH_24_BITS	0x6
#define ADDR_LENGTH_32_BITS	0x8

/* defines the mode in which the slave Device is operating in */
#define DEVICE_MODE_SINGLE		1
#define DEVICE_MODE_DUAL		2
#define DEVICE_MODE_QUAD		4
#define DEVICE_MODE_OCTAL		8

/* defines the Slave Device */
#define DEVICE_ADESTO_NOR_FLASH	1
#define DEVICE_ISSI_NOR_FLASH	2

/* Adesto Flash Memory device constants */
#define FM_PAGE_SIZE		256

#define FMC_READ_ARRAY		0x0B
#define FMC_BLOCK_ERASE4K	0x20
#define FMC_BLOCK_ERASE32K	0x52
#define FMC_BLOCK_ERASE64K	0xD8
#define FMC_CHIP_ERASE		0x60
#define FMC_BYTE_PROGRAM	0x02
#define FMC_BUFFER_WRITE	0x84
#define FMC_BUFFER2MEMORY	0x88
#define FMC_ERASE_SUSPEND	0xB0
#define FMC_ERASE_RESUME	0xD0
#define FMC_WRITE_ENABLE	0x06
#define FMC_WRITE_DISABLE	0x04
#define FMC_PROT_SECTOR		0x36
#define FMC_UNPROT_SECTOR	0x39
#define FMC_READ_PROT		0x3C

#define FMC_PROG_OTP		0x9B
#define FMC_READ_OTP		0x77

#define FMC_READ_STATUS_C	0x65
#define FMC_READ_STATUS_B1	0x05
#define FMC_STATUS_INT		0x25
#define FMC_WRITE_STATUS_C	0x71
#define FMC_WRITE_STATUS_B1	0x01
#define FMC_WRITE_STATUS_B2	0x31

#define FMC_TERMINATE		0xF0
#define FMC_RESET_ENABLE	0x66
#define FMC_RESET		0x99
#define FMC_DEEP_POWER_DN	0xB9
#define FMC_RESUME		0xAB
#define FMC_UDEEP_PWR_DN	0x79
#define FMC_READ_SFPD		0x5A

/* Commands only used in Standard Mode */
#define FMC_READ_ARRAY3		0x03
#define FMC_READ_ARRAY4		0x13
#define FMC_BUFFER_READ 	0xD4
#define FMC_READ_ID		0x9F
#define FMC_ENTER_OCTAL		0xE8

/* Command only used in Octal Mode */
#define FMC_BURST_READ		0x0C
#define FMC_ECHO		0xAA
#define FMC_ECHO_INVERTED	0xA5
#define FMC_RETURN2SPI		0xFF


/* ISSI Flash Memory device Commands */

#define FM_PAGE_SIZE		256

#define ISSI_RESET_ENABLE	0x66
#define ISSI_RESET_MEMORY	0x99

#define ISSI_READ_ID		0x9E
#define ISSI_READ_MULTIPLE_ID	0x9F

/* READ REGISTER OPERATIONS */
#define ISSI_READ_STATUS_REG			0x05
#define ISSI_READ_FLAG_STATUS_REG		0x70
#define ISSI_READ_NONVOLATILE_CONFIG_REG	0xB5
#define ISSI_READ_VOLATILE_CONFIG_REG		0x85
#define ISSI_READ_PROTECTION_MANAGEMENT_REG	0x2B

/* READ MEMORY OPERATIONS with 3-Byte/4-Byte Address */
#define ISSI_READ				0x03
#define ISSI_FAST_READ				0x0B
#define ISSI_OCTAL_OUTPUT_FAST_READ		0x8B
#define ISSI_OCTAL_IO_FAST_READ			0xCB
#define ISSI_DDR_OCTAL_OUTPUT_FAST_READ		0x9D
#define ISSI_DDR_OCTAL_IO_FAST_READ		0xFD

/* READ MEMORY OPERATIONS with 4-Byte Address */
#define ISSI_4BYTE_READ				0x13
#define ISSI_4BYTE_FAST_READ			0x0C
#define ISSI_4BYTE_OCTAL_OUTPUT_FAST_READ	0x7C
#define ISSI_4BYTE_OCTAL_IO_FAST_READ		0xCC

/* WRITE OPERATIONS */
#define ISSI_WRITE_ENABLE	0x06
#define ISSI_WRITE_DISABLE	0x04

/* WRITE REGISTER OPERATIONS */
#define ISSI_WRITE_STATUS_REG			0x01
#define ISSI_WRITE_NONVOLATILE_CONFIG_REG	0xB1
#define ISSI_WRITE_VOLATILE_CONFIG_REG		0x81
#define ISSI_WRITE_PROTECTION_MANAGEMENT_REG	0x68

/* CLEAR OPERATIONS */
#define ISSI_CLEAR_FLAG_STATUS_REG	0x50
#define ISSI_CLEAR_ERRB			0xB6

/* PROGRAM OPERATIONS with 3-Byte/4-Byte Address */
#define ISSI_PAGE_PROGRAM			0x02
#define ISSI_OCTAL_INPUT_FAST_PROGRAM		0x82
#define ISSI_EXTN_OCTAL_INPUT_FAST_PROGRAM	0xC2

/* PROGRAM OPERATIONS with 4-Byte Address */
#define ISSI_4BYTE_PAGE_PROGRAM				0x12
#define ISSI_4BYTE_OCTAL_INPUT_FAST_PROGRAM		0x84
#define ISSI_4BYTE_EXTN_OCTAL_INPUT_FAST_PROGRAM	0x8E

/* ERASE OPERATIONS with 3-Byte/4-Byte Address */
#define ISSI_4KB_ERASE		0x20
#define ISSI_32KB_ERASE		0x52
#define ISSI_128KB_ERASE	0xD8
#define ISSI_CHIP_ERASE		0x60

/* ERASE OPERATIONS with 4-Byte Address */
#define ISSI_4BYTE_4KB_ERASE	0x5C
#define ISSI_4BYTE_32KB_ERASE	0x21
#define ISSI_4BYTE_128KB_ERASE	0xDC

/* 4-Byte ADDRESS MODE OPERATIONS */
#define ISSI_ENTER_4BYTE_ADDRESS_MODE	0xB7
#define ISSI_EXIT_4BYTE_ADDRESS_MODE	0xE9

/*Pad Control Register */
#define PAD_CTRL_REN		0x01			/* Read Enable */
#define PAD_CTRL_SMT		0x02			/* Schmitt Trigger Enable */
#define PAD_CTRL_SR		0x04			/* Fast Slew Rate Enable */
#define PAD_CTRL_HIGHZ		0			/* Driver Disabled State Control - High Z - Normal operation */
#define PAD_CTRL_PULLUP		(1<<3)			/* Driver Disabled State Control - Weak Pull-Up */
#define PAD_CTRL_PULLDN		(2<<3)			/* Driver Disabled State Control - Weak Pull-Down */
#define PAD_CTRL_REPEAT		(3<<3)			/* Driver Disabled State Control - Repeater - Bus keeper */
#define PAD_CTRL_2MA		0x00			/* Output Drive Strength 2mA */
#define PAD_CTRL_4MA		(1<<5)			/* Output Drive Strength 4mA */
#define PAD_CTRL_8MA		(2<<5)			/* Output Drive Strength 8mA */
#define PAD_CTRL_12MA		(3<<5)			/* Output Drive Strength 12mA */
#define PAD_CTRL_OPENDRAIN	0x80			/* Open Drain Driver */

/** Peripheral OSPI0 base pointer */
#define OSPI0			((ssi_regs_t *)OSPI0_BASE)
#define OSPI1			((ssi_regs_t *)OSPI1_BASE)
#define AES0			((cspi_regs_t *)AES0_BASE)
#define AES1			((cspi_regs_t *)AES1_BASE)
#define GPIO_P1			((uint32_t *)GPIO_P1_BASE)
#define ACLK			400			/* OSPI Clock in MHz */

#define PINMUX_P1_16_23		(volatile uint32_t *)0x71006018
#define PINMUX_P1_24_31		(volatile uint32_t *)0x7100601C
#define PINMUX_P2_0_7		(volatile uint32_t *)0x71006020
#define PINMUX_P2_8_15		(volatile uint32_t *)0x71006024
#define PINMUX_P2_16_23		(volatile uint32_t *)0x71006028

#define PINMUX_REG(p,b)		(volatile uint32_t *)(PINMUX_BASE + ((p*8 + b)>>2))
#define PINMUX_MASK(b)		(0x0f << (b&3))
#define PINMUX_OFFSET(b)	(b&3)
#define SET_PINMUX(p,b,c)	\
{ \
	uint32_t t = *PINMUX_REG(p,b);\
	t &= ~PINMUX_MASK(b); \
	t |= c << PINMUX_OFFSET(b); \
	PINMUX_REG(p,b) |= t; \
}

#define BYTES_PER_PAGE 		256
#define WORDS_PER_PAGE 		(BYTES_PER_PAGE>>4)
#define PAGES_PER_BLOCK 	16
#define BLOCKS_PER_SECTOR 	64

#define TXBUFF_SIZE		BYTES_PER_PAGE
#define RXBUFF_SIZE		BYTES_PER_PAGE
#define ID_BYTES 		5

typedef struct {
	ssi_regs_t * regs;		/* Pointer to OSPI0 or OSPI1 base address */
	cspi_regs_t * aes;
	uint32_t	mode;		/* Simple, XIP, Hyperbus - application defined at run time */

	uint32_t	gpio_reset;	/* GPIO to toggle Memory Device reset line - application defined */
	uint32_t	pinmux[12];	/* Pin-mux configuration - application defined */
	uint32_t	padctrl[12];	/* Pad controller configuration - bus capacitance defined */

	uint32_t	dev_type;	/* Single, Dual, Quad, Octal - device type defined */
	uint32_t	ospi_clock;	/* Octal SPI clock - Device and Bus characteristics defined */
	uint32_t	spi_ser;	/* Slave Select / Enable register - application defined (to drive ospiN_ss0 or ospiN_ss1) */
	uint32_t	xip_ser;	/* Slave Select / Enable register - application defined (to drive ospiN_ss0 or ospiN_ss1) */
	uint32_t	addrlen;	/* Address length 3 or 4 bytes depending on the device capacity */
	uint32_t	drv_strength;	/* Drive strength */
	uint32_t	ds_en;		/* DS signal enabled */
	uint32_t	ddr_en;		/* DDR enable if set to 1, default 0 */
	uint32_t	aes_en;		/* AES decryption enable if set to 1, default 0 */

	uint32_t	wait_cycles;	/* Wait cycles - Device defined for switching from Rx to Tx */
	uint32_t	rx_req;		/* Requested data to receive */
	uint32_t	rx_cnt;		/* Received data count */
	uint32_t	scbyte1;	/* Status / Control Byte 1 */
	uint32_t	scbyte2;	/* Status / Control Byte 2 */
	uint32_t	scbyte3;	/* Status / Control Byte 3 */

	uint32_t	device_id[ID_BYTES];	/* Manufacturer and Device ID */
	uint8_t		tx_buff[TXBUFF_SIZE];	/* Page buffer - 256 bytes */
	uint8_t		rx_buff[RXBUFF_SIZE];	/* Read buffer */
} ospi_cfg_t ;

uint32_t dummy_cycles[16] = {
		8, 10, 12, 14, 16, 18, 20, 22
};


volatile uint32_t ts1, ts2, pmu_adj;

#define DSB() __asm__ __volatile__ ("dsb" : : : "memory")
#define dw_readl(a, r)		(a->regs->r)
#define dw_writel(a, r, v)	a->regs->r = (v); DSB()

#define spi_enable(dws) 	dw_writel(dws, DW_SPI_SSIENR, 1)
#define spi_disable(dws) 	dw_writel(dws, DW_SPI_SSIENR, 0)
#define spi_set_clk(dws, div) 	dw_writel(dws, DW_SPI_BAUDR, div << 1)


#define PADCTRL_REG(p,b) (volatile uint32_t *)(0x71007000 + ((p-1)*32 + b)*4)

/*  OSPI Memory structure:
 * 	Sector has 64 Blocks
 *  Block has 16 Pages
 *  Page has 256 bytes (64 words)
 */

