/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/alif/common/plat_alif.h>
#include <lib/mmio.h>
#include <common/debug.h>
#include <lib/utils_def.h>
#include <drivers/delay_timer.h>
#include <lib/libc/errno.h>

/* Constants for delay calculations */
#define MICROSEC_PER_SEC             1000000U
#define COUNTER_MAX_VALUE            0xFFFFFFFFFFFFFFFFULL
#define MIN_DELAY_US                 1U
#define DELAY_LOOP_THRESHOLD_US      100U

/**
 * @brief Memory map table for the Devkit E7 platform.
 * 
 * This table defines the memory regions and their attributes for the platform.
 * The last entry must be a null terminator {0}.
 */
const mmap_region_t plat_alif_mmap[] = {
    DEVKIT_E7_MAP_DEVICE,
    UART_MAP_DEVICE,
    OSPI_MAP_DEVICE,
    GPIO_MAP_DEVICE,
    SE_MHU0_SEND_DEVICE,
    SE_MHU0_RECV_DEVICE,
    MHU0_PAYLOAD_MAP,
    LPGPIO_MAP_DEVICE,
    CLKCTL_SLV_MAP_DEVICE,
    {0} /* Null terminator */
};

/**
 * @brief Get the platform memory map
 * 
 * @return const mmap_region_t* Pointer to the platform memory map
 */
const mmap_region_t *plat_get_alif_mmap(void)
{
    return plat_alif_mmap;
}

/**
 * @brief Returns the frequency of the system counter in ticks per second.
 *
 * @return System counter frequency in Hz.
 */
unsigned int plat_get_syscnt_freq2(void)
{
    return SYS_COUNTER_FREQ_IN_TICKS;
}

/**
 * @brief Delays execution for a specified number of microseconds.
 *
 * This function uses the system counter to generate precise delays.
 * For very short delays (< DELAY_LOOP_THRESHOLD_US), a simple loop is used
 * to avoid the overhead of counter calculations.
 *
 * @param delay_us Number of microseconds to delay (must be > 0)
 * @return int 0 on success, negative error code on failure
 */
int delay_in_us(uint32_t delay_us)
{
       uint32_t cnt_clk_freq;
       uint64_t total_count, pre_timestamp, cur_timestamp, cur_count;
       /* Get system counter timer frequency */
       cnt_clk_freq = plat_get_syscnt_freq2();
       /* Get total count value for the given microseconds */
       total_count = (delay_us * (cnt_clk_freq/MICROSEC_PER_SEC));

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
                       cur_count += ((COUNTER_MAX_VALUE - pre_timestamp)
                                       + cur_timestamp);
               }
               /* Update latest counter value */
               pre_timestamp = cur_timestamp;
       }
    return 0;
}

/**
 * @brief Initialize platform-specific delay timer
 */
void plat_delay_timer_init(void)
{
    /* Register the delay timer function */
    timer_ops_t ops = {
        .clk_mult = 0,
        .clk_div = 0,
        .get_timer_value = NULL,
        .delay = delay_in_us
    };

    timer_init(&ops);
}
