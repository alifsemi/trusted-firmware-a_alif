/*
 * Copyright (c) 2021, Alif Semiconductor Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include "dwc_spi.h"

/* To make this work on CARRIER board pass the command line option -DCARRIER, by defaukt this for DEV Board */
/* CARRIER = Ensemble, DEV = devkit-e7 */
#ifdef PRELOADED_BL33_BASE
#if (PRELOADED_BL33_BASE == 0xE0000000)
#define CARRIER
#endif
#if (PRELOADED_BL33_BASE == 0xD0000000)
#define DEVBOARD
#endif
#endif

void setup_PinMUX()
{
#if defined (DEVBOARD) || defined (CARRIER)
#ifdef CARRIER
	/* Configure OctalSPI 1 pins - CarrierBoard
	*
	* P2_8 .. P2_15 = D0..D7
	* P2_16 = RXDS
	* P2_19 = SCLK
	* P2_17 = CS
	* P2_18 = SCLKN
	*/
	*PINMUX_P2_8_15 = 0x43444444;		/* P2_8 .. P2_15 = D0..D7 */
	*PINMUX_P2_16_23 = 0x00004434;		/* P2_16 = RXDS, P2_17 = CS, P2_19 = SCLK */

	/* Configure pad control registers */
#define PAD_CTRL_DATA (PAD_CTRL_REN)
#define PAD_CTRL_CLK (PAD_CTRL_12MA|PAD_CTRL_SR)

	*PADCTRL_REG(2, 8) = PAD_CTRL_DATA;
	*PADCTRL_REG(2, 9) = PAD_CTRL_DATA;
	*PADCTRL_REG(2, 10) = PAD_CTRL_DATA;
	*PADCTRL_REG(2, 11) = PAD_CTRL_DATA;
	*PADCTRL_REG(2, 12) = PAD_CTRL_DATA;
	*PADCTRL_REG(2, 13) = PAD_CTRL_DATA;
	*PADCTRL_REG(2, 14) = PAD_CTRL_DATA;
	*PADCTRL_REG(2, 15) = PAD_CTRL_DATA;
	*PADCTRL_REG(2, 16) = PAD_CTRL_DATA;

	*PADCTRL_REG(2, 17) = PAD_CTRL_CLK;
	*PADCTRL_REG(2, 19) = PAD_CTRL_CLK;

#else
	/* Configure OctalSPI 0 pins - DevBoard
	*
	* P1_16 .. P1_23 = D0..D7
	* P1_26 = RXDS
	* P1_25 = SCLK
	* P2_6 = CS
	* P2_7 = SCLKN
	*/
	*PINMUX_P1_16_23 = 0x34433333;		/* P1_16 .. P1_23 = D0..D7 */
	*PINMUX_P1_24_31 = 0x11111330;		/* P1_25 = SCLK, P1_26 = RXDS */
	*PINMUX_P2_0_7 = 0x04000000;		/* P2_6 = CS */

	/* Configure pad control registers */
#define PAD_CTRL_DATA (PAD_CTRL_REN)
#define PAD_CTRL_CLK (PAD_CTRL_12MA|PAD_CTRL_SR)

	*PADCTRL_REG(1, 16) = PAD_CTRL_DATA;
	*PADCTRL_REG(1, 17) = PAD_CTRL_DATA;
	*PADCTRL_REG(1, 18) = PAD_CTRL_DATA;
	*PADCTRL_REG(1, 19) = PAD_CTRL_DATA;
	*PADCTRL_REG(1, 20) = PAD_CTRL_DATA;
	*PADCTRL_REG(1, 21) = PAD_CTRL_DATA;
	*PADCTRL_REG(1, 22) = PAD_CTRL_DATA;
	*PADCTRL_REG(1, 23) = PAD_CTRL_DATA;
	*PADCTRL_REG(1, 26) = PAD_CTRL_DATA;

	*PADCTRL_REG(1, 25) = PAD_CTRL_CLK;
	*PADCTRL_REG(2, 6) = PAD_CTRL_CLK;
#endif
#endif
return;
}

/* Disable IRQ bits */
static inline void spi_mask_intr(ospi_cfg_t *dws, uint32_t mask)
{
	uint32_t new_mask;

	new_mask = dw_readl(dws, DW_SPI_IMR) & ~mask;
	dw_writel(dws, DW_SPI_IMR, new_mask);
}

/*
 * This disables the SPI controller, interrupts, clears the interrupts status
 * and CS, then re-enables the controller back. Transmit and receive FIFO
 * buffers are cleared when the device is disabled.
 */
static inline void spi_reset_chip(ospi_cfg_t *dws)
{
	spi_disable(dws);
	spi_mask_intr(dws, 0xff);
	uint32_t t = dw_readl(dws, DW_SPI_ICR);
	t = 0;
	dw_writel(dws, DW_SPI_SER, t);
}

void ospi_setup_read(ospi_cfg_t *dws, uint32_t addr_len, uint32_t read_len, uint32_t dummy_cycles)
{
	dw_writel(dws, DW_SPI_SER, 0);				/* Clear Slave Select to end previous transaction */
	spi_disable(dws);
	uint32_t t = DWC_SSI_CTRLR0_IS_MST
		|(SPI_OCTAL << DWC_SSI_CTRLR0_SPI_FRF_OFFSET)	/* Octal SPI frame format */
		|(SPI_TMOD_RO << DWC_SSI_CTRLR0_TMOD_OFFSET)	/* Transfer mode: receive only */
		|(7 << DWC_SSI_CTRLR0_DFS_OFFSET);		/* 8-bit data frame */
	dw_writel(dws, DW_SPI_CTRLR0, t);
	dw_writel(dws, DW_SPI_CTRLR1, read_len - 1);		/* Number of data frames to receive */

	t = DWC_SPI_TRANS_TYPE_FRF_DEFINED				/* OctalSPI transfer type */
		|((dws->ds_en) << DWC_SPI_CTRLR0_SPI_RXDS_EN_OFFSET)	/* Enable Data Strobe */
		|(2 << DWC_SPI_CTRLR0_INST_L_OFFSET)			/* Instruction length - always 8-bit */
		|(addr_len << (DWC_SPI_CTRLR0_ADDR_L_OFFSET+1)) 	/* Address length - X */
		|(dummy_cycles << DWC_SPI_CTRLR0_WAIT_CYCLES_OFFSET);	/* Dummy cycles - Y */
	dw_writel(dws, DW_SPI_CS_CTRLR0, t);
	dws->rx_req = read_len;						/* Save the requested receive count for synchronization */
	spi_enable(dws);
}

void ospi_setup_write(ospi_cfg_t *dws, uint32_t addr_len)
{
	dw_writel(dws, DW_SPI_SER, 0);				/* Clear Slave Select to end previous transaction */
	spi_disable(dws);
	uint32_t t = DWC_SSI_CTRLR0_IS_MST
		|(SPI_OCTAL << DWC_SSI_CTRLR0_SPI_FRF_OFFSET)	/* Octal SPI frame format */
		|(SPI_TMOD_TO << DWC_SSI_CTRLR0_TMOD_OFFSET)	/* Transfer mode: transmit only */
		|(7 << DWC_SSI_CTRLR0_DFS_OFFSET);		/* 8-bit data frame */
	dw_writel(dws, DW_SPI_CTRLR0, t);
	dw_writel(dws, DW_SPI_CTRLR1, 0);			/* No data frames to be received */

	t = DWC_SPI_TRANS_TYPE_FRF_DEFINED				/* OctalSPI transfer type */
		|((dws->ds_en) << DWC_SPI_CTRLR0_SPI_RXDS_EN_OFFSET)	/* Enable Data Strobe */
		|(2 << DWC_SPI_CTRLR0_INST_L_OFFSET)			/* Instruction length - always 8-bit */
		|(addr_len << (DWC_SPI_CTRLR0_ADDR_L_OFFSET+1)) 	/* Address length - X */
		|(0 << DWC_SPI_CTRLR0_WAIT_CYCLES_OFFSET);		/* No dummy cycles */
	dw_writel(dws, DW_SPI_CS_CTRLR0, t);
	spi_enable(dws);
}

inline static void ospi_send(ospi_cfg_t *dws, uint32_t v)
{
	dw_writel(dws, DW_SPI_DR, v);			/* Write data payload */
	dw_writel(dws, DW_SPI_SER, dws->spi_ser);	/* Set Slave Select to start transaction */
	while ((dw_readl(dws, DW_SPI_SR) & (SR_TF_EMPTY|SR_BUSY)) != SR_TF_EMPTY)
	{
	}
}

inline static void ospi_push(ospi_cfg_t *dws, uint32_t v)
{
	dw_writel(dws, DW_SPI_DR, v);
}

void ospi_write_en(ospi_cfg_t *dws)
{
	/* Write WEL bit in OctalSPI mode */
	ospi_setup_write(dws, 0);
	ospi_send(dws, FMC_WRITE_ENABLE);	/* Write data payload */
}

void ospi_set_scbyte(ospi_cfg_t *dws, uint32_t a, uint32_t v)
{
	/* Prior to writing Status Registers, Write Enable Command have to be issued */
	ospi_write_en(dws);

	ospi_setup_write(dws, 1);
	ospi_push(dws, FMC_WRITE_STATUS_C);	/* Write Status Register command */
	ospi_push(dws, a);			/* Write address byte */
	ospi_send(dws, v);			/* Write data byte */
}

void ospi_xip_enter(ospi_cfg_t *dws)
{
	/* Configure SSI XIP setting */
	spi_disable(dws);

	uint32_t t = DWC_SSI_CTRLR0_IS_MST
		|(SPI_OCTAL << DWC_SSI_CTRLR0_SPI_FRF_OFFSET)			/* Octal SPI frame format */
		|(0 << DWC_SSI_CTRLR0_SCPOL_OFFSET)
		|(0 << DWC_SSI_CTRLR0_SCPH_OFFSET)
		|(0 << DWC_SSI_CTRLR0_SSTE_OFFSET)
		|(SPI_TMOD_RO << DWC_SSI_CTRLR0_TMOD_OFFSET)			/* Transfer mode: receive only */
		|(31 << DWC_SSI_CTRLR0_DFS_OFFSET);				/* 32-bit data frame */
	dw_writel(dws, DW_SPI_CTRLR0, t);
	dw_writel(dws, DW_SPI_CTRLR1, 0x00000010);				/* 16 bytes read */
	t = DWC_SPI_TRANS_TYPE_FRF_DEFINED					/* OctalSPI transfer type */
		|((dws->ds_en) << DWC_SPI_CTRLR0_SPI_RXDS_EN_OFFSET)		/* Enable Data Strobe */
		|((dws->ddr_en) << DWC_SPI_CTRLR0_SPI_DDR_EN_OFFSET)		/* Enable SDR/DDR */
		|(2 << DWC_SPI_CTRLR0_XIP_MBL_OFFSET)
		|(1 << DWC_SPI_CTRLR0_XIP_DFS_HC_OFFSET)			/* DFS set to 32-bit */
		|(1 << DWC_SPI_CTRLR0_XIP_INST_EN_OFFSET)
		|(2 << DWC_SPI_CTRLR0_INST_L_OFFSET)				/* Instruction length - always 8-bit */
		|((dws->addrlen) << (DWC_SPI_CTRLR0_ADDR_L_OFFSET+1)) 		/* Address length - 4 bytes */
		|(dws->wait_cycles << DWC_SPI_CTRLR0_WAIT_CYCLES_OFFSET);	/* Dummy cycles - Y */
	dw_writel(dws, DW_SPI_CS_CTRLR0, t);

	dw_writel(dws, DW_XIP_INCR_INST, 0x0b);					/* Read Array instruction */
	dw_writel(dws, DW_XIP_WRAP_INST, 0x0c); 				/* Burst Read with Wrap instruction */
	dw_writel(dws, DW_XIP_SER, dws->xip_ser);
	dw_writel(dws, DW_SPI_SER, dws->spi_ser);
	dw_writel(dws, DW_XIP_CNT_TIME_OUT, 100);				/* Continuous XIP mode deselect timeout in hclk */
	spi_enable(dws);
}

void cspi_decrypt_enable(ospi_cfg_t *dws, uint8_t *key) __attribute__((optimize("-O0")));

void cspi_decrypt_enable(ospi_cfg_t *dws, uint8_t *key)
{
	uint32_t t;
	if (dws->aes_en)
	{
		dws->aes->CSPI_AES_KEY0_REG = (key[12U+3U] & 0xFF) | ((key[12U+2U] & 0xFF) << 8) |
			((key[12U+1U] & 0xFF) << 16) | ((key[12U] & 0xFF) << 24); /* reg[31:0] */
		dws->aes->CSPI_AES_KEY1_REG = (key[8U+3U] & 0xFF) | ((key[8U+2U] & 0xFF) << 8) |
			((key[8U+1U] & 0xFF) << 16) | ((key[8U] & 0xFF) << 24); /* reg[63:32] */
		dws->aes->CSPI_AES_KEY2_REG = (key[4U+3U] & 0xFF) | ((key[4U+2U] & 0xFF) << 8) |
			((key[4U+1U] & 0xFF) << 16) | ((key[4U] & 0xFF) << 24); /* reg[95:64] */
		dws->aes->CSPI_AES_KEY3_REG = (key[3U] & 0xFF) | ((key[2U] & 0xFF) << 8) |
			((key[1U] & 0xFF) << 16) | ((key[0] & 0xFF) << 24) ; /* reg[127:96] */

		t = (1<<CSPI_CTRL_DECRYPT_ENABLE_OFFSET)
			| (1<<CSPI_CTRL_XIP_DIRECT_MODE_ENABLE_OFFSET);
		dws->aes->CSPI_CTRL_REG |= t; DSB();
	}
	else
	{
		t = (1<<CSPI_CTRL_XIP_DIRECT_MODE_ENABLE_OFFSET);
		dws->aes->CSPI_CTRL_REG |= t; DSB();
	}
	return;
}

void ospi_init(ospi_cfg_t *dws)
{
	spi_disable(dws);				/* Disable SSI operation */
	dw_writel(dws, DW_SPI_SER, 0);			/* Clear Slave Select register */

	spi_set_clk(dws, ACLK / ((dws->ospi_clock)<<1));/* Set Octal SPI Clock Divider - Peripheral Clock / (Desired Clock * 2) */
	dw_writel(dws, DW_SPI_TXFTLR, 0);		/* tx fifo threshold - start TX trigger / signal FIFO depletion */
	dw_writel(dws, DW_SPI_RXFTLR, 16);		/* rx fifo threshold - tolerate SW/IRQ latency */

	/* Set SSE in standard format because the OSPI memory boots expecting commands shifted on SD0 */
	volatile uint32_t t;
	t = DWC_SSI_CTRLR0_IS_MST
		|(SPI_OCTAL << DWC_SSI_CTRLR0_SPI_FRF_OFFSET)	/* Octal SPI frame format */
		|(SPI_TMOD_TO << DWC_SSI_CTRLR0_TMOD_OFFSET)	/* Transmit only mode */
		|(7 << DWC_SSI_CTRLR0_DFS_OFFSET);		/* 8-bit data frame */
	dw_writel(dws, DW_SPI_CTRLR0, t);
	dw_writel(dws, DW_SPI_CTRLR1, 0);			/* Number of data frames to receive cleared */

	t = DWC_SPI_TRANS_TYPE_STANDARD				/* Standard transfer type */
		|(2 << DWC_SPI_CTRLR0_INST_L_OFFSET)		/* Instruction length - 8-bit */
		|(0 << DWC_SPI_CTRLR0_ADDR_L_OFFSET)		/* Address length - 0 */
		|(0 << DWC_SPI_CTRLR0_WAIT_CYCLES_OFFSET);	/* Wait cycles are ignored in Standard frame format */
	dw_writel(dws, DW_SPI_CS_CTRLR0, t);

	spi_enable(dws);

	ospi_send(dws, FMC_WRITE_ENABLE);			/* Set write enable to switch to OctalSPI protocol*/
	ospi_send(dws, FMC_ENTER_OCTAL);			/* Request switch to OctalSPI protocol */

	ospi_set_scbyte(dws, 129, dws->drv_strength);		/* set drive strength */
	return;
}


void flash_set_dummy_cycles_and_wrap_size(ospi_cfg_t *dws, uint32_t wrap_mode, uint32_t wrap_size) __attribute__((optimize("-O0")));
/*
 * Function: flash_set_dummy_cycles_and_wrap_size
 * Configure number of dummy cycles (for octal-SPI reads) and wrap size (if
 * applicable) in flash device
 * Arguments:
 * dws->wait_cycles: number of dummy cycles to be configured (range 8-22, even  values only)
 * wrap_mode: 0 for wrap-around and 1 for wrap-and-continue
 * wrap_size: size of wrap window for wrapped reads (8, 16, 32 or 64)
 */
void flash_set_dummy_cycles_and_wrap_size(ospi_cfg_t *dws, uint32_t wrap_mode, uint32_t wrap_size)
{
	uint8_t bit_val_dummy = 0, bit_val_wrap = 0;
	uint32_t cycles = 0;
	cycles = dws->wait_cycles; 	/* dummy_cycles to be the same on SPI controller and Flash */
	/* Check for validity of dummy cycle argument */
	if(cycles < 8 || cycles > 22 || (cycles & 0x1))
		return;

	bit_val_dummy = (cycles - 8) >> 1;

	switch(wrap_size)
	{
		/* Note: we only set wrap-around */
		case 8:
			bit_val_wrap = 0;
			break;
		case 16:
			bit_val_wrap = 1;
			break;
		case 32:
			bit_val_wrap = 2;
			break;
		case 64:
			bit_val_wrap = 3;
			break;
		default:
			bit_val_wrap = 3;
			break;
	}
	if(wrap_mode == 1)
		bit_val_wrap = bit_val_wrap + 4;

	/* TODO: The below loop is for delay cycles
	 * Changes to the Devkit-ex for SRAM SYSTOP_PWR_REQ
	 * host_mem_write(0x1A010400, 0x00000038);
	 */
	for(cycles = 0; cycles <= 0x100; cycles++);
	/* write calculated values to Status/Control Register 3 of Adesto */
	ospi_set_scbyte(dws, 3, ((bit_val_wrap << 5) | 0x10 | bit_val_dummy));
	return;
}

void flash_set_Data_mode(ospi_cfg_t *dws) __attribute__((optimize("-O0")));
/*
 * Function: flash_set_Data_mode
 * Configure the Flash for SDR or DDR mode based on the user parameter
 * Arguments:
 * dws->ddr_en: set to SDR if 0 and set to DDR if 1.
 */
void flash_set_Data_mode(ospi_cfg_t *dws)
{
	uint32_t scbyte2, cycles;
	scbyte2 = dws->ddr_en ? 0x88 : 0x08;
	for(cycles = 0; cycles <= 0x100; cycles++);
	/* write calculated values to Status/Control Register 2 of Adesto */
	ospi_set_scbyte(dws, 2, scbyte2);
	return;
}

inline static void ospi_send_recv(ospi_cfg_t *dws, uint32_t v)
{
        dw_writel(dws, DW_SPI_DR, v);                   /* Write data payload */
        dw_writel(dws, DW_SPI_SER, dws->spi_ser);       /* Set Slave Select to start transaction */

        uint8_t * p = dws->rx_buff;                     /* Init RX buffer */

        dws->rx_cnt = 0;
        /* SSI is busy if the TX FIFO is not empty OR the BUSY flat is set */
        while (dws->rx_cnt < dws->rx_req)
        {
                while (dw_readl(dws, DW_SPI_RXFLR) > 0)
                {
                        uint32_t t = dw_readl(dws, DW_SPI_DR);
                        if (dws->rx_cnt < RXBUFF_SIZE)
                        {
                                *p++ = (uint8_t)t;
                                dws->rx_cnt++;
                        }
                }
        }
}

uint32_t ospi_get_scbytes(ospi_cfg_t *dws)
{
        /* Read Memory Status register in OctalSPI mode */
        ospi_setup_read(dws, 1, 3, 4);
        ospi_push(dws, FMC_READ_STATUS_C);	/* Get Status/Control Registers command */
        ospi_send_recv(dws, 1);			/* Address byte */
        dws->scbyte1 = (uint32_t)dws->rx_buff[0];
        dws->scbyte2 = (uint32_t)dws->rx_buff[1];
        dws->scbyte3 = (uint32_t)dws->rx_buff[2];
        return (uint32_t)dws->rx_buff[0] | ((uint32_t)dws->rx_buff[1]<<8) | ((uint32_t)dws->rx_buff[2]<<16);
}

ospi_cfg_t ospi;
uint8_t * aes_key;

int init_nor_flash(void)
{
	ospi_cfg_t *dws = &ospi;

	/* OSPI Configuration settings */
#ifdef CARRIER
	dws->regs = OSPI1;
	dws->aes = AES1;
#else
	dws->regs = OSPI0;
	dws->aes = AES0;
#endif
	dws->spi_ser = 1;
	dws->xip_ser = 1;
	dws->addrlen = 4;
	dws->ospi_clock = 30;		/* OSPI clock in MHz */
	dws->drv_strength = 1;		/* Drive strength 0 - normal, 4 - higher, 1 - highest speed */
	dws->ds_en = 1;			/* DS signal Enabled */
	dws->ddr_en = 0;        	/* DDR is Disabled for now */
	dws->aes_en = 0;        	/* AES decrypt disable by default */

#if ENABLE_AES != 0
	{
		dws->aes_en = 1;        		/* AES decrypt enable */
		aes_key = (uint8_t *)  AES_ENC_KEY;	/* AES Key */
	}
#else
	{
		dws->aes_en = 0;        	/* AES decrypt disable */
	}
#endif
	dws->wait_cycles = 14;		/* Dummy cycles should be between 8 and 22 cycles */

	setup_PinMUX();

	/* Initialize flash driver */
	ospi_init(dws);

	/* Set dummy cycles and Wrap around 0 or Wrap continuous 1 */
	flash_set_dummy_cycles_and_wrap_size(dws, 0, 64);

	/* Read back register settings */
	ospi_get_scbytes(dws);

	/* Set DDR or SDR operating mode */
	flash_set_Data_mode(dws);

	/* Switch Octal SPI to memory mapped mode */
	ospi_xip_enter(dws);

	/* Enable AES Decryption on CSPI */
	cspi_decrypt_enable(dws, aes_key);

  return 0;
}

