# Alif Semiconductor Devkit E7 (BL32 / SP_MIN)

This platform builds **BL32 (SP_MIN)** for Alif's Ensemble E7/E8 devices. The
boot flow starts directly at BL32 (AArch32); there is no BL1/BL2/BL31, so the
build target is always `bl32`.

Build instructions — including the memory-relocation knobs
(`ALIF_BL32_XIP_BASE`, `ALIF_TRUSTED_SRAM_BASE`, `PRELOADED_BL33_BASE`, …), the
`ALIF_SOC_E8` selector for building for E7, and ready-to-use command lines for
the upstream and Alif SDK Zephyr payloads — are documented at:

- [`docs/plat/alif.rst`](../../../../docs/plat/alif.rst)

Quick start (upstream Zephyr, booting from `0x80000000`):

```shell
make CROSS_COMPILE=arm-zephyr-eabi- ARCH=aarch32 PLAT=devkit_e7 \
     ALIF_BL32_XIP_BASE=0x80100000 \
     ALIF_TRUSTED_SRAM_BASE=0x2400000 \
     PRELOADED_BL33_BASE=0x80000000 \
     bl32
```

See the full document for the Alif SDK variant and E7 builds.
