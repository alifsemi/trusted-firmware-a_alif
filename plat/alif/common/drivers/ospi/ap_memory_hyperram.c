/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file ap_memory_hyperram.c
 * @brief AP Memory APS512Mb PSRAM driver for OSPI interface
 *
 * This driver initializes and configures the APS512Mb PSRAM for XIP access
 * via the OSPI0 controller in dual-octal DDR mode.
 */

#include <plat/alif/common/plat_alif.h>
#include <lib/mmio.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <plat/alif/common/drivers/pinconf.h>
#include <plat/alif/common/drivers/ospi.h>
#include <plat/alif/common/drivers/ap_memory_hyperram.h>
#include <arch_helpers.h>
#include <lib/utils_def.h>

/* APS512Mb PSRAM Constants */
#define APS512Mb_ID                           0xDE
#define APS512Mb_CMD_SYNC_READ                0x00
#define APS512Mb_CMD_SYNC_WRITE               0x80
#define APS512Mb_CMD_LINEAR_BURST_READ        0x20
#define APS512Mb_CMD_LINEAR_BURST_WRITE       0xA0
#define APS512Mb_CMD_MODE_REGISTER_READ       0x40
#define APS512Mb_CMD_MODE_REGISTER_WRITE      0xC0
#define APS512Mb_CMD_GLOBAL_RESET             0xFF

/* APS512Mb Wait Cycles */
#define APS512Mb_INIT_REG_READ_WAIT_CYCLES    4
#define APS512Mb_REG_WRITE_WAIT_CYCLES        0
#define APS512Mb_RESET_WAIT_CYCLES            3
#define APS512Mb_INIT_READ_WRITE_WAIT_CYCLES  4

/* APS512Mb Mode Register Addresses */
#define APS512Mb_MODE_REG0_ADDR               0x0
#define APS512Mb_MODE_REG1_ADDR               0x1
#define APS512Mb_MODE_REG2_ADDR               0x2
#define APS512Mb_MODE_REG4_ADDR               0x4
#define APS512Mb_MODE_REG8_ADDR               0x8

/* APS512Mb Mode Register Bit Fields */
#define APS512Mb_MODE_REG0_DRIVE_STR          0
#define APS512Mb_MODE_REG0_READ_LATENCY_CODE  2
#define APS512Mb_MODE_REG0_LATENCY_TYPE       5
#define APS512Mb_MODE_REG4_WRITE_LATENCY_CODE 5
#define APS512Mb_MODE_REG4_READ_RF_RATE       3
#define APS512Mb_MODE_REG4_PASR               0
#define APS512Mb_MODE_REG8_TRANSFER_MODE      6
#define APS512Mb_MODE_REG8_RBX_READ_EN        3
#define APS512Mb_MODE_REG8_BURST_TYPE         2
#define APS512Mb_MODE_REG8_BURST_LEN          0

/* APS512Mb DFS for register access */
#define APS512Mb_OSPI_REG_DFS                 16

/* PSRAM Configuration Values */
#define PSRAM_WAIT_CYCLES                      4
#define PSRAM_DUAL_OCTAL_MODE_ENABLE           1
#define PSRAM_OSPI_SS_LINE                     0

/* HyperRAM XIP Base Address and Size */
#define HYPERRAM_XIP_BASE                      0xA0000000
#define HYPERRAM_SIZE_64MB                     (64 * 1024 * 1024)

/* SPI Frame Formats */
#define SPI_FRF_OCTAL                          3
#define SPI_FRF_DUAL_OCTAL                     3

/* SPI Address Length */
#define SPI_ADDR_L_0_BIT                       0x00
#define SPI_ADDR_L_32_BIT                      0x08

/* SPI Instruction Length */
#define SPI_INST_L_0_BIT                       0x00
#define SPI_INST_L_8_BIT                       0x02
#define SPI_INST_L_16_BIT                      0x03

/* SPI Transfer Types */
#define SPI_TRANS_TYPE_FRF_DUAL_OCTAL          3
#define SPI_TRANS_TYPE_FRF_DEFINED             2

/* OSPI Controller Configuration */
#define OSPI_BAUDR_VALUE                       0x4
#define OSPI_RX_SAMPLE_DLY_VALUE               0x8
#define OSPI_TXD_DRIVE_EDGE_VALUE              0x1
#define OSPI_TX_FIFO_DEPTH                     256
#define OSPI_TXFTLR_TFT_SHIFT                  16

/* OSPI CTRLR0 Register */
#define OSPI_CTRLR0_DFS_MASK                   0x1F
#define OSPI_CTRLR0_SPI_FRF_MASK               (0x3 << OSPI_CTRLR0_SPI_FRF_OFFSET)
#define OSPI_CTRLR0_DFS_32BIT                  31
#define OSPI_FRAME_SIZE_16BIT_THRESHOLD        0x0F

/* AES RXDS Delay Configuration */
#define AES_RXDS_DELAY_FALL_EDGE               15
#define AES_RXDS_DELAY_RISE_EDGE               13
#define AES_RXDS_DELAY_RISE_SHIFT              8

/* XIP Configuration */
#define XIP_CNT_TIMEOUT_VALUE                  255
#define XIP_INST_SHIFT                         8

/* XIP Control Register Bit Positions */
#define XIP_CTRL_FRF_POS                       0
#define XIP_CTRL_TRANS_TYPE_POS                2
#define XIP_CTRL_ADDR_L_POS                    4
#define XIP_CTRL_INST_L_POS                    9
#define XIP_CTRL_WAIT_CYCLES_POS               13
#define XIP_CTRL_DFS_HC_POS                    18
#define XIP_CTRL_DDR_EN_POS                    19
#define XIP_CTRL_INST_DDR_EN_POS               20
#define XIP_CTRL_RXDS_EN_POS                   21
#define XIP_CTRL_INST_EN_POS                   22
#define XIP_CTRL_RXDS_SIG_EN_POS               25

/* XIP Write Control Register Bit Positions */
#define XIP_WR_CTRL_FRF_POS                    0
#define XIP_WR_CTRL_TRANS_TYPE_POS             2
#define XIP_WR_CTRL_ADDR_L_POS                 4
#define XIP_WR_CTRL_INST_L_POS                 8
#define XIP_WR_CTRL_DDR_EN_POS                 10
#define XIP_WR_CTRL_INST_DDR_EN_POS            11
#define XIP_WR_CTRL_RXDS_SIG_EN_POS            13
#define XIP_WR_CTRL_DM_EN_POS                  14
#define XIP_WR_CTRL_WAIT_CYCLES_POS            16
#define XIP_WR_CTRL_DFS_HC_POS                 21

/* AES Address Control Register Bit Positions */
#define AES_ADDR_CTRL_ARRAY_MASK_POS           0
#define AES_ADDR_CTRL_SS0_ARRAY_MODE_POS       18
#define AES_ADDR_CTRL_SS1_ARRAY_MODE_POS       19
#define AES_ADDR_CTRL_ARRAY_MODE_SHIFT_POS     22
#define AES_ADDR_CTRL_ARRAY_MODE_SPLIT_POS     28

/* Dual Octal Mode Address Configuration */
#define DUAL_OCTAL_ADDR_MASK                   0x7FF
#define DUAL_OCTAL_ADDR_LOWER_BITS             11U
#define DUAL_OCTAL_ADDR_UPPER_SHIFT            12

/* Common Values */
#define DDR_MODE_ENABLE                        1
#define DDR_MODE_DISABLE                       0
#define FEATURE_ENABLE                         1
#define FEATURE_DISABLE                        0

/* Data Bus Width */
#define DATA_BUS_WIDTH_BITS                    32


/**
 * @brief OSPI transfer helper structure for PSRAM operations
 */
typedef struct {
	uint32_t spi_frf;
	uint32_t addr_len;
	uint32_t dummy_cycle;
	uint32_t tx_total_cnt;
	uint32_t tx_current_cnt;
	uint32_t rx_total_cnt;
	uint32_t rx_current_cnt;
	const uint32_t *tx_buff;
	void *rx_buff;
	uint32_t tx_default_val;
	uint32_t ddr;
	uint32_t inst_len;
} ospi_psram_transfer_t;

/**
 * @brief Write data to OSPI in polling mode
 */
static void ospi_psram_send_polling(ospi_cfg_t *ospi, ospi_psram_transfer_t *config)
{
	uint32_t val, tx_count, curr_fifo_level;
	ospi_regs_t *regs = ospi->regs;
	const uint32_t *tx_buff = config->tx_buff;
	uint32_t tx_current = 0;

	ospi_disable(ospi);

	/* Configure control registers */
	val = regs->ctrlr0;
	val &= ~(OSPI_CTRLR0_TMOD_MASK | OSPI_CTRLR0_SPI_FRF_MASK);
	val |= (SPI_TMOD_TO << OSPI_CTRLR0_TMOD_OFFSET);
	val |= (config->spi_frf << OSPI_CTRLR0_SPI_FRF_OFFSET);
	regs->ctrlr0 = val;

	regs->ctrlr1 = FEATURE_DISABLE;

	/* Configure SPI control register */
	val = (config->tx_default_val << OSPI_SPI_CTRLR0_TRANS_TYPE_OFFSET)
	      | (FEATURE_ENABLE << OSPI_SPI_CTRLR0_SPI_RXDS_EN_OFFSET)
	      | (config->ddr << OSPI_SPI_CTRLR0_SPI_DDR_EN_OFFSET)
	      | (config->inst_len << OSPI_SPI_CTRLR0_INST_L_OFFSET)
	      | (config->addr_len << OSPI_SPI_CTRLR0_ADDR_L_OFFSET)
	      | (config->dummy_cycle << OSPI_SPI_CTRLR0_WAIT_CYCLES_OFFSET);
	regs->spi_ctrlr0 = val;

	ospi_enable(ospi);

	/* Transmit data with proper FIFO handling */
	while (config->tx_total_cnt != tx_current) {
		/* Wait for TX FIFO not full */
		while (!(regs->sr & SR_TF_NOT_FULL));

		/* Get current FIFO level */
		curr_fifo_level = regs->txflr;

		/* Calculate words to transmit */
		if (config->tx_total_cnt >= (tx_current + OSPI_TX_FIFO_DEPTH - curr_fifo_level)) {
			tx_count = OSPI_TX_FIFO_DEPTH - curr_fifo_level;
		} else {
			tx_count = config->tx_total_cnt - tx_current;
		}

		/* Set TX FIFO threshold */
		regs->txftlr |= ((tx_count - 1) << OSPI_TXFTLR_TFT_SHIFT);

		/* Write data to FIFO */
		for (uint32_t i = 0; i < tx_count; i++) {
			regs->datareg = *tx_buff;
			tx_buff++;
			tx_current++;
		}
	}

	/* Wait for completion */
	while (regs->sr & SR_BUSY);

	ospi_disable(ospi);
}

/**
 * @brief Transfer data (read) from OSPI in polling mode
 */
static void ospi_psram_transfer_polling(ospi_cfg_t *ospi, ospi_psram_transfer_t *config)
{
	uint32_t val, index, rx_count;
	uint16_t *rx_data;
	uint16_t frame_size;
	ospi_regs_t *regs = ospi->regs;
	uint32_t rx_current = 0;

	ospi_disable(ospi);

	/* Configure control registers - clear SPI_FRF before setting */
	val = regs->ctrlr0;
	val &= ~(OSPI_CTRLR0_TMOD_MASK | OSPI_CTRLR0_SPI_FRF_MASK);
	val |= (SPI_TMOD_RO << OSPI_CTRLR0_TMOD_OFFSET);
	val |= (config->spi_frf << OSPI_CTRLR0_SPI_FRF_OFFSET);
	regs->ctrlr0 = val;

	regs->ctrlr1 = config->rx_total_cnt - 1;

	/* Configure SPI control register */
	val = (config->tx_default_val << OSPI_SPI_CTRLR0_TRANS_TYPE_OFFSET)
	      | (FEATURE_ENABLE << OSPI_SPI_CTRLR0_SPI_RXDS_EN_OFFSET)
	      | (config->ddr << OSPI_SPI_CTRLR0_SPI_DDR_EN_OFFSET)
	      | (config->inst_len << OSPI_SPI_CTRLR0_INST_L_OFFSET)
	      | (config->addr_len << OSPI_SPI_CTRLR0_ADDR_L_OFFSET)
	      | (config->dummy_cycle << OSPI_SPI_CTRLR0_WAIT_CYCLES_OFFSET);
	regs->spi_ctrlr0 = val;

	ospi_enable(ospi);

	/* Send command and address */
	for (index = 0; index < config->tx_total_cnt; index++) {
		regs->datareg = config->tx_buff[index];
	}

	/* Wait for transfer to complete */
	while (regs->sr & SR_BUSY);

	/* Receive data */
	rx_data = (uint16_t *)config->rx_buff;
	while (config->rx_total_cnt != rx_current) {
		frame_size = regs->ctrlr0 & OSPI_CTRLR0_DFS_MASK;
		rx_count = regs->rxflr;

		if (frame_size > OSPI_FRAME_SIZE_16BIT_THRESHOLD) {
			/* 32-bit frame */
			for (index = 0; index < rx_count; index++) {
				*((uint32_t *)config->rx_buff) = regs->datareg;
				config->rx_buff = (uint8_t *)config->rx_buff + sizeof(uint32_t);
				rx_current++;
			}
		} else {
			/* 16-bit frame */
			for (index = 0; index < rx_count; index++) {
				rx_data[rx_current] = (uint16_t)regs->datareg;
				rx_current++;
			}
		}
	}

	ospi_disable(ospi);
}

/**
 * @brief Send global reset command to APS512Mb PSRAM
 */
static void aps512mb_global_reset(ospi_cfg_t *ospi)
{
	uint32_t reset_cmd = APS512Mb_CMD_GLOBAL_RESET;
	ospi_psram_transfer_t config;

	config.addr_len = SPI_ADDR_L_0_BIT;
	config.dummy_cycle = APS512Mb_RESET_WAIT_CYCLES;
	config.tx_total_cnt = 1;
	config.tx_current_cnt = 0;
	config.tx_buff = &reset_cmd;
	config.tx_default_val = SPI_TRANS_TYPE_FRF_DEFINED;
	config.spi_frf = SPI_FRF_OCTAL;
	config.ddr = DDR_MODE_ENABLE;
	config.inst_len = SPI_INST_L_8_BIT;

	ospi_psram_send_polling(ospi, &config);
}

/**
 * @brief Read APS512Mb mode register
 */
static uint8_t aps512mb_read_reg(ospi_cfg_t *ospi, uint32_t reg_addr, uint8_t wait_cycles)
{
	uint32_t cmd_addr_buff[2];
	uint16_t reg_value = 0xFFFF;
	ospi_psram_transfer_t config;

	cmd_addr_buff[0] = APS512Mb_CMD_MODE_REGISTER_READ;
	cmd_addr_buff[1] = reg_addr;

	config.addr_len = SPI_ADDR_L_32_BIT;
	config.dummy_cycle = wait_cycles;
	config.tx_total_cnt = 2;
	config.tx_current_cnt = 0;
	config.rx_total_cnt = 1;
	config.rx_current_cnt = 0;
	config.tx_buff = cmd_addr_buff;
	config.rx_buff = &reg_value;
	config.tx_default_val = SPI_TRANS_TYPE_FRF_DEFINED;
	config.spi_frf = SPI_FRF_OCTAL;
	config.ddr = DDR_MODE_ENABLE;
	config.inst_len = SPI_INST_L_8_BIT;

	ospi_psram_transfer_polling(ospi, &config);

	return (uint8_t)(reg_value >> XIP_INST_SHIFT);
}

/**
 * @brief Write APS512Mb mode register
 */
static void aps512mb_write_reg(ospi_cfg_t *ospi, uint32_t reg_addr, uint8_t reg_val)
{
	uint32_t write_buff[3];
	ospi_psram_transfer_t config;

	write_buff[0] = APS512Mb_CMD_MODE_REGISTER_WRITE;
	write_buff[1] = reg_addr;
	/* Creating data for 16 bits as controller is in DDR mode */
	write_buff[2] = (reg_val << XIP_INST_SHIFT | reg_val);

	config.addr_len = SPI_ADDR_L_32_BIT;
	config.dummy_cycle = APS512Mb_REG_WRITE_WAIT_CYCLES;
	config.tx_total_cnt = 3;
	config.tx_current_cnt = 0;
	config.tx_buff = write_buff;
	config.tx_default_val = SPI_TRANS_TYPE_FRF_DEFINED;
	config.spi_frf = SPI_FRF_OCTAL;
	config.ddr = DDR_MODE_ENABLE;
	config.inst_len = SPI_INST_L_8_BIT;

	ospi_psram_send_polling(ospi, &config);
}

/**
 * @brief Set OSPI data frame size
 */
static void ospi_psram_set_dfs(ospi_cfg_t *ospi, uint8_t dfs)
{
	uint32_t val;
	ospi_regs_t *regs = ospi->regs;

	ospi_disable(ospi);

	val = regs->ctrlr0;
	val &= ~OSPI_CTRLR0_DFS_MASK;  /* Clear DFS bits */
	val |= (dfs - 1);
	regs->ctrlr0 = val;

	ospi_enable(ospi);
}

/**
 * @brief Configure HyperRAM/PSRAM OSPI pins
 */
static void ap_memory_pinmux_config(void)
{
	/* Pad control for data pins - need READ_ENABLE + SLEW_RATE_FAST + high drive strength */
	uint32_t pad_ctrl_data = PADCTRL_READ_ENABLE | PADCTRL_SLEW_RATE_FAST | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA;
	/* Pad control for clock/CS pins - NO READ_ENABLE, just SLEW_RATE_FAST + high drive strength */
	uint32_t pad_ctrl_clk = PADCTRL_SLEW_RATE_FAST | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA;

	/* Data pins PORT_2[0:7] - OSPI D0-D7 */
	pinconf_set(PORT_2, PIN_0, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_2, PIN_1, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_2, PIN_2, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_2, PIN_3, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_2, PIN_4, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_2, PIN_5, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_2, PIN_6, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_2, PIN_7, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);

	/* Data pins PORT_16[0:7] - OSPI D8-D15 (for x16/dual-octal mode) */
	pinconf_set(PORT_16, PIN_0, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_16, PIN_1, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_16, PIN_2, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_16, PIN_3, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_16, PIN_4, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_16, PIN_5, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_16, PIN_6, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);
	pinconf_set(PORT_16, PIN_7, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data);

	/* Clock and CS pins - NO READ_ENABLE (output only) */
	pinconf_set(PORT_3, PIN_0, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_clk);  /* OSPI_SCLK */
	pinconf_set(PORT_3, PIN_2, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_clk);  /* OSPI_CS */

	/* DQS/RWDS and control pins - need READ_ENABLE */
	pinconf_set(PORT_1, PIN_6, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data); /* OSPI_DQS/RWDS0 */
	pinconf_set(PORT_8, PIN_5, PINMUX_ALTERNATE_FUNCTION_1, pad_ctrl_data); /* OSPI_DQS/RWDS1 */
}

/**
 * @brief Initialize OSPI controller for APS512Mb PSRAM
 */
static void ospi_psram_controller_init(ospi_cfg_t *ospi_cfg)
{
	ospi_cfg->regs = (ospi_regs_t *)OSPI0_BASE;
	ospi_cfg->aes_regs = (aes_regs_t *)AES0_BASE;
	ospi_cfg->ss = PSRAM_OSPI_SS_LINE;

	ospi_disable(ospi_cfg);

	INFO("AP Memory: OSPI Controller Configuration\n");
	writel(ospi_cfg, baudr, OSPI_BAUDR_VALUE);
	writel(ospi_cfg, rx_sample_dly, OSPI_RX_SAMPLE_DLY_VALUE);
	writel(ospi_cfg, txd_drive_edge, OSPI_TXD_DRIVE_EDGE_VALUE);

	/* Configure AES RXDS delay for DDR mode
	 * Bits [7:0]: RXDS delay for falling edge
	 * Bits [15:8]: RXDS delay for rising edge
	 */
	ospi_cfg->aes_regs->aes_rxds_delay = AES_RXDS_DELAY_FALL_EDGE |
	                                     (AES_RXDS_DELAY_RISE_EDGE << AES_RXDS_DELAY_RISE_SHIFT);

	ospi_enable(ospi_cfg);
}

/**
 * @brief Configure XIP mode for PSRAM
 */
static void ospi_psram_xip_mode_init(ospi_cfg_t *ospi_cfg, uint8_t wait_cycles)
{
	ospi_regs_t *regs = ospi_cfg->regs;

	INFO("AP Memory: XIP Mode Configuration\n");
	ospi_disable(ospi_cfg);

	/* Configure CTRLR0 for XIP mode */
	uint32_t ctrlr0_val = OSPI_CTRLR0_IS_MST
	                      | (SPI_OCTAL << OSPI_CTRLR0_SPI_FRF_OFFSET)
	                      | (FEATURE_DISABLE << OSPI_CTRLR0_SCPOL_OFFSET)
	                      | (FEATURE_DISABLE << OSPI_CTRLR0_SCPH_OFFSET)
	                      | (FEATURE_DISABLE << OSPI_CTRLR0_SSTE_OFFSET)
	                      | (SPI_TMOD_RO << OSPI_CTRLR0_TMOD_OFFSET)
	                      | (OSPI_CTRLR0_DFS_32BIT << OSPI_CTRLR0_DFS_OFFSET);

	regs->ctrlr0 = ctrlr0_val;

	regs->xip_mode_bits = FEATURE_DISABLE;
	regs->xip_incr_inst = (APS512Mb_CMD_LINEAR_BURST_READ << XIP_INST_SHIFT) | APS512Mb_CMD_LINEAR_BURST_READ;
	regs->xip_wrap_inst = (APS512Mb_CMD_SYNC_READ << XIP_INST_SHIFT) | APS512Mb_CMD_SYNC_READ;
	regs->xip_write_incr_inst = (APS512Mb_CMD_LINEAR_BURST_WRITE << XIP_INST_SHIFT) | APS512Mb_CMD_LINEAR_BURST_WRITE;
	regs->xip_write_wrap_inst = (APS512Mb_CMD_SYNC_WRITE << XIP_INST_SHIFT) | APS512Mb_CMD_SYNC_WRITE;
	regs->xip_cnt_time_out = XIP_CNT_TIMEOUT_VALUE;

	/* Configure XIP control register */
	uint32_t xip_ctrl_val = (SPI_FRF_OCTAL << XIP_CTRL_FRF_POS)
	                        | (SPI_TRANS_TYPE_FRF_DUAL_OCTAL << XIP_CTRL_TRANS_TYPE_POS)
	                        | (SPI_ADDR_L_32_BIT << XIP_CTRL_ADDR_L_POS)
	                        | (SPI_INST_L_16_BIT << XIP_CTRL_INST_L_POS)
	                        | (wait_cycles << XIP_CTRL_WAIT_CYCLES_POS)
	                        | (FEATURE_ENABLE << XIP_CTRL_DFS_HC_POS)
	                        | (FEATURE_ENABLE << XIP_CTRL_DDR_EN_POS)
	                        | (FEATURE_ENABLE << XIP_CTRL_INST_DDR_EN_POS)
	                        | (FEATURE_ENABLE << XIP_CTRL_RXDS_EN_POS)
	                        | (FEATURE_ENABLE << XIP_CTRL_INST_EN_POS)
	                        | (FEATURE_ENABLE << XIP_CTRL_RXDS_SIG_EN_POS);

	regs->xip_ctrl = xip_ctrl_val;

	/* Configure XIP write control register */
	uint32_t xip_wr_ctrl_val = (SPI_FRF_OCTAL << XIP_WR_CTRL_FRF_POS)
	                           | (SPI_TRANS_TYPE_FRF_DUAL_OCTAL << XIP_WR_CTRL_TRANS_TYPE_POS)
	                           | (SPI_ADDR_L_32_BIT << XIP_WR_CTRL_ADDR_L_POS)
	                           | (SPI_INST_L_16_BIT << XIP_WR_CTRL_INST_L_POS)
	                           | (FEATURE_ENABLE << XIP_WR_CTRL_DDR_EN_POS)
	                           | (FEATURE_ENABLE << XIP_WR_CTRL_INST_DDR_EN_POS)
	                           | (FEATURE_ENABLE << XIP_WR_CTRL_RXDS_SIG_EN_POS)
	                           | (FEATURE_ENABLE << XIP_WR_CTRL_DM_EN_POS)
	                           | (wait_cycles << XIP_WR_CTRL_WAIT_CYCLES_POS)
	                           | (FEATURE_ENABLE << XIP_WR_CTRL_DFS_HC_POS);

	regs->xip_write_ctrl = xip_wr_ctrl_val;

	ospi_enable(ospi_cfg);
}

/**
 * @brief Initialize AP Memory APS512Mb PSRAM via OSPI0
 */
int ap_memory_hyperram_init(void)
{
	static ospi_cfg_t ospi_cfg_struct;
	ospi_cfg_t *ospi_cfg = &ospi_cfg_struct;
	uint8_t reg_value;

	INFO("AP Memory: APS512Mb HyperRAM Initialization\n");

	/* Configure pinmux for PSRAM OSPI signals */
	ap_memory_pinmux_config();

	/* Initialize OSPI controller */
	ospi_psram_controller_init(ospi_cfg);

	/* Set data frame size for register operations */
	ospi_psram_set_dfs(ospi_cfg, APS512Mb_OSPI_REG_DFS);

	/* Enable chip select */
	ospi_disable(ospi_cfg);
	ospi_cfg->regs->ser |= (1 << ospi_cfg->ss);
	ospi_enable(ospi_cfg);

	/* Send global reset command */
	aps512mb_global_reset(ospi_cfg);
	INFO("AP Memory: Global reset completed\n");

	/* Read and verify device ID */
	reg_value = aps512mb_read_reg(ospi_cfg, APS512Mb_MODE_REG2_ADDR,
	                                APS512Mb_INIT_REG_READ_WAIT_CYCLES);
	INFO("AP Memory: Device ID = 0x%02X (expected: 0x%02X)\n", reg_value, APS512Mb_ID);
	if (reg_value != APS512Mb_ID) {
		ERROR("AP Memory: Device ID mismatch!\n");
		return -1;
	}

	/* Configure wait cycles if needed */
	if (PSRAM_WAIT_CYCLES != APS512Mb_INIT_READ_WRITE_WAIT_CYCLES) {
		/* Configure read wait cycles (MR0) */
		reg_value = (FEATURE_DISABLE << APS512Mb_MODE_REG0_LATENCY_TYPE)
		             | (PSRAM_WAIT_CYCLES << APS512Mb_MODE_REG0_READ_LATENCY_CODE)
		             | (FEATURE_DISABLE << APS512Mb_MODE_REG0_DRIVE_STR);
		aps512mb_write_reg(ospi_cfg, APS512Mb_MODE_REG0_ADDR, reg_value);

		/* Configure write wait cycles (MR4) */
		reg_value = (FEATURE_DISABLE << APS512Mb_MODE_REG4_READ_RF_RATE)
		             | (PSRAM_WAIT_CYCLES << APS512Mb_MODE_REG4_WRITE_LATENCY_CODE)
		             | (FEATURE_DISABLE << APS512Mb_MODE_REG4_PASR);
		aps512mb_write_reg(ospi_cfg, APS512Mb_MODE_REG4_ADDR, reg_value);
	}

	/* Configure device mode (MR8) - enable dual octal mode */
	reg_value = (FEATURE_ENABLE << APS512Mb_MODE_REG8_BURST_LEN)
	             | (FEATURE_DISABLE << APS512Mb_MODE_REG8_BURST_TYPE)
	             | (FEATURE_DISABLE << APS512Mb_MODE_REG8_RBX_READ_EN)
	             | (PSRAM_DUAL_OCTAL_MODE_ENABLE << APS512Mb_MODE_REG8_TRANSFER_MODE);
	aps512mb_write_reg(ospi_cfg, APS512Mb_MODE_REG8_ADDR, reg_value);

	/* Verify MR8 configuration */
	reg_value = aps512mb_read_reg(ospi_cfg, APS512Mb_MODE_REG8_ADDR,
	                                APS512Mb_INIT_REG_READ_WAIT_CYCLES);
	INFO("AP Memory: MR8 = 0x%02X (dual-octal mode)\n", reg_value);

	/* Configure AES Address Control Shim for dual octal mode */
	if (PSRAM_DUAL_OCTAL_MODE_ENABLE) {
		uint8_t ss0_array_mode_en = (PSRAM_OSPI_SS_LINE == 0) ? FEATURE_ENABLE : FEATURE_DISABLE;
		uint8_t ss1_array_mode_en = (PSRAM_OSPI_SS_LINE == 0) ? FEATURE_DISABLE : FEATURE_ENABLE;

		uint32_t aes_addr_ctrl_val = (DUAL_OCTAL_ADDR_MASK << AES_ADDR_CTRL_ARRAY_MASK_POS)
		                             | (ss0_array_mode_en << AES_ADDR_CTRL_SS0_ARRAY_MODE_POS)
		                             | (ss1_array_mode_en << AES_ADDR_CTRL_SS1_ARRAY_MODE_POS)
		                             | (DUAL_OCTAL_ADDR_UPPER_SHIFT << AES_ADDR_CTRL_ARRAY_MODE_SHIFT_POS)
		                             | (DUAL_OCTAL_ADDR_LOWER_BITS << AES_ADDR_CTRL_ARRAY_MODE_SPLIT_POS);

		ospi_cfg->aes_regs->aes_addr_control = aes_addr_ctrl_val;
	}

	/* Set final data frame size */
	ospi_psram_set_dfs(ospi_cfg, DATA_BUS_WIDTH_BITS);

	/* Configure XIP mode */
	ospi_psram_xip_mode_init(ospi_cfg, PSRAM_WAIT_CYCLES);

	/* Enable AES XIP */
	ospi_cfg->aes_regs->aes_control |= AES_CONTROL_XIP_EN;
	INFO("AP Memory: AES XIP enabled\n");

	/* Disable regular chip select (XIP uses xip_ser instead) */
	ospi_disable(ospi_cfg);
	ospi_cfg->regs->ser &= ~(1 << ospi_cfg->ss);
	ospi_enable(ospi_cfg);

	INFO("AP Memory: APS512Mb HyperRAM initialization done\n");

	return 0;
}
