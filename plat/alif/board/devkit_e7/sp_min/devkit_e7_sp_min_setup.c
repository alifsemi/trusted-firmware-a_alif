/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/alif/common/plat_alif.h>
#include <lib/mmio.h>
#include <plat/alif/common/drivers/pinconf.h>
#include <plat/alif/common/drivers/ospi.h>
#include <plat/alif/common/drivers/issi_hyperram.h>
#include <arch_helpers.h>

#define LPGPIO_BASE                     0x42002000UL
#define GPIO_SWPORTA_DR_OFFSET          0x0
#define GPIO_SWPORTA_DDR_OFFSET         0x4
#define GPIO_INTMASK_OFFSET             0x34
#define GPIO_PIN_DIRECTION_INPUT        0
#define GPIO_PIN_DIRECTION_OUTPUT       1
#define OSPI_RESET_PIN                  6

extern int init_nor_flash(void);

#define PERIPH_CLK_ENA			0x4902F03C
#define PERIPH_CLK_ENA_OSPI0_CKEN	(1 << 0)
#define PERIPH_CLK_ENA_OSPI1_CKEN	(1 << 1)

static void devkit_e7_devices_init(void)
{
	uint32_t value;

	/* Enable peripheral and APB clocks */
	value = mmio_read_32(EXPMST0_CTRL_REG);
	mmio_write_32(EXPMST0_CTRL_REG, (value | 0xC0000000));

	/* Enable UART clock and select UART clock source */
	value = mmio_read_32(UART_CTRL_REG);
	mmio_write_32(UART_CTRL_REG, (value | 0x0000FFFF));

	value = mmio_read_32(PERIPH_CLK_ENA);
	mmio_write_32(PERIPH_CLK_ENA, (value | PERIPH_CLK_ENA_OSPI0_CKEN));

	pinconf_set(PORT_1, PIN_0, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE);
	pinconf_set(PORT_1, PIN_1, PINMUX_ALTERNATE_FUNCTION_1, 0);
}

#if HYPRAM_EN
static ospi_cfg_t ospi_cfg;

static void ospi_hyperram_init(void)
{
	pinconf_set(PORT_2, PIN_0, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA);
	pinconf_set(PORT_2, PIN_1, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA);
	pinconf_set(PORT_2, PIN_2, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA);
	pinconf_set(PORT_2, PIN_3, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA);
	pinconf_set(PORT_2, PIN_4, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA);
	pinconf_set(PORT_2, PIN_5, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA);
	pinconf_set(PORT_2, PIN_6, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA);
	pinconf_set(PORT_2, PIN_7, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA);
	pinconf_set(PORT_3, PIN_0, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA);
	pinconf_set(PORT_3, PIN_1, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA);
	pinconf_set(PORT_3, PIN_2, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA);
	pinconf_set(PORT_1, PIN_6, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE | PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA);

	ospi_cfg_t *ospi = &ospi_cfg;

	ospi->regs = (ospi_regs_t *) OSPI0_BASE;
	ospi->aes_regs = (aes_regs_t *) AES0_BASE;
	ospi->ss = 0;

	ospi_disable(ospi);
	writel(ospi, baudr, 0x4);
	writel(ospi, rx_sample_dly, 0x5);
	writel(ospi, txd_drive_edge, 0x1);
	ospi_enable(ospi);

        /* Set 64byte wrap length (b01) along with default settings*/
	hyper_ram_write_conf_reg0(ospi, 0x8f1d);

	hyper_ram_xip_init(ospi);
}
#endif
void plat_alif_sp_min_early_platform_setup(u_register_t arg0, u_register_t arg1,
			u_register_t arg2, u_register_t arg3)
{
	devkit_e7_devices_init();

	/* Call common early platform setup code for alif platforms */
	alif_sp_min_early_platform_setup((void *)arg0, arg1, arg2, (void *)arg3);
}

void plat_alif_sp_min_platform_setup(void)
{
#if HYPRAM_EN
	ospi_hyperram_init();
#endif

#if FLASH_EN
	if (init_nor_flash()) {
		ERROR("%s: OSPI1 NOR flash initialization failed\n",
			 __func__);
		panic();
	}

#endif
}
