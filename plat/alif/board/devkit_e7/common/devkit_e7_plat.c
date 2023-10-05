/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Table of regions to map using the MMU.
 * Replace or extend the below regions as required
 */
#include <plat/alif/common/plat_alif.h>

const mmap_region_t plat_alif_mmap[] = {
	DEVKIT_E7_MAP_DEVICE,
	UART_MAP_DEVICE,
	OSPI_MAP_DEVICE,
	GPIO_MAP_DEVICE,
	AES0_MAP_DEVICE,
	{0}
};

unsigned int plat_get_syscnt_freq2(void)
{
        return SYS_COUNTER_FREQ_IN_TICKS;
}
