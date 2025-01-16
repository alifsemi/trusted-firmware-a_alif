/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/psci/psci.h>
#include <lib/mmio.h>
#include <plat/alif/common/plat_alif.h>

extern unsigned int secondary_cpu_flags[3];

static int ensemble_pwr_domain_on(u_register_t mpidr)
{
	int rc = PSCI_E_SUCCESS;
	unsigned int cpu = mpidr, val;

#define HOST_BASE_SYS_CTRL              0x1A010000
#define PE_CONFIG(cpu)                  ((cpu) * 0x10 + 0x0)
#define HOST_CPU_BOOT_MSK               0x300
#define HOST_CPU_WAKEUP                 0x308

	/* Set the flag so that core jumps to sp_min_warm_boot */
	secondary_cpu_flags[cpu - 1] = 0x000ADD;

	flush_dcache_range((uint32_t)&secondary_cpu_flags,
		sizeof(secondary_cpu_flags));

	val = mmio_read_32(HOST_BASE_SYS_CTRL + HOST_CPU_BOOT_MSK);
	val |= (1 << cpu);
	mmio_write_32(HOST_BASE_SYS_CTRL + HOST_CPU_BOOT_MSK, val);

	val = mmio_read_32(HOST_BASE_SYS_CTRL + HOST_CPU_WAKEUP);
	val |= (1 << cpu);
	mmio_write_32(HOST_BASE_SYS_CTRL + HOST_CPU_WAKEUP, val);

	dsb();
	return rc;
}

static void ensemble_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	/* Enable the gic cpu interface */
	plat_alif_gic_pcpu_init();

	/* Program the gic per-cpu distributor or re-distributor interface */
	plat_alif_gic_cpuif_enable();
}

/*******************************************************************************
 * Export the platform handlers via plat_alif_psci_pm_ops. The ALIF Standard
 * platform layer will take care of registering the handlers with PSCI.
 ******************************************************************************/
plat_psci_ops_t plat_alif_psci_pm_ops = {
	.pwr_domain_on = ensemble_pwr_domain_on,
	.pwr_domain_on_finish = ensemble_pwr_domain_on_finish,
};

const plat_psci_ops_t *plat_alif_psci_override_pm_ops(plat_psci_ops_t *ops)
{
	return ops;
}
