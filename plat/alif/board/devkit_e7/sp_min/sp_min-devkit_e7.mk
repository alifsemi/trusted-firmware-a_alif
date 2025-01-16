#
# Copyright (c) 2025, Alif Semiconductor. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# SP_MIN source files specific to the Devkit E7 platform
BL32_SOURCES	+=	plat/alif/board/devkit_e7/sp_min/devkit_e7_sp_min_setup.c	\
			plat/alif/board/devkit_e7/common/devkit_e7_plat.c		\
			plat/alif/board/devkit_e7/common/devkit_e7_helpers.S		\
			plat/alif/board/devkit_e7/common/devkit_e7_topology.c		\
			plat/alif/board/devkit_e7/common/devkit_e7_pm.c			\
			${DEVKIT_E7_GIC_SOURCES}

ifneq (${ENABLE_STACK_PROTECTOR},0)
	ifneq (${ENABLE_STACK_PROTECTOR},none)
		BL32_SOURCES += plat/alif/board/devkit_e7/common/devkit_e7_stack_protector.c
	endif
endif

include plat/alif/common/sp_min/alif_sp_min.mk
