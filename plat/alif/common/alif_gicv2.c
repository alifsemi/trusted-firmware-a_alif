/*
 * Copyright (c) 2025, Alif Semiconductor. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>

#include <drivers/arm/gicv2.h>
#include <plat/alif/common/plat_alif.h>
#include <plat/common/platform.h>

/******************************************************************************
 * The following functions are defined as weak to allow a platform to override
 * the way the GICv2 driver is initialised and used.
 *****************************************************************************/
#pragma weak plat_alif_gic_driver_init
#pragma weak plat_alif_gic_init

static const interrupt_prop_t alif_interrupt_props[] = {
	PLAT_ALIF_G0_IRQ_PROPS(GICV2_INTR_GROUP0)
};

static unsigned int target_mask_array[PLATFORM_CORE_COUNT];

static const gicv2_driver_data_t alif_gic_data = {
	.gicd_base = PLAT_ALIF_GICD_BASE,
	.gicc_base = PLAT_ALIF_GICC_BASE,
	.interrupt_props = alif_interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(alif_interrupt_props),
	.target_masks = target_mask_array,
	.target_masks_num = ARRAY_SIZE(target_mask_array),
};

/******************************************************************************
 * ALIF common helpers to initialize the GICv2 driver.
 *****************************************************************************/
void plat_alif_gic_driver_init(void)
{
	gicv2_driver_init(&alif_gic_data);
}

void plat_alif_gic_init(void)
{
	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_set_pe_target_mask(plat_my_core_pos());
	gicv2_cpuif_enable();
}

/******************************************************************************
 * ALIF common helper to enable the GICv2 CPU interface
 *****************************************************************************/
void plat_alif_gic_cpuif_enable(void)
{
	gicv2_cpuif_enable();
}

/******************************************************************************
 * ALIF common helper to disable the GICv2 CPU interface
 *****************************************************************************/
void plat_alif_gic_cpuif_disable(void)
{
	gicv2_cpuif_disable();
}

/******************************************************************************
 * ALIF common helper to initialize the per cpu distributor interface in GICv2
 *****************************************************************************/
void plat_alif_gic_pcpu_init(void)
{
	gicv2_pcpu_distif_init();
	gicv2_set_pe_target_mask(plat_my_core_pos());
}
