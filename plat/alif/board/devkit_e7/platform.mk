#
# Copyright (c) 2025, Alif Semiconductor. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#

$(eval $(call add_define,HYPRAM_EN))

BL32_SOURCES		+=	lib/xlat_tables/aarch32/xlat_tables.c	\
				lib/xlat_tables/xlat_tables_common.c	\
				plat/alif/common/alif_common.c		\
				lib/cpus/aarch32/cortex_a32.S

PLAT_INCLUDES		:=	-Iplat/alif/board/devkit_e7/common/include	\
				-Iinclude/plat/alif/common	\
				-Iplat/alif/common/drivers/ospi/

NEED_BL32		:=	yes

ifeq (${AARCH32_SP},none)
    $(error Variable AARCH32_SP has to be set for AArch32)
endif

# Include GICv2 driver files
include drivers/arm/gic/v2/gicv2.mk

DEVKIT_E7_GIC_SOURCES	:=	${GICV2_SOURCES}			\
				plat/alif/common/alif_gicv2.c

# BL1/BL2 Image not a part of the capsule Image for devkit e7
override NEED_BL1	:=	no
override NEED_BL2	:=	no
override NEED_BL2U	:=	no
override NEED_BL33	:=	no

#TFA for ALIF platforms starts from BL32
override RESET_TO_SP_MIN	:=	1
override BL32_IN_XIP_MEM	:= 	1

# Check for Linux kernel as a BL33 image by default
$(eval $(call add_define,ARM_LINUX_KERNEL_AS_BL33))
  ifndef ARM_PRELOADED_DTB_BASE
    $(error "ARM_PRELOADED_DTB_BASE must be set if ARM_LINUX_KERNEL_AS_BL33 is used.")
  endif
  $(eval $(call add_define,ARM_PRELOADED_DTB_BASE))

# Adding TARGET_PLATFORM as a GCC define (-D option)
$(eval $(call add_define,TARGET_PLATFORM_$(call uppercase,${TARGET_PLATFORM})))
(eval $(call add_define,HYPRAM_EN))
(eval $(call add_define,FLASH_EN))
