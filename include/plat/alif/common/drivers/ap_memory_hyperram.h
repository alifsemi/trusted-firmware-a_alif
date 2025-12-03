/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef AP_MEMORY_HYPERRAM_H
#define AP_MEMORY_HYPERRAM_H

#include <stdint.h>

/**
 * @brief Initialize AP Memory APS512Mb PSRAM via OSPI0
 *
 * This function initializes the APS512Mb PSRAM connected via OSPI0 interface:
 * - Configures pinmux for OSPI signals (D0-D15, CLK, CS, DQS)
 * - Initializes OSPI controller with proper timing
 * - Configures AES RXDS delay for DDR mode
 * - Performs global reset of PSRAM
 * - Verifies device ID
 * - Configures read/write wait cycles
 * - Enables dual-octal mode (x16 data bus)
 * - Configures XIP mode for memory-mapped access
 *
 * After initialization, PSRAM is accessible at XIP base address 0xA0000000
 *
 * @return 0 on success, negative error code on failure
 */
int ap_memory_hyperram_init(void);

/**
 * @brief Test HyperRAM 64MB memory space (rigorous)
 *
 * Performs comprehensive memory validation including:
 * - Data bus test (walking 1s and 0s on all 32 bits)
 * - Address bus test (aliasing detection across 64MB)
 * - March C- algorithm (industry-standard fault detection)
 * - Checkerboard pattern test (adjacent cell interference)
 * - Pseudo-random pattern test (data retention)
 * - Address-in-address test (address line verification)
 * - Bit-flip stress test (transition faults)
 *
 * Tests run on 4KB blocks sampled every 4MB across the 64MB range.
 *
 * @return 0 on success, -1 on failure
 */
int ap_memory_hyperram_test_64mb(void);

#endif /* AP_MEMORY_HYPERRAM_H */
