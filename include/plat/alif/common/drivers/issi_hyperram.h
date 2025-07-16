/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ISSI_HYPERRAM_H
#define ISSI_HYPERRAM_H

#include "ospi.h"
#include <stdint.h>

/* Write to the configuration register 0 of the ISSI hyperram device */
int hyper_ram_write_conf_reg0(ospi_cfg_t *ospi, uint16_t data);

/* Initialize hyperram xip mode */
int hyper_ram_xip_init(ospi_cfg_t *ospi);
#endif /* ISSI_HYPERRAM_H */

