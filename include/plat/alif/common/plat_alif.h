/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_ALIF_H
#define PLAT_ALIF_H

#include <stdint.h>

#include <lib/el3_runtime/cpu_data.h>
#include <lib/xlat_tables/xlat_tables_compat.h>

/* SP_MIN utility functions */
void alif_sp_min_early_platform_setup(void *from_bl2, uintptr_t tos_fw_config,
				uintptr_t hw_config, void *plat_params_from_bl2);
void alif_sp_min_plat_runtime_setup(void);
void alif_sp_min_plat_arch_setup(void);

const mmap_region_t *plat_alif_get_mmap(void);

/* GIC utility functions */
void plat_alif_gic_driver_init(void);
void plat_alif_gic_init(void);
void plat_alif_gic_cpuif_enable(void);
void plat_alif_gic_cpuif_disable(void);
void plat_alif_gic_pcpu_init(void);

/* Console utility functions */
void alif_console_boot_init(void);
void alif_console_boot_end(void);
void alif_console_runtime_init(void);
void alif_console_runtime_end(void);

/*
 * Utility functions
 */
uint32_t alif_get_spsr_for_bl33_entry(void);
uint32_t plat_alif_calc_core_pos(u_register_t mpidr);

/* Optional functions for SP_MIN */
void plat_alif_sp_min_early_platform_setup(u_register_t arg0, u_register_t arg1,
			u_register_t arg2, u_register_t arg3);


void plat_alif_sp_min_platform_setup(void);
/* Allow platform to override psci_pm_ops during runtime */
const plat_psci_ops_t *plat_alif_psci_override_pm_ops(plat_psci_ops_t *ops);

/* global variables */
extern plat_psci_ops_t plat_alif_psci_pm_ops;
extern const mmap_region_t plat_alif_mmap[];
#endif /* PLAT_ALIF_H */
