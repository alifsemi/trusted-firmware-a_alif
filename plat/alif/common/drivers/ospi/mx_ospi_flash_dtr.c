/* Copyright (C) 2026 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/**************************************************************************//**
 * @file     mx_ospi_flash_dtr.c
 * @version  V1.0.0
 * @brief    Macronix MX66UW1G45G OSPI NOR Flash driver for OPI DTR mode
 *           with 16-bit DFS (Data Frame Size)
 * @Note     This driver initializes flash to DOPI mode using SPI commands,
 *           then performs device detection and XIP setup in OPI DTR mode.
 *           Based on MX66UW1G45G datasheet.
 ******************************************************************************/

#include <common/debug.h>
#include <lib/mmio.h>
#include "dwc_spi.h"
#include "ospi_drv.h"
#include "ospi_xip_user.h"

/* Multiplier to Calculate the delay */
#define MULT 100

/*===========================================================================
 * Macronix MX66UW1G45G Command Definitions (from datasheet)
 *===========================================================================*/

/* SPI Mode Commands (1-1-1) - used to initialize flash to DOPI mode */
#define MX_CMD_RDID_SPI             0x9F    /* Read ID */
#define MX_CMD_WREN_SPI             0x06    /* Write Enable */
#define MX_CMD_RDSR_SPI             0x05    /* Read Status Register */
#define MX_CMD_WRCR2_SPI            0x72    /* Write Configuration Register 2 */
#define MX_CMD_SBL_SPI              0xC0    /* Set Burst Length */
#define MX_CMD_RBL_SPI              0xC1    /* Read Burst Length */

/* OPI DTR Mode Commands (8D-8D-8D) - 16-bit: CMD + inverted CMD */
#define MX_CMD_RDID_OPI             0x9F60  /* Read ID: 9Fh/60h */
#define MX_CMD_WREN_OPI             0x06F9  /* Write Enable: 06h/F9h */
#define MX_CMD_RDSR_OPI             0x05FA  /* Read Status Register: 05h/FAh */
#define MX_CMD_RDCR2_OPI            0x718E  /* Read CR2: 71h/8Eh */
#define MX_CMD_WRCR2_OPI            0x728D  /* Write CR2: 72h/8Dh */
#define MX_CMD_8DTRD                0xEE11  /* 8DTRD Read: EEh/11h */
#define MX_CMD_8READ                0xEE13  /* 8READ Read: EEh/13h */
#define MX_CMD_SBL_OPI              0xC03F  /* Set Burst Length: C0h/3Fh */
#define MX_CMD_RBL_OPI              0xC13E  /* Read Burst Length: C1h/3Eh */

/* Configuration Register 2 Addresses */
#define MX_CR2_ADDR_IO_MODE         0x00000000  /* I/O mode configuration */
#define MX_CR2_ADDR_DQS             0x00000200  /* DQS configuration */
#define MX_CR2_ADDR_DUMMY           0x00000300  /* Dummy cycle configuration */

/* CR2 Values */
#define MX_CR2_DOPI_EN              0x02    /* DOPI (OPI DTR) mode enabled */

/* Status Register bits */
#define MX_SR_WIP                   (1U << 0)   /* Write In Progress */
#define MX_SR_WEL                   (1U << 1)   /* Write Enable Latch */

/* Burst Length Configuration */
#define MX_BURST_LINEAR             0x10    /* Linear burst (no wrap) */
#define MX_BURST_WRAP16             0x01    /* 16-byte wrap */
#define MX_BURST_WRAP32             0x02    /* 32-byte wrap */
#define MX_BURST_WRAP64             0x03    /* 64-byte wrap */

/* Device ID */
#define MX_MANUFACTURER_ID          0xC2    /* Macronix manufacturer ID */

/* Timing */
#define MX_DEFAULT_WAIT_CYCLES      10      /* 10 dummy cycles for OPI DTR @ 100MHz */
#define MX_RDID_DUMMY_CYCLES        4      /* 4 dummy cycles for RDID in DOPI mode */

/* GPIO for OSPI Reset */
#define LPGPIO_BASE                 0x42002000UL
#define GPIO_INTMASK_OFFSET         0x34
#define GPIO_SWPORTA_DR_OFFSET      0x0
#define GPIO_SWPORTA_DDR_OFFSET     0x4
#define OSPI_RESET_PIN              7

/* Pad Control */
#define PAD_CTRL_DATA               (PAD_CTRL_12MA | PAD_CTRL_SR | PAD_CTRL_REN)
#define PAD_CTRL_CLK                (PAD_CTRL_12MA | PAD_CTRL_SR)

/*===========================================================================
 * Private Data
 *===========================================================================*/

static ospi_flash_cfg_t mx_ospi_flash_config;

extern int service_ospi_write_aes_key(void);

/*===========================================================================
 * Utility Functions
 *===========================================================================*/

/**
 * @brief Microsecond delay
 */
static void mx_delay_us(uint32_t us)
{
    volatile uint32_t count = us * MULT;
    while (count--) {
        __asm__ volatile("nop");
    }
}

/**
 * @brief Configure GPIO pins for OSPI interface
 */
static void mx_setup_pinmux(void)
{
    uint32_t value;

    /* Configure pad control registers and Mux value */
    *PADCTRL_REG(9, 5, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(9, 6, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(9, 7, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(10, 0, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(10, 1, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(10, 2, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(10, 3, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(10, 4, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(10, 7, PAD_CTRL_DATA, 7);
    *PADCTRL_REG(5, 5, PAD_CTRL_CLK, 1);
    *PADCTRL_REG(5, 7, PAD_CTRL_12MA, 1);
    *PADCTRL_REG(9, 0, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(8, 0, PAD_CTRL_12MA, 1);

    /* Initialize GPIO for OSPI reset */
    value = mmio_read_32((LPGPIO_BASE + GPIO_INTMASK_OFFSET));
    mmio_write_32((LPGPIO_BASE + GPIO_INTMASK_OFFSET),
                  (value | (1 << OSPI_RESET_PIN)));

    /* Set direction output */
    value = mmio_read_32((LPGPIO_BASE + GPIO_SWPORTA_DDR_OFFSET));
    mmio_write_32((LPGPIO_BASE + GPIO_SWPORTA_DDR_OFFSET),
                  (value | (1 << OSPI_RESET_PIN)));

    /* Reset pulse: low then high */
    value = mmio_read_32((LPGPIO_BASE + GPIO_SWPORTA_DR_OFFSET));
    mmio_write_32((LPGPIO_BASE + GPIO_SWPORTA_DR_OFFSET),
                  (value & ~(1 << OSPI_RESET_PIN)));

    value = mmio_read_32((LPGPIO_BASE + GPIO_SWPORTA_DR_OFFSET));
    mmio_write_32((LPGPIO_BASE + GPIO_SWPORTA_DR_OFFSET),
                  (value | (1 << OSPI_RESET_PIN)));
}

/*===========================================================================
 * SPI Mode Functions - Used to initialize flash to DOPI mode
 *===========================================================================*/

/**
 * @brief Send Write Enable command in SPI mode
 */
static void mx_write_enable_spi(ospi_flash_cfg_t *ospi_cfg)
{
    ssi_regs_t *regs = ospi_cfg->regs;
    uint32_t val;

    regs->ser = 0;
    regs->ssienr = 0;

    /* Configure for Standard SPI TX mode, 8-bit DFS */
    val = CTRLR0_IS_MST
        | (SINGLE << CTRLR0_SPI_FRF_OFFSET)
        | (TMOD_TO << CTRLR0_TMOD_OFFSET)
        | (CTRLR0_DFS_8bit << CTRLR0_DFS_OFFSET);
    regs->ctrlr0 = val;

    val = TRANS_TYPE_STANDARD
        | (CTRLR0_INST_L_8bit << CTRLR0_INST_L_OFFSET)
        | (ADDR_LENGTH_0_BITS << CTRLR0_ADDR_L_OFFSET);
    regs->spi_ctrlr0 = val;

    regs->ssienr = 1;
    regs->data_reg = MX_CMD_WREN_SPI;
    regs->ser = ospi_cfg->ser;

    while ((regs->sr & SR_TF_EMPTY) == 0) { }
    while ((regs->sr & SR_BUSY) != 0) { }

    regs->ser = 0;
    regs->ssienr = 0;
}

/**
 * @brief Read Status Register in SPI mode
 */
static uint8_t mx_read_status_spi(ospi_flash_cfg_t *ospi_cfg)
{
    ssi_regs_t *regs = ospi_cfg->regs;
    uint32_t val;
    uint8_t status;

    regs->ser = 0;
    regs->ssienr = 0;

    val = CTRLR0_IS_MST
        | (SINGLE << CTRLR0_SPI_FRF_OFFSET)
        | (TMOD_RO << CTRLR0_TMOD_OFFSET)
        | (CTRLR0_DFS_8bit << CTRLR0_DFS_OFFSET);
    regs->ctrlr0 = val;
    regs->ctrlr1 = 0;

    val = TRANS_TYPE_STANDARD
        | (CTRLR0_INST_L_8bit << CTRLR0_INST_L_OFFSET)
        | (ADDR_LENGTH_0_BITS << CTRLR0_ADDR_L_OFFSET);
    regs->spi_ctrlr0 = val;

    regs->ssienr = 1;
    regs->data_reg = MX_CMD_RDSR_SPI;
    regs->ser = ospi_cfg->ser;

    while ((regs->sr & SR_RF_NOT_EMPT) == 0) { }
    status = (uint8_t)regs->data_reg;

    while ((regs->sr & SR_BUSY) != 0) { }
    regs->ser = 0;
    regs->ssienr = 0;

    return status;
}

/**
 * @brief Read device ID in SPI mode
 */
static uint8_t mx_read_id_spi(ospi_flash_cfg_t *ospi_cfg)
{
    ssi_regs_t *regs = ospi_cfg->regs;
    uint32_t val;
    uint8_t id;

    regs->ser = 0;
    regs->ssienr = 0;

    val = CTRLR0_IS_MST
        | (SINGLE << CTRLR0_SPI_FRF_OFFSET)
        | (TMOD_RO << CTRLR0_TMOD_OFFSET)
        | (CTRLR0_DFS_8bit << CTRLR0_DFS_OFFSET);
    regs->ctrlr0 = val;
    regs->ctrlr1 = 2;  /* Read 3 bytes */

    val = TRANS_TYPE_STANDARD
        | (CTRLR0_INST_L_8bit << CTRLR0_INST_L_OFFSET)
        | (ADDR_LENGTH_0_BITS << CTRLR0_ADDR_L_OFFSET);
    regs->spi_ctrlr0 = val;

    regs->ssienr = 1;
    regs->data_reg = MX_CMD_RDID_SPI;
    regs->ser = ospi_cfg->ser;

    while ((regs->sr & SR_RF_NOT_EMPT) == 0) { }
    id = (uint8_t)regs->data_reg;

    /* Drain remaining bytes */
    while ((regs->sr & SR_RF_NOT_EMPT) != 0) {
        (void)regs->data_reg;
    }

    while ((regs->sr & SR_BUSY) != 0) { }
    regs->ser = 0;
    regs->ssienr = 0;

    INFO("MX SPI: Device ID = 0x%02X\n", id);
    return id;
}

/**
 * @brief Write to Configuration Register 2 in SPI mode
 */
static void mx_write_cr2_spi(ospi_flash_cfg_t *ospi_cfg, uint32_t addr, uint8_t value)
{
    ssi_regs_t *regs = ospi_cfg->regs;
    uint32_t val;
    uint8_t status;
    uint32_t timeout;


    /* Send Write Enable first */
    mx_write_enable_spi(ospi_cfg);
    mx_delay_us(10);

    /* Verify WEL is set */
    status = mx_read_status_spi(ospi_cfg);
    if ((status & MX_SR_WEL) == 0) {
        ERROR("MX SPI: WRCR2 - WEL not set (SR=0x%02X)\n", status);
        return;
    }

    regs->ser = 0;
    regs->ssienr = 0;

    val = CTRLR0_IS_MST
        | (SINGLE << CTRLR0_SPI_FRF_OFFSET)
        | (TMOD_TO << CTRLR0_TMOD_OFFSET)
        | (CTRLR0_DFS_8bit << CTRLR0_DFS_OFFSET);
    regs->ctrlr0 = val;

    val = TRANS_TYPE_STANDARD
        | (CTRLR0_INST_L_8bit << CTRLR0_INST_L_OFFSET)
        | (ADDR_LENGTH_0_BITS << CTRLR0_ADDR_L_OFFSET);
    regs->spi_ctrlr0 = val;

    regs->ssienr = 1;

    /* Send WRCR2 + 4-byte address + data */
    regs->data_reg = MX_CMD_WRCR2_SPI;
    regs->data_reg = (addr >> 24) & 0xFF;
    regs->data_reg = (addr >> 16) & 0xFF;
    regs->data_reg = (addr >> 8) & 0xFF;
    regs->data_reg = addr & 0xFF;
    regs->data_reg = value;

    regs->ser = ospi_cfg->ser;

    while ((regs->sr & SR_TF_EMPTY) == 0) { }
    while ((regs->sr & SR_BUSY) != 0) { }

    regs->ser = 0;
    regs->ssienr = 0;

    /* Wait for write complete (skip if switching to DOPI - can't read status) */
    if (!(addr == MX_CR2_ADDR_IO_MODE && value == MX_CR2_DOPI_EN)) {
        timeout = 10000U;
        do {
            mx_delay_us(10);
            status = mx_read_status_spi(ospi_cfg);
            timeout--;
        } while ((status & MX_SR_WIP) && (timeout > 0U));

        if (timeout == 0U) {
            ERROR("MX SPI: WRCR2 timeout waiting for WIP\n");
        }

    }

}

/**
 * @brief Set Burst Length in SPI mode with read-back verification
 * Command: SBL (C0h) + 1 byte data
 */
static void mx_set_burst_length_spi(ospi_flash_cfg_t *ospi_cfg, uint8_t burst_config)
{
    ssi_regs_t *regs = ospi_cfg->regs;
    uint32_t val;

    regs->ser = 0;
    regs->ssienr = 0;

    val = CTRLR0_IS_MST
        | (SINGLE << CTRLR0_SPI_FRF_OFFSET)
        | (TMOD_TO << CTRLR0_TMOD_OFFSET)
        | (CTRLR0_DFS_8bit << CTRLR0_DFS_OFFSET);
    regs->ctrlr0 = val;

    val = TRANS_TYPE_STANDARD
        | (CTRLR0_INST_L_8bit << CTRLR0_INST_L_OFFSET)
        | (ADDR_LENGTH_0_BITS << CTRLR0_ADDR_L_OFFSET);
    regs->spi_ctrlr0 = val;

    regs->ssienr = 1;

    /* Send SBL command (0xC0) + burst config byte */
    regs->data_reg = MX_CMD_SBL_SPI;
    regs->data_reg = burst_config;

    regs->ser = ospi_cfg->ser;

    while ((regs->sr & SR_TF_EMPTY) == 0) { }
    while ((regs->sr & SR_BUSY) != 0) { }

    regs->ser = 0;
    regs->ssienr = 0;

}

/**
 * @brief Initialize flash to DOPI (OPI DTR) mode using SPI commands
 */
static int mx_init_dopi_mode(ospi_flash_cfg_t *ospi_cfg)
{
    uint8_t id;

    /* Step 1: Read device ID in SPI mode to verify flash is present */
    id = mx_read_id_spi(ospi_cfg);
    if (id != MX_MANUFACTURER_ID) {
        ERROR("MX: Flash not detected in SPI mode (ID=0x%02X, expected 0x%02X)\n",
              id, MX_MANUFACTURER_ID);
        return -1;
    }

    /* Step 2: Configure dummy cycles in CR2 @ 0x00000300
     * DC=0x05 = 10 dummy cycles for OPI DTR @ 100MHz
     */
    mx_write_cr2_spi(ospi_cfg, MX_CR2_ADDR_DUMMY, 0x05);
    mx_delay_us(100);

    /* Step 3: Configure 64-byte burst wrap mode in SPI mode
     * 0x03 = 64-byte wrap boundary
     */
    mx_set_burst_length_spi(ospi_cfg, 0x03);
    mx_delay_us(100);

    /* Step 4: Enable DOPI (OPI DTR) mode in CR2 @ 0x00000000 */
    mx_write_cr2_spi(ospi_cfg, MX_CR2_ADDR_IO_MODE, MX_CR2_DOPI_EN);
    mx_delay_us(1000);  /* Wait for mode switch */

    return 0;
}

/*===========================================================================
 * OPI DTR Mode Functions (8D-8D-8D) - Used after flash is in DOPI mode
 *===========================================================================*/

/**
 * @brief Read device ID in OPI DTR mode with 16-bit DFS
 * Command: RDID (9Fh/60h), 4-byte address (0x00000000), 4 dummy cycles
 * 
 * In DOPI mode, RDID returns: Manufacturer ID (0xC2), Memory Type (0x80), Density (0x3B)
 * Due to DQS pre-cycle, data may be shifted - read multiple frames to find correct byte.
 * 
 * @return Manufacturer ID
 */
static uint8_t mx_read_id_opi_dtr(ospi_flash_cfg_t *ospi_cfg)
{
    ssi_regs_t *regs = ospi_cfg->regs;
    uint32_t val;
    uint16_t data[3];  /* Read multiple frames to debug */
    uint8_t id;
    uint32_t timeout;
    int i, frame_count = 3;

    /* Step 1: Disable SPI and slave select */
    regs->ser = 0;
    regs->ssienr = 0;

    /* Step 2: Configure CTRLR0 for Octal EEPROM Read mode with 16-bit DFS */
    val = CTRLR0_IS_MST
        | (OCTAL << CTRLR0_SPI_FRF_OFFSET)
        | (TMOD_RO << CTRLR0_TMOD_OFFSET)
        | (CTRLR0_DFS_16bit << CTRLR0_DFS_OFFSET);
    regs->ctrlr0 = val;

    /* Step 3: Configure CTRLR1 - read 4 x 16-bit frames = 8 bytes */
    regs->ctrlr1 = frame_count - 1;

    /* Step 4: Configure SPI_CTRLR0 for OPI DTR mode */
    val = TRANS_TYPE_FRF_DEFINED
        | (1U << CTRLR0_SPI_DDR_EN_OFFSET)
        | (1U << CTRLR0_INST_DDR_EN_OFFSET)
        | (CTRLR0_INST_L_16bit << CTRLR0_INST_L_OFFSET)
        | (ADDR_LENGTH_32_BITS << CTRLR0_ADDR_L_OFFSET)
        | (MX_RDID_DUMMY_CYCLES << CTRLR0_WAIT_CYCLES_OFFSET)
        | (1U << CTRLR0_SPI_RXDS_EN_OFFSET);
    regs->spi_ctrlr0 = val;

    /* Step 5: Configure timing */
    regs->rx_sample_dly = 0;
    ospi_cfg->aes_regs->aes_rxds_delay = 11;

    /* Step 6: Enable SPI */
    regs->ssienr = 1;

    /* Step 7: Push RDID command and address */
    regs->data_reg = MX_CMD_RDID_OPI;
    regs->data_reg = 0x00000000;

    /* Step 8: Start transfer */
    regs->ser = ospi_cfg->ser;

    /* Step 9: Read all frames */
    for (i = 0; i < frame_count; i++) {
        timeout = 100000U;
        while ((regs->sr & SR_RF_NOT_EMPT) == 0) {
            timeout--;
            if (timeout == 0) {
                ERROR("MX RDID: TIMEOUT on frame %d\n", i);
                regs->ser = 0;
                regs->ssienr = 0;
                return 0xFF;
            }
        }
        data[i] = (uint16_t)regs->data_reg;
    }

    /* Wait for transfer complete */
    timeout = 100000U;
    while ((regs->sr & SR_BUSY) != 0) {
        timeout--;
        if (timeout == 0) break;
    }

    regs->ser = 0;
    regs->ssienr = 0;

    /* In DOPI mode with DQS, data bytes are:
     * Frame 0: Manufacturer ID (high) + Memory Type (low) - but may be shifted
     * Look for 0xC2 in all bytes */
    uint8_t bytes[6];
    bytes[0] = (uint8_t)(data[0] >> 8);
    bytes[1] = (uint8_t)(data[0] & 0xFF);
    bytes[2] = (uint8_t)(data[1] >> 8);
    bytes[3] = (uint8_t)(data[1] & 0xFF);
    bytes[4] = (uint8_t)(data[2] >> 8);
    bytes[5] = (uint8_t)(data[2] & 0xFF);

    INFO("MX RDID: Bytes: %02X %02X %02X %02X %02X %02X\n",
         bytes[0], bytes[1], bytes[2], bytes[3],
         bytes[4], bytes[5]);

    /* Find manufacturer ID (0xC2) in the bytes */
    id = 0xFF;
    for (i = 0; i < 6; i++) {
        if (bytes[i] == MX_MANUFACTURER_ID) {
            id = bytes[i];
            break;
        }
    }

    /* If not found, use first non-zero byte as ID for debugging */
    if (id == 0xFF) {
        for (i = 0; i < 6; i++) {
            if (bytes[i] != 0x00) {
                id = bytes[i];
                INFO("MX RDID: Using byte %d (0x%02X) as ID\n", i, id);
                break;
            }
        }
    }

    ospi_cfg->device_id = id;
    return id;
}

/*===========================================================================
 * XIP Mode Functions
 *===========================================================================*/

/**
 * @brief Initialize XIP mode for OPI DTR with 16-bit DFS
 * Uses 8DTRD command (EEh/11h)
 * Note: Using 16-bit DFS for byte-level access in DOPI mode
 */
static int mx_flash_xip_init(ospi_flash_cfg_t *ospi_cfg)
{
    ssi_regs_t *regs = ospi_cfg->regs;
    uint32_t val;

    /* Disable SPI */
    regs->ser = 0;
    regs->ssienr = 0;

    /* Configure CTRLR0 for OPI DDR mode with 16-bit DFS */
    val = CTRLR0_IS_MST
        | (OCTAL << CTRLR0_SPI_FRF_OFFSET)
        | (0 << CTRLR0_SCPOL_OFFSET)
        | (0 << CTRLR0_SCPH_OFFSET)
        | (0 << CTRLR0_SSTE_OFFSET)
        | (TMOD_RO << CTRLR0_TMOD_OFFSET)
        | (CTRLR0_DFS_16bit << CTRLR0_DFS_OFFSET);  /* 16-bit DFS */
    regs->ctrlr0 = val;

    /* XIP_CTRL for OPI DDR mode with 64-byte burst
     * XIP_MBL: 0=2, 1=4, 2=8, 3=16, 4=32 data frames
     * For 64-byte burst with 16-bit DFS, use MBL=4 (32 frames x 2 bytes = 64 bytes per burst) */
    val = (OCTAL << XIP_CTRL_FRF_OFFSET)
        | (0x2 << XIP_CTRL_TRANS_TYPE_OFFSET)       /* TT=2: Inst + Addr in FRF format */
        | (ADDR_L32bit << XIP_CTRL_ADDR_L_OFFSET)   /* 32-bit address */
        | (CTRLR0_INST_L_16bit << XIP_CTRL_INST_L_OFFSET)  /* 16-bit instruction */
        | (0x0 << XIP_CTRL_MD_BITS_EN_OFFSET)
        | ((MX_DEFAULT_WAIT_CYCLES) << XIP_CTRL_WAIT_CYCLES_OFFSET)
        | (0x1 << XIP_CTRL_DFC_HC_OFFSET)
        | (0x1 << XIP_CTRL_DDR_EN_OFFSET)           /* DDR enabled */
        | (0x1 << XIP_CTRL_INST_DDR_EN_OFFSET)      /* Instruction DDR enabled */
        | (0x1 << XIP_CTRL_RXDS_EN_OFFSET)          /* RXDS enabled */
        | (0x1 << XIP_CTRL_INST_EN_OFFSET)          /* Instruction enable */
        | (0x0 << XIP_CTRL_CONT_XFER_EN_OFFSET)
        | (0x0 << XIP_CTRL_HYPERBUS_EN_OFFSET)
        | (0x1 << XIP_CTRL_RXDS_SIG_EN)             /* RXDS signal enable */
        | (0x4 << XIP_CTRL_XIP_MBL_OFFSET)          /* MBL=4: 32 data frames for 64-byte burst */
        | (0x0 << XIP_PREFETCH_EN_OFFSET)
        | (0x1 << XIP_CTRL_RXDS_VL_EN_OFFSET);
    regs->xip_ctrl = val;

    /* Timing registers */
    regs->rx_sample_dly = 0;
    ospi_cfg->aes_regs->aes_rxds_delay = 11;

    /* XIP instruction registers */
    regs->xip_mode_bits = 0x0;
    regs->xip_incr_inst = MX_CMD_8DTRD;
    regs->xip_wrap_inst = MX_CMD_8DTRD;

    /* Enable SPI */
    regs->ssienr = 1;

    /* Enable XIP in AES control register */
    ospi_cfg->aes_regs->aes_control = (AES_CONTROL_XIP_EN);
#if AES_EN
    ospi_cfg->aes_regs->aes_control |= (AES_CONTROL_LD_KEY | AES_CONTROL_DECRYPT_EN);
#endif
    INFO("MX: XIP enabled\n");

    return 0;
}

/*===========================================================================
 * Flash Probe and Initialization
 *===========================================================================*/

/**
 * @brief Probe for Macronix flash and configure for OPI DTR mode
 * 1. Detect flash in SPI mode
 * 2. Switch flash to DOPI mode using SPI commands
 * 3. Verify flash responds in OPI DTR mode
 */
static int mx_flash_probe(ospi_flash_cfg_t *ospi_cfg)
{
    uint8_t id;

    INFO("MX: Probing for Macronix MX66UW1G45G flash...\n");

    /* Step 1: Initialize flash to DOPI mode using SPI commands */
    if (mx_init_dopi_mode(ospi_cfg) != 0) {
        ERROR("MX: Failed to initialize flash to DOPI mode\n");
        return -1;
    }

    /* Step 2: Set controller to DDR mode for OPI DTR operation */
    ospi_cfg->ddr_en = 1U;
    ospi_cfg->wait_cycles = MX_DEFAULT_WAIT_CYCLES;

    /* Step 3: Verify flash responds in OPI DTR mode by reading device ID */
    id = mx_read_id_opi_dtr(ospi_cfg);

    if (id != MX_MANUFACTURER_ID) {
        ERROR("MX: Flash not detected in OPI DTR mode (ID=0x%02X, expected 0x%02X)\n",
              id, MX_MANUFACTURER_ID);
        ERROR("MX: Ensure flash is configured for DOPI mode before calling this driver\n");
        return -1;
    }

    INFO("MX: Macronix flash detected in OPI DTR mode (ID=0x%02X)\n", id);

    INFO("MX: Flash probe successful\n");
    return 0;
}

/**
 * @brief Set up flash for XIP mode
 * 1. Initialize controller in SPI mode
 * 2. Switch flash to DOPI mode
 * 3. Verify flash in OPI DTR mode
 * 4. Configure XIP
 */
static int mx_setup_flash_xip(void)
{
    ospi_flash_cfg_t *ospi_cfg = &mx_ospi_flash_config;

    /* Configure OSPI controller */
    ospi_cfg->regs = (ssi_regs_t *)OSPI1_BASE;
    ospi_cfg->aes_regs = (aes_regs_t *)AES1_BASE;
    ospi_cfg->xip_base = (volatile void *)OSPI1_XIP_BASE;

    ospi_cfg->ser = 1;
    ospi_cfg->addrlen = ADDR_LENGTH_32_BITS;
    ospi_cfg->ospi_clock = OSPI_CLOCK;
    ospi_cfg->ddr_en = 0;  /* Start in SPI mode (SDR) */
    ospi_cfg->wait_cycles = MX_DEFAULT_WAIT_CYCLES;

    /* Initialize OSPI controller in SPI mode first */
    ospi_init(ospi_cfg);

    /* Probe flash: detect in SPI, switch to DOPI, verify in OPI DTR */
    if (mx_flash_probe(ospi_cfg) != 0) {
        return -1;
    }

    /* Initialize XIP mode */
    if (mx_flash_xip_init(ospi_cfg) != 0) {
        return -1;
    }

    return 0;
}

/*===========================================================================
 * Public API
 *===========================================================================*/

/**
 * @brief Initialize Macronix MX66UW1G45G NOR Flash and set to XIP mode
 * 
 * Flow:
 * 1. Detect flash in SPI mode
 * 2. Configure flash for DOPI (OPI DTR) mode
 * 3. Verify flash responds in OPI DTR mode
 * 4. Set up XIP for OPI DTR read
 * 
 * @return 0 on success, -1 on failure
 */
int init_mx_nor_flash_dtr(void)
{
    int ret;

    INFO("====================================================\n");
    INFO("MX66UW1G45G OSPI Flash Driver - OPI DTR Mode\n");
    INFO("====================================================\n");

    mx_setup_pinmux();

#if AES_EN
    if (service_ospi_write_aes_key()) {
        ERROR("MX: Unable to write OSPI AES KEY\n");
        return -1;
    }
#endif

    ret = mx_setup_flash_xip();

    if (ret != 0) {
        ERROR("MX: Unable to set OSPI flash in XIP mode\n");
        return -1;
    }

    INFO("MX: Configured OSPI1 NOR Flash successfully\n");

    return 0;
}

/**
 * @brief Get pointer to OSPI flash configuration
 */
ospi_flash_cfg_t *get_mx_ospi_flash_config(void)
{
    return &mx_ospi_flash_config;
}
