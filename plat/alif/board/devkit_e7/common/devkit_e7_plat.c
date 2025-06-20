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
#include <lib/mmio.h>
#include <common/debug.h>

#define MICRO_UNIT		1000000
#define MAX_REFCLK_COUNT	0xFFFFFFFFFFFFFFFFULL

const mmap_region_t plat_alif_mmap[] = {
	DEVKIT_E7_MAP_DEVICE,
	UART_MAP_DEVICE,
	OSPI_MAP_DEVICE,
	GPIO_MAP_DEVICE,
	AES0_MAP_DEVICE,
	OSPI1_MAP_DEVICE,
	AES1_MAP_DEVICE,
	SE_MHU0_SEND_DEVICE,
	SE_MHU0_RECV_DEVICE,
	MHU0_PAYLOAD_MAP,
	LPGPIO_MAP_DEVICE,
	{0}
};

unsigned int plat_get_syscnt_freq2(void)
{
        return SYS_COUNTER_FREQ_IN_TICKS;
}

/* @brief Gives delay in microseconds.
 * parameters,
 * delay - delay value in microseconds.
 */
void delay_in_us(uint32_t delay)
{
       uint32_t cnt_clk_freq;
       uint64_t total_count, pre_timestamp, cur_timestamp, cur_count;
       /* Get system counter timer frequency */
       cnt_clk_freq = plat_get_syscnt_freq2();
       /* Get total count value for the given microseconds */
       total_count = (delay * (cnt_clk_freq/MICRO_UNIT));

       cur_count = 0;

       /* Read the start counter value */
       cur_timestamp = mmio_read_64(ARM_SYS_CNTREAD_BASE);
       pre_timestamp = cur_timestamp;

       /* Loop until the counter value exceeds the required count */
       while (cur_count < total_count) {
               cur_timestamp = mmio_read_64(ARM_SYS_CNTREAD_BASE);
               dsb();
               if (cur_timestamp > pre_timestamp) {
                       /* Increament count */
                       cur_count += (cur_timestamp - pre_timestamp);
               } else {
                       /* The 64bit counter overflowed so adjust */
                       /* count by subtracting with the max value */
                       cur_count += ((MAX_REFCLK_COUNT - pre_timestamp)
                                       + cur_timestamp);
               }
               /* Update latest counter value */
               pre_timestamp = cur_timestamp;
       }
}
