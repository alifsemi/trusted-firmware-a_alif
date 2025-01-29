/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <platform_def.h>

#include <common/debug.h>
#include <drivers/ti/uart/uart_16550.h>
#include <drivers/console.h>
#include <plat/alif/common/plat_alif.h>

#pragma weak alif_console_runtime_init
#pragma weak alif_console_runtime_end

/*******************************************************************************
 * Functions that set up the console
 ******************************************************************************/
static console_t alif_boot_console;
static console_t alif_runtime_console;

/* Initialize the console to provide early debug support */
void __init alif_console_boot_init(void)
{
	/* If the console was initialized already, don't initialize again */
	if (alif_boot_console.base == PLAT_ALIF_BOOT_UART_BASE) {
		return;
	}

	int rc = console_16550_register(PLAT_ALIF_BOOT_UART_BASE,
					PLAT_ALIF_BOOT_UART_CLK_IN_HZ,
					PLAT_ALIF_CONSOLE_BAUDRATE,
					&alif_boot_console);
	if (rc == 0) {
		/*
		 * The crash console doesn't use the multi console API, it uses
		 * the core console functions directly. It is safe to call panic
		 * and let it print debug information.
		 */
		panic();
	}

	console_set_scope(&alif_boot_console, CONSOLE_FLAG_BOOT);
}

void alif_console_boot_end(void)
{
	console_flush();
	(void)console_unregister(&alif_boot_console);
}

/* Initialize the runtime console */
void alif_console_runtime_init(void)
{
	int rc = console_16550_register(PLAT_ALIF_RUN_UART_BASE,
					PLAT_ALIF_RUN_UART_CLK_IN_HZ,
					PLAT_ALIF_CONSOLE_BAUDRATE,
					&alif_runtime_console);
	if (rc == 0)
		panic();

	console_set_scope(&alif_runtime_console, CONSOLE_FLAG_RUNTIME);
}

void alif_console_runtime_end(void)
{
	console_flush();
}
