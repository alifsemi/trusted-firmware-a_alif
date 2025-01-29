/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>
#include <arch_helpers.h>
#include <lib/psci/psci.h>
#include <plat/alif/common/plat_alif.h>
#include <plat/common/platform.h>

int __init plat_setup_psci_ops(uintptr_t sec_entrypoint,
				const plat_psci_ops_t **psci_ops)
{
	*psci_ops = plat_alif_psci_override_pm_ops(&plat_alif_psci_pm_ops);

	return 0;
}
