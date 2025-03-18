/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/alif/common/drivers/ospi.h>
#include <plat/alif/common/drivers/issi_hyperram.h>
#include <stdint.h>

#define IS66_WAIT_CYCLES		6
#define IS66_DUAL_DIE_PACKAGE		1
#define IS66_CA_WRITE_CONF_REG0		0x60000100
#define IS66_CA_WRITE_CONF_REG0_DIE1	0x60200100

void hyper_ram_xip_init(ospi_cfg_t *ospi)
{
	uint32_t val;

	ospi_disable(ospi);

	writel(ospi, ctrlr0, 0xF << OSPI_CTRLR0_DFS_OFFSET);

	val = (IS66_WAIT_CYCLES << OSPI_XIP_CTRL_WAIT_CYCLES_OFFSET)
		| (0x1 << OSPI_XIP_CTRL_XIP_HYPERBUS_EN_OFFSET)
		| (0x1 << OSPI_XIP_CTRL_RXDS_SIG_EN_OFFSET)
		| (0x1 << OSPI_XIP_CTRL_DFS_HC_OFFSET);

	writel(ospi, xip_ctrl, val);

	val = (IS66_WAIT_CYCLES << OSPI_XIP_WR_CTRL_WAIT_CYCLES)
		| (0x1 << OSPI_XIP_WR_CTRL_WR_DM_EN)
		| (0x1 << OSPI_XIP_WR_CTRL_WR_HYPERBUS_EN)
		| (0x1 << OSPI_XIP_WR_CTRL_WR_RXDS_SIG_EN)
		| (0x1 << OSPI_XIP_WR_CTRL_DFS_HC);

	writel(ospi, xip_write_ctrl, val);

	ospi_enable(ospi);

	ospi->aes_regs->aes_control |= AES_CONTROL_XIP_EN;
}

//write 16bit data into the configuration register 0 of the ISSI device
void hyper_ram_write_conf_reg0(ospi_cfg_t *ospi, uint16_t data)
{
	uint32_t val;

	ospi_disable(ospi);

	writel(ospi, ser, 0);

	val = (1 << OSPI_CTRLR0_SPI_HE_OFFSET)
		|(SPI_OCTAL << OSPI_CTRLR0_SPI_FRF_OFFSET)
		|(SPI_TMOD_TO << OSPI_CTRLR0_TMOD_OFFSET)
		|(0xf << OSPI_CTRLR0_DFS_OFFSET);

	writel(ospi, ctrlr0, val);

	val = OSPI_TRANS_TYPE_FRF_DEFINED
		|(0xc << (OSPI_SPI_CTRLR0_ADDR_L_OFFSET))
		|(0 << OSPI_SPI_CTRLR0_INST_L_OFFSET)
		|(0 << OSPI_SPI_CTRLR0_WAIT_CYCLES_OFFSET)
		|(1 << OSPI_SPI_CTRLR0_SPI_DDR_EN_OFFSET)
		|(0 << OSPI_SPI_CTRLR0_SPI_RXDS_EN_OFFSET)
		|(0 << OSPI_SPI_CTRLR0_SPI_DM_EN_OFFSET)
		|(0 << OSPI_SPI_CTRLR0_SPI_RXDS_SIG_EN_OFFSET);

	writel(ospi, spi_ctrlr0, val);

	ospi_enable(ospi);

	/*
	 * Form the command to write into configuration register 0.
	 * Refer to the ISSI data sheet for the below CA bytes.
	 */
	writel(ospi, datareg, IS66_CA_WRITE_CONF_REG0);
	writel(ospi, datareg, 0x0);
	writel(ospi, datareg, data);
	writel(ospi, ser, 1 << ospi->ss);

	while ((readl(ospi, sr) & (SR_TF_EMPTY)) != SR_TF_EMPTY);

#if IS66_DUAL_DIE_PACKAGE
	/*
	 * write into the configuration register of the second die.
	 */
	writel(ospi, datareg, IS66_CA_WRITE_CONF_REG0_DIE1);
	writel(ospi, datareg, 0x0);
	writel(ospi, datareg, data);

	while ((readl(ospi, sr) & (SR_TF_EMPTY)) != SR_TF_EMPTY);
#endif
}
