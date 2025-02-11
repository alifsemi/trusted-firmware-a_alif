#
# Copyright (c) 2025, Alif Semiconductor. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# SP MIN source files common to Alif platforms
#
BL32_SOURCES		+=	plat/alif/common/sp_min/alif_sp_min_setup.c	\
				plat/alif/common/alif_pm.c			\
				plat/alif/common/alif_console.c			\
				plat/alif/common/drivers/pinconf/pinconf.c	\
				plat/alif/common/drivers/ospi/issi_hyperram.c   \
				plat/common/aarch32/platform_mp_stack.S		\
				plat/common/plat_psci_common.c			\
				drivers/ti/uart/aarch32/16550_console.S
