.. _pbus:

=========
PBUS area
=========

.. contents::


Introduction
============

PBUS is present on all nvidia cards. In theory, it deals with "bus control".
In practice, it accumulates all sort of junk nobody bothered to create
a special area for. It is unaffected by any PMC.ENABLE bits.


.. _pbus-mmio:

MMIO registers
=========================

The main PBUS MMIO range is 0x1000:0x2000. It's present on all cards.
In addition to this range, PBUS also owns :ref:`PEEPHOLE <peephole-mmio>` and
:ref:`PBUS_HWSQ_NEW_CODE <pbus-hwsq-new-code-mmio>` ranges.

The registers in the PBUS area are:

.. space:: 8 pbus 0x1000 bus control
   0x084 DEBUG_1 pbus-debug-1 NV4:
   0x098 DEBUG_6 pbus-debug-6 NV17:NV20,NV25:
   0x100 INTR pbus-intr NV3:
   0x104 INTR_GPIO pbus-intr-gpio NV31:NV50
   0x140 INTR_ENABLE pbus-intr-enable NV3:
   0x144 INTR_GPIO_ENABLE pbus-intr-gpio-enable NV31:NV50
   0x144 INTR_ENABLE_NRHOST pbus-intr-enable-nrhost GF100:
   0x150 INTR_USER0_TRIGGER pbus-intr-user-trigger NV50:
   0x154[4] INTR_USER0_SCRATCH pbus-intr-user-scratch NV50:
   0x170 INTR_USER1_TRIGGER pbus-intr-user-trigger GF100:
   0x174[4] INTR_USER1_SCRATCH pbus-intr-user-scratch GF100:
   0x200 ROM_TIMINGS nv3-prom-rom-timings NV4:NV10
   0x200 ROM_TIMINGS nv10-prom-rom-timings NV10:NV50
   0x204 ROM_SPI_CTRL prom-spi-ctrl NV17:NV20,NV25:NV50
   0xa14 IBUS_TIMEOUT pbus-ibus-timeout NVA3:GF100

   .. todo:: connect

   ============= ========== ===============
   Range         Variants   Description
   ============= ========== ===============
   0010f0:0010f4 NV11:NV50  :ref:`PWM - PWM generators <pbus-mmio-pwm>`
   001300:001380 NV17:NV20  :ref:`HWSQ - hardware sequencer <hwsq-mmio>`
                 NV25:GF100
   001380:001400 NV41:NV50  :ref:`VGA_STACK <pbus-mmio-vga-stack>`
   001400:001500 NV17:NV20  :ref:`HWSQ - hardware sequencer <hwsq-mmio>`
                 NV25:GF100
   001500:001540 ???        :ref:`DEBUG registers <pbus-mmio-debug>`
   001540:001550 NV40:GF100 HWUNITS - enabling/disabling optional hardware subunits [see below]
   00155c:001578 NV30:NV84  :ref:`PEEPHOLE - indirect memory access <peephole-mmio>`
   001578:001580 NV41:GF100 :ref:`HWSQ - hardware sequencer <hwsq-mmio>`
   001580:0015a0 NV17:NV20  CLOCK_GATE - clock gating registers [see below]
                 NV25:GF100
   0015b0:0015c0 NV43:NV50  :ref:`THERM - thermal sensor <nv43-therm-mmio>`
   0015f4:001604 NV41:NV50  :ref:`PWM - PWM generators <pbus-mmio-pwm>`
   001700:001800 TC         :ref:`HOST_MEM - host memory access setup <pbus-mmio-nv44-host-mem>`
   001700:001800 NV50:GF100 :ref:`HOST_MEM - host memory access setup <pbus-mmio-nv50-host-mem>`
   001700:001800 GF100-     :ref:`HOST_MEM - host memory access setup <pbus-mmio-gf100-host-mem>`
   001800:001a00 NV1:NV50   :ref:`PCI - PCI configuration space <pbus-mmio-pci>`
   001900:001980 NV50:GF100 :ref:`REMAP - BAR1 remapping circuitry <pbus-mmio-nv50-remap>`
   001980:001a00 NV50:GF100 :ref:`P2P - NV50 P2P slave <pbus-mmio-nv50-p2p>`
   ============= ========== ===============

   .. todo:: loads and loads of unknown registers not shown


DEBUG registers
===============

DEBUG registers store misc hardware control bits. They're mostly unknown, and
usually group together unrelated bits. The known bits include:

.. reg:: 32 pbus-debug-1 misc stuff

   - bit 11: FUSE_READOUT_ENABLE - enables reads from fuses in :ref:`PFUSE <pfuse>` [NV50:GF100]
   - bit 28: HEADS_TIED - mirrors writes to :ref:`CRTC <pcrtc-mmio>`/:ref:`RAMDAC <pramdac-mmio>` registers on any head to
     the other head too [NV11:NV20, NV25:NV50]

.. reg:: 32 pbus-debug-6 misc stuff

   - bit 3: :ref:`HWSQ_ENABLE - enables HWSQ effects <hwsq-mmio>`
   - bit 4: :ref:`HWSQ_OVERRIDE_MODE - selects read value for HWSQ-overriden registers <hwsq-mmio>`

.. todo:: document other known stuff


.. _pbus-intr:

Interrupts
==========

.. todo:: cleanup

On NV3+, PMC interrupt line 28 is connected to PBUS. On GF100+, there are
actually two lines: the normal line and the NRHOST line [see :ref:`pmc-intr`
for a description of them]. PBUS has many subinterrupts. The PBUS->PMC interrupt
line is active when any PBUS interrupt is both active [the bit in INTR
or INTR_GPIO is 1] and enabled [the bit in INTR_EN or INTR_GPIO_EN is 1].
The NRHOST PBUS->PMC interrupt line is active when any PBUS interrupt is both
active and enabled for NRHOST [the bit in INTR_EN_NRHOST is 1].

Most PBUS interrupts are reported via INTR register and enabled via INTR_EN
and INTR_EN_NRHOST registers:

.. reg:: 32 pbus-intr interrupt status/acknowledge

   - bit 0: BUS_ERROR - ??? [NV3:NV50]
   - bit 1: MMIO_DISABLED_ENG - MMIO access from host failed due to accessing
     an area disabled via PMC.ENABLE [GF100-] [XXX: document]
   - bit 2: MMIO_RING_ERR - :ref:`MMIO access from host failed due to some error in
     PRING <pbus-intr-mmio-ring-err>` [GF100-]
   - bit 3: MMIO_FAULT - MMIO access from host failed due to other reasons
     [NV41-] [XXX: document]
   - bit 4: GPIO_0_RISE - :ref:`GPIO #0 went from 0 to 1 [NV10:NV31] <nv10-gpio-intr>`
   - bit 7: HOST_MEM_TIMEOUT - :ref:`an access to memory from host timed out [GF100-]
     <pbus-intr-host-mem-timeout>`
   - bit 8: GPIO_0_FALL - :ref:`GPIO #0 went from 1 to 0 [NV10:NV31] <nv10-gpio-intr>`
   - bit 8: HOST_MEM_ZOMBIE - :ref:`an access to memory from host thought to have timed
     out has finally succeeded [GF100-] <pbus-intr-host-mem-zombie>`
   - bit 12: PEEPHOLE_W_PAIR_MISMATCH - :ref:`violation of PEEPHOLE write port protocol
     [NV30:GF100] <pbus-intr-peephole-w-pair-mismatch>`
   - bit 16: THERM_ALARM - Temperature is critical and requires actions
     [NV43-] [:ref:`NV43 <nv43-therm-intr-alarm>`, :ref:`NV50 <ptherm-intr>`]
   - bit 17: THERM_THRS_LOW - Temperature is lower than TEMP_RANGE.LOW
     [NV43:NV50] [:ref:`NV43 <nv43-therm-intr-range>`]
   - bit 18: THERM_THRS_HIGH - Temperature is higher than TEMP_RANGE.HIGH
     [NV43:NV50] [:ref:`NV43 <nv43-therm-intr-range>`]
   - bit 26: USER0 - user interrupt #0 [NV50-] [see below]
   - bit 28: USER1 - user interrupt #1. Note that this interrupt cannot be
     enabled for delivery to NRHOST line. [GF100-] [see below]

Writing the INTR register clears interrupts that correspond to bits that
are set in the written value.

.. reg:: 32 pbus-intr-enable interrupt enable

   Same bitfields as in INTR.

.. reg:: 32 pbus-intr-enable-nrhost NRHOST interrupt enable

   Same bitfields as in INTR, except USER1 is not present.

On NV40:NV50 GPUs, the PBUS additionally deals with GPIO change interrupts,
which are reported via INTR_GPIO register and enabled via INTR_GPIO_EN
register. These registers effectively function as extra bits to INTR and
INTR_EN. For description of these registrers and GPIO interupts, see
:ref:`nv10-gpio-intr`.


User interrupts
---------------

NV50+ PBUS has one [NV50:GF100] or two [GF100-] user-triggerable interupts.
These interrupts are triggered by writing any value to a trigger register:

.. reg:: 32 intr-user-trigger user interrupt generation

   Writing any value triggers the USERx interrupt. This register is write-only.

There are also 4 scratch registers per interrupt provided for software use.
The hardware doesn't use their contents for anything:

.. reg:: 32 intr-user-scratch user interrupt scratch register

   32-bit scratch registers for USERx interrupt.


NVA3 IBUS timeout
=================

.. todo:: description, maybe move somewhere else

On NVA3:GF100, the IBUS timeout is controlled by:

.. reg:: 32 pbus-ibus-timeout IBUS timeout length

   Specifies how many host cycles to wait for response on MMIO accesses
   forwarded to the IBUS.

.. todo:: verify that it's host cycles

Reads that time out return a value of 0. Note that using too long timeout
value will result in PCIE master timeouts instead, with possibly quite bad
consequences. An IBUS timeout will cause the MMIO_FAULT interrupt to be lit.
