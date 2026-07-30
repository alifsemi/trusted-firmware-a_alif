Alif Semiconductor Ensemble (Devkit E7)
=======================================

This port targets Alif Semiconductor's Ensemble family (E7 and E8) using the
``devkit_e7`` platform. Unlike upstream TF-A, the Alif boot flow starts directly
at **BL32 (SP_MIN)** and is **AArch32**: there is no BL1/BL2/BL31, and the build
target is always ``bl32``. BL32 performs early SoC setup and then hands control
to a preloaded non-secure payload (BL33) such as Zephyr or Linux.

BL32 runs execute-in-place (XIP): its code and read-only data stay in MRAM and
are executed from there, while its writable data (``.data``/``.bss``/stack/
translation tables) run from Trusted SRAM. The ``.data`` section is copied from
its MRAM shadow to SRAM at boot.

Memory layout knobs
-------------------

The location and size of both regions, and the location of the preloaded
payload, are build-time configurable so the image can be moved for different
boot scenarios without editing the source. All addresses must be 4 KB
(page) aligned.

.. list-table::
   :header-rows: 1
   :widths: 30 15 55

   * - Build option
     - Default
     - Description
   * - ``ALIF_BL32_XIP_BASE``
     - ``0x80002000``
     - Base of the code + read-only data region, executed in place from MRAM.
   * - ``ALIF_BL32_XIP_SIZE``
     - ``0x8000``
     - Size of the code + read-only data (RO) region (32 KB).
   * - ``ALIF_TRUSTED_SRAM_BASE``
     - ``0x2000000``
     - Base of the writable data/bss/stack region in Trusted SRAM.
   * - ``ALIF_TRUSTED_SRAM_SIZE``
     - ``0x20000``
     - Size of the writable (RW) region (128 KB).
   * - ``PRELOADED_BL33_BASE``
     - *(required)*
     - Address at which the non-secure payload (Zephyr/Linux) is preloaded.

The default ``ALIF_TRUSTED_SRAM_BASE`` (``0x2000000``, the start of SRAM0) is
chosen because it is valid on **both** E7 and E8. Layouts that place BL32's RW
region higher in SRAM (see the Zephyr examples below) use an address that is
only available on E8.

SoC selection
-------------

.. list-table::
   :header-rows: 1
   :widths: 25 15 60

   * - Build option
     - Default
     - Description
   * - ``ALIF_SOC_E8``
     - ``1``
     - Selects code that programs registers present only on the E8 SoC (the
       OSPI clock-enable bits in ``PERIPH_CLK_ENA`` and the NPU-HG registers).
       Build with ``ALIF_SOC_E8=0`` for E7 devices, where those registers are
       absent; leaving the default booted on an E7 will fault.

Building
--------

A cross toolchain is required (``arm-zephyr-eabi-`` is used below). ``ARCH`` must
be set to ``aarch32``; ``AARCH32_SP`` defaults to ``sp_min`` for this platform
and need not be passed. The build target is ``bl32``.

Boot examples (Zephyr)
----------------------

Two Zephyr variants are supported. They boot from different MRAM addresses, so
BL32's XIP region and the payload base are swapped between them: whichever base
Zephyr does not occupy is where BL32's code is placed.

.. list-table::
   :header-rows: 1
   :widths: 30 25 25 25

   * - Payload
     - Zephyr boots at
     - ``ALIF_BL32_XIP_BASE``
     - ``PRELOADED_BL33_BASE``
   * - Upstream Zephyr
     - ``0x80000000``
     - ``0x80100000``
     - ``0x80000000``
   * - Zephyr Alif SDK
     - ``0x80100000``
     - ``0x80000000``
     - ``0x80100000``

Both examples place BL32's RW region at ``0x2400000``. Zephyr uses the 4 MB of
SRAM starting at ``0x2000000`` (``0x2000000``–``0x2400000``), so BL32's data is
placed immediately above it to avoid overlap. This address is only valid on E8;
for E7, choose an unused base within the device's SRAM (for example ``0x8000000``).

Upstream Zephyr (boots from ``0x80000000``):

.. code:: shell

    make CROSS_COMPILE=arm-zephyr-eabi- ARCH=aarch32 PLAT=devkit_e7 \
         ALIF_BL32_XIP_BASE=0x80100000 \
         ALIF_TRUSTED_SRAM_BASE=0x2400000 \
         PRELOADED_BL33_BASE=0x80000000 \
         bl32

Zephyr Alif SDK (boots from ``0x80100000``):

.. code:: shell

    make CROSS_COMPILE=arm-zephyr-eabi- ARCH=aarch32 PLAT=devkit_e7 \
         ALIF_BL32_XIP_BASE=0x80000000 \
         ALIF_TRUSTED_SRAM_BASE=0x2400000 \
         PRELOADED_BL33_BASE=0x80100000 \
         bl32

Flashing with Alif SETOOLs
--------------------------

BL32 and the payload are programmed into MRAM with the Alif SETOOLs, which take
a JSON config describing each binary and the ``mramAddress`` it is flashed to.
Because BL32 executes in place, it must be flashed at *exactly* the address it
was linked for, and it enters the payload at *exactly* the configured base. The
SETOOLs addresses must therefore match the build knobs:

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Build option
     - SETOOLs JSON field
   * - ``ALIF_BL32_XIP_BASE``
     - ``BOOTLOAD.mramAddress`` (``bl32.bin``)
   * - ``PRELOADED_BL33_BASE``
     - ``A32_APP.mramAddress`` (``zephyr.bin``)

For the upstream-Zephyr example above (``ALIF_BL32_XIP_BASE=0x80100000``,
``PRELOADED_BL33_BASE=0x80000000``), the matching SETOOLs entries are::

    "BOOTLOAD": {
        "binary": "bl32.bin",
        "version" : "0.4.3",
        "mramAddress": "0x80100000",
        "signed": true,
        "cpu_id": "A32_0",
        "flags": ["boot"]
    },
    "A32_APP": {
        "binary": "zephyr.bin",
        "version" : "1.0.0",
        "mramAddress": "0x80000000",
        "signed": true,
        "cpu_id": "A32_0"
    },


The Alif SDK layout simply swaps the two ``mramAddress`` values to match its
build knobs. A mismatch between the build configuration and the SETOOLs
addresses will not boot.

Building for E7
---------------

For E7 devices, add ``ALIF_SOC_E8=0`` and use an ``ALIF_TRUSTED_SRAM_BASE`` that
lies within E7 memory. For example, the Zephyr Alif SDK layout on an E7:

.. code:: shell

    make CROSS_COMPILE=arm-zephyr-eabi- ARCH=aarch32 PLAT=devkit_e7 \
         ALIF_SOC_E8=0 \
         ALIF_BL32_XIP_BASE=0x80000000 \
         PRELOADED_BL33_BASE=0x80100000 \
         ALIF_TRUSTED_SRAM_BASE=0x8000000 \
         bl32

--------------

*Copyright (c) 2026, Alif Semiconductor. All rights reserved.*
