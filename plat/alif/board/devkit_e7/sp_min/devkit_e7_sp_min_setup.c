/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/alif/common/plat_alif.h>
#include <lib/mmio.h>
#include <plat/alif/common/drivers/pinconf.h>

static void devkit_e7_devices_init(void)
{
	uint32_t value;
	/* Enable peripheral and APB clocks */
	value = mmio_read_32(EXPMST0_CTRL_REG);
	mmio_write_32(EXPMST0_CTRL_REG, (value | 0xC0000000));

	/* Enable UART clock and select UART clock source */
	value = mmio_read_32(UART_CTRL_REG);
	mmio_write_32(UART_CTRL_REG, (value | 0x0000FFFF));

	pinconf_set(PORT_1, PIN_0, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_READ_ENABLE);
	pinconf_set(PORT_1, PIN_1, PINMUX_ALTERNATE_FUNCTION_1, 0);
}

void plat_alif_sp_min_early_platform_setup(u_register_t arg0, u_register_t arg1,
			u_register_t arg2, u_register_t arg3)
{
	devkit_e7_devices_init();

	/* Call common early platform setup code for alif platforms */
	alif_sp_min_early_platform_setup((void *)arg0, arg1, arg2, (void *)arg3);
}
