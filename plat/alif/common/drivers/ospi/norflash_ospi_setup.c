/* Copyright (C) 2022 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

#include "dwc_spi.h"
#include "ospi.h"
#include "ospi_drv.h"
#include "ospi_xip_user.h"

#define PAD_CTRL_DATA (PAD_CTRL_12MA|PAD_CTRL_SR|PAD_CTRL_REN)
#define PAD_CTRL_CLK (PAD_CTRL_12MA|PAD_CTRL_SR)

#define WRAP_32_BYTE                    0xFD
#define OCTAL_DDR_DQS                   0xE7
#define DEVICE_ID_ISSI_FLASH_IS25WX256  0x9D
#define SPI_ENABLE                      1U
#define SPI_DISABLE                     0U

/* OSPI FLASH CONFIG */
static ospi_flash_cfg_t ospi_flash_config;

void static setup_PinMUX()
{

    /* Configure pad control registers and Mux value*/
    *PADCTRL_REG(9, 5, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(9, 6, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(9, 7, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(10, 0, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(10, 1, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(10, 2, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(10, 3, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(10, 4, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(10, 7, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(5, 5, PAD_CTRL_CLK, 1);
    *PADCTRL_REG(5, 7, PAD_CTRL_12MA, 1);
    *PADCTRL_REG(5, 6, PAD_CTRL_DATA, 1);
    *PADCTRL_REG(8, 0, PAD_CTRL_12MA, 1);
    return ;
}

static void issi_write_enable(ospi_flash_cfg_t *ospi_cfg)
{
    /* Write WEL bit in OctalSPI mode */
    ospi_setup_write(ospi_cfg, ADDR_LENGTH_0_BITS);
    ospi_send(ospi_cfg, ISSI_WRITE_ENABLE);
}

static uint8_t issi_decode_id(ospi_flash_cfg_t *ospi_cfg, uint8_t *buffer)
{
    uint8_t iter, id = 0;

    for (iter = 0 ; iter < 8; iter++)
    {
        /* Since SPI controller supports octal mode only, so 1 byte of data sent by flash will be distributed over 8 byte data read */
        if (*buffer & 0x2)
        {
            id |= 1;
        }
        if (iter < 7)
        {
            id <<= 1;
        }
        buffer++;
    }

    ospi_cfg->device_id = id;
    return id;
}

static void ospi_write_en(ospi_flash_cfg_t *ospi_cfg)
{
    /* Write WEL bit in OctalSPI mode */
    ospi_setup_write(ospi_cfg, ADDR_LENGTH_0_BITS);
    ospi_send(ospi_cfg, ISSI_WRITE_ENABLE);	/* Write data payload */
}

static void issi_flash_set_configuration_register_DDR(ospi_flash_cfg_t *ospi_cfg, uint8_t cmd, uint8_t address, uint8_t value)
{
    ospi_write_en(ospi_cfg);
    ospi_setup_write(ospi_cfg, ADDR_LENGTH_32_BITS);
    ospi_push(ospi_cfg, cmd);		/* Write Status Register command */
    ospi_push(ospi_cfg, address);	/* Write address byte */
    ospi_push(ospi_cfg, value);
    ospi_send(ospi_cfg, value);		/* Write data byte */
    return;
}

static uint32_t issi_flash_read_configuration_register_ddr(ospi_flash_cfg_t *ospi_cfg, uint32_t reg_type, uint32_t cmd)
{
    uint8_t rBuff[256] = {0};
    /* Read Memory Status register in OctalSPI mode */
    ospi_setup_read(ospi_cfg, ADDR_LENGTH_32_BITS, 1, 8);

    if(reg_type == 0) {
	ospi_push(ospi_cfg, ISSI_READ_VOLATILE_CONFIG_REG);	/* Get Status/Control Registers command */
    }
    else if (reg_type == 1) {
	ospi_push(ospi_cfg, ISSI_READ_NONVOLATILE_CONFIG_REG);	/* Get Status/Control Registers command */
    }

    ospi_recv(ospi_cfg, cmd, rBuff);

    return (uint32_t)rBuff[0] ;
}

static void issi_flash_set_configuration_register_SDR(ospi_flash_cfg_t *ospi_cfg, uint8_t cmd, uint8_t address, uint8_t value)
{
    issi_write_enable(ospi_cfg);
    ospi_setup_write_sdr(ospi_cfg, ADDR_LENGTH_24_BITS);
    ospi_push(ospi_cfg, cmd);
    ospi_push(ospi_cfg, 0x00);
    ospi_push(ospi_cfg, 0x00);
    ospi_push(ospi_cfg, address);
    ospi_send(ospi_cfg, value);
}

static uint8_t issi_flash_ReadID(ospi_flash_cfg_t *ospi_cfg)
{
    uint8_t buffer[8];

    ospi_setup_read(ospi_cfg, ADDR_LENGTH_0_BITS, 8, 0);
    ospi_recv(ospi_cfg, ISSI_READ_ID, buffer);

    return issi_decode_id(ospi_cfg, buffer);
}

static int issi_flash_probe (ospi_flash_cfg_t *ospi_cfg)
{
    /* Initialize SPI in Single mode 1-1-1 and read Flash ID */
    if (issi_flash_ReadID(ospi_cfg) == DEVICE_ID_ISSI_FLASH_IS25WX256)
    {
        /* Set wrap configuration to 32 bytes */
        issi_flash_set_configuration_register_SDR(ospi_cfg, ISSI_WRITE_VOLATILE_CONFIG_REG, 0x07, WRAP_32_BYTE);

        /* Switch the flash to Octal DDR mode */
        issi_flash_set_configuration_register_SDR(ospi_cfg, ISSI_WRITE_VOLATILE_CONFIG_REG, 0x00, OCTAL_DDR_DQS);

        return 0;
    }

    return -1;
}

static int flash_xip_init(ospi_flash_cfg_t *ospi_cfg)
{
    ospi_xip_enter(ospi_cfg, ISSI_DDR_OCTAL_IO_FAST_READ, ISSI_DDR_OCTAL_IO_FAST_READ);
    return 0;
}

static int setup_flash_xip(void)
{
    ospi_flash_cfg_t *ospi_cfg = &ospi_flash_config;

    ospi_cfg->regs = (ssi_regs_t *) OSPI1_BASE;
    ospi_cfg->aes_regs = (aes_regs_t *) AES1_BASE;
    ospi_cfg->xip_base = (volatile void *) OSPI1_XIP_BASE;

    ospi_cfg->ser = 1;
    ospi_cfg->addrlen = ADDR_LENGTH_32_BITS;
    ospi_cfg->ospi_clock = OSPI_CLOCK;
    ospi_cfg->ddr_en = 0;
    ospi_cfg->wait_cycles = DEFAULT_WAIT_CYCLES_ISSI;

    ospi_init(ospi_cfg);

    if (issi_flash_probe(ospi_cfg))
    {
        return -1;
    }
    ospi_cfg->ddr_en = 1;

    //Specific for A32.
    if (issi_flash_read_configuration_register_ddr(ospi_cfg, 0, 0x01) != ospi_cfg->wait_cycles)
    {
	issi_flash_set_configuration_register_DDR(ospi_cfg, ISSI_WRITE_VOLATILE_CONFIG_REG, 0x01, ospi_cfg->wait_cycles);
    }

    /* Check and Set Wrap Configuration to 64-byte wrap */
    if (issi_flash_read_configuration_register_ddr(ospi_cfg, 0, 0x07) != 0xFE)
    {
	issi_flash_set_configuration_register_DDR(ospi_cfg, ISSI_WRITE_VOLATILE_CONFIG_REG, 0x07, 0xFE);
    }

    if (flash_xip_init(ospi_cfg))
    {
        return -1;
    }

    return 0;
}

/* Init Flash and set to XiP Mode */
int init_nor_flash(void)
{
    int ret;

    setup_PinMUX();

    ret = setup_flash_xip();

    if (ret)
    {
         while(1);
    }
    return 0;
}
