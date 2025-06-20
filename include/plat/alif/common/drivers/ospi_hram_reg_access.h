/* Copyright (C) 2023 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

 /**************************************************************************//**
 * @file     ospi_hram_reg_access.h
 * @author   Silesh C V
 * @email    silesh@alifsemi.com
 * @version  V1.0.0
 * @date     12-Sep-2023
 * @brief    Utility functions to access the registers of the ISSI hram device.
 ******************************************************************************/
#ifndef OSPI_HRAM_REG_ACCESS_H
#define OSPI_HRAM_REG_ACCESS_H

#include "ospi_drv.h"
#include <stdint.h>

/* Write to the configuration register 0 */
void hyper_ram_command_write_conf_reg0(ospi_cfg_t *ospi, uint16_t data);

/* Get all the registers of the hyperram device */
void hyper_ram_get_regs(ospi_cfg_t *ospi);
#endif /* OSPI_HRAM_REG_ACCESS_H */

