/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/alif/common/drivers/ospi.h>
#include <plat/alif/common/drivers/issi_hyperram.h>
#include <stdint.h>
#include <assert.h>
#include <common/debug.h>
#include <lib/libc/errno.h>

/* HyperRAM configuration parameters */
#define IS66_WAIT_CYCLES_DEFAULT		6
#define IS66_DUAL_DIE_PACKAGE			1
#define IS66_CA_WRITE_CONF_REG0			0x60000100
#define IS66_CA_WRITE_CONF_REG0_DIE1		0x60200100

/* Timeout and retry settings */
#define MAX_RETRY_COUNT				1000000
#define RETRY_DELAY_US				10

/* Register field macros */
#define XIP_CTRL_DEFAULT(val)			((val) | BIT(OSPI_XIP_CTRL_XIP_HYPERBUS_EN_OFFSET))
#define XIP_WRITE_CTRL_DEFAULT(val)		((val) | BIT(OSPI_XIP_WR_CTRL_WR_HYPERBUS_EN))

/**
 * @brief Wait for OSPI transmission to complete
 * 
 * @param ospi Pointer to OSPI configuration
 * @return int 0 on success, -ETIMEDOUT on failure
 */
static int hyper_ram_wait_transfer_complete(ospi_cfg_t *ospi)
{
    uint32_t retry;

    for (retry = 0; retry < MAX_RETRY_COUNT; retry++) {
        if ((readl(ospi, sr) & SR_TF_EMPTY)) {
            return 0;
        }
    }

    ERROR("ISSI HyperRAM: Transfer timeout\n");
    return -ETIMEDOUT;
}

/**
 * @brief Initializes the HyperRAM in eXecute In Place (XIP) mode via OSPI.
 *
 * @param ospi Pointer to the OSPI configuration structure.
 * @return int 0 on success, negative error code on failure
 */
int hyper_ram_xip_init(ospi_cfg_t *ospi)
{
    uint32_t val;
    int ret = 0;

    if (ospi == NULL || ospi->regs == NULL) {
        ERROR("ISSI HyperRAM: Invalid OSPI configuration\n");
        return -EINVAL;
    }

    ospi_disable(ospi);

    /* Set data frame size to 8 bits (DFS = 0xF) */
    writel(ospi, ctrlr0, (0xF << OSPI_CTRLR0_DFS_OFFSET));

    /* Configure XIP control register */
    val = (IS66_WAIT_CYCLES_DEFAULT << OSPI_XIP_CTRL_WAIT_CYCLES_OFFSET) |
          BIT(OSPI_XIP_CTRL_XIP_HYPERBUS_EN_OFFSET) |
          BIT(OSPI_XIP_CTRL_RXDS_SIG_EN_OFFSET) |
          BIT(OSPI_XIP_CTRL_DFS_HC_OFFSET);

    writel(ospi, xip_ctrl, val);

    /* Configure XIP write control register */
    val = (IS66_WAIT_CYCLES_DEFAULT << OSPI_XIP_WR_CTRL_WAIT_CYCLES) |
          BIT(OSPI_XIP_WR_CTRL_WR_DM_EN) |
          BIT(OSPI_XIP_WR_CTRL_WR_HYPERBUS_EN) |
          BIT(OSPI_XIP_WR_CTRL_WR_RXDS_SIG_EN) |
          BIT(OSPI_XIP_WR_CTRL_DFS_HC);

    writel(ospi, xip_write_ctrl, val);

    /* Enable the OSPI controller */
    ospi_enable(ospi);

    /* Enable XIP mode in AES controller if supported */
    if (ospi->aes_regs != NULL) {
        ospi->aes_regs->aes_control |= AES_CONTROL_XIP_EN;
	ospi->aes_regs->aes_rxds_delay = 6;
        INFO("ISSI HyperRAM: XIP mode enabled with AES support\n");
    } else {
        INFO("ISSI HyperRAM: XIP mode enabled\n");
    }

    return ret;
}

/**
 * @brief Configure OSPI controller for HyperRAM register access
 * 
 * @param ospi Pointer to OSPI configuration
 */
static void hyper_ram_config_ospi(ospi_cfg_t *ospi)
{
    uint32_t val;

    /* Configure OSPI for OCTAL mode */
    val = BIT(OSPI_CTRLR0_SPI_HE_OFFSET) |
          (SPI_OCTAL << OSPI_CTRLR0_SPI_FRF_OFFSET) |
          (SPI_TMOD_TO << OSPI_CTRLR0_TMOD_OFFSET) |
          (0xf << OSPI_CTRLR0_DFS_OFFSET);

    writel(ospi, ctrlr0, val);

    /* Configure SPI controller for HyperRAM access */
    val = OSPI_TRANS_TYPE_FRF_DEFINED |
          (0xc << OSPI_SPI_CTRLR0_ADDR_L_OFFSET) |
          (0 << OSPI_SPI_CTRLR0_INST_L_OFFSET) |
          (0 << OSPI_SPI_CTRLR0_WAIT_CYCLES_OFFSET) |
          BIT(OSPI_SPI_CTRLR0_SPI_DDR_EN_OFFSET) |
          (0 << OSPI_SPI_CTRLR0_SPI_RXDS_EN_OFFSET) |
          (0 << OSPI_SPI_CTRLR0_SPI_DM_EN_OFFSET) |
          (0 << OSPI_SPI_CTRLR0_SPI_RXDS_SIG_EN_OFFSET);

    writel(ospi, spi_ctrlr0, val);
}

/**
 * @brief Write configuration register to a single HyperRAM die
 * 
 * @param ospi Pointer to OSPI configuration
 * @param cmd_addr Command address for the die
 * @param data 16-bit value to write
 * @return int 0 on success, negative error code on failure
 */
static int hyper_ram_write_conf_reg_die(ospi_cfg_t *ospi, uint32_t cmd_addr, uint16_t data)
{
    int ret;

    /* Prepare command sequence for writing to Config Reg 0 */
    writel(ospi, datareg, cmd_addr);  /* Command Address */
    writel(ospi, datareg, 0x0);       /* Address */
    writel(ospi, datareg, data);      /* Data (16-bit) */
    writel(ospi, ser, BIT(ospi->ss)); /* Select Slave */

    /* Wait until transmission completes */
    ret = hyper_ram_wait_transfer_complete(ospi);
    if (ret != 0) {
        ERROR("ISSI HyperRAM: Failed to write config register 0x%x\n", cmd_addr);
    }

    return ret;
}

/**
 * @brief Writes a 16-bit value into configuration register 0 of the ISSI HyperRAM.
 *
 * @param ospi Pointer to the OSPI configuration structure.
 * @param data 16-bit value to be written to configuration register 0.
 * @return int 0 on success, negative error code on failure
 */
int hyper_ram_write_conf_reg0(ospi_cfg_t *ospi, uint16_t data)
{
    int ret = 0;

    if (ospi == NULL || ospi->regs == NULL) {
        ERROR("ISSI HyperRAM: Invalid OSPI configuration\n");
        return -EINVAL;
    }

    ospi_disable(ospi);

    /* Reset command FIFO */
    writel(ospi, ser, 0);

    /* Configure OSPI for HyperRAM access */
    hyper_ram_config_ospi(ospi);

    ospi_enable(ospi);

    /* Write to primary die */
    ret = hyper_ram_write_conf_reg_die(ospi, IS66_CA_WRITE_CONF_REG0, data);
    if (ret != 0) {
        goto exit;
    }

#if IS66_DUAL_DIE_PACKAGE
    /* Write to second die in dual-die package */
    ret = hyper_ram_write_conf_reg_die(ospi, IS66_CA_WRITE_CONF_REG0_DIE1, data);
    if (ret != 0) {
        goto exit;
    }
#endif /* IS66_DUAL_DIE_PACKAGE */

exit:
    return ret;
}
