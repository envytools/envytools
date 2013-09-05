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

============= ========= ===============
Range         Variants  Description
============= ========= ===============
001000:0010f0 NV04-     :ref:`DEBUG registers <pbus-mmio-debug>`
0010f0:0010f4 NV11:NV50 PWM - PWM generators [io/nv10-gpio.txt]
001100:001200 NV03-     :ref:`interrupts <pbus-mmio-intr>`
001200:001208 NV04:NV50 ROM control [io/prom.txt]
001300:001380 NV17:NV20 :ref:`HWSQ - hardware sequencer <hwsq-mmio>`
              NV25:NVC0
001380:001400 NV41:NV50 VGA_STACK [display/nv03/vga-stack.txt]
001400:001500 NV17:NV20 :ref:`HWSQ - hardware sequencer <hwsq-mmio>`
              NV25:NVC0
001500:001540 ???       :ref:`DEBUG registers <pbus-mmio-debug>`
001540:001550 NV40:NVC0 HWUNITS - enabling/disabling optional hardware subunits [see below]
00155c:001578 NV30:NV84 PEEPHOLE - indirect memory access [memory/peephole.txt]
001578:001580 NV41:NVC0 :ref:`HWSQ - hardware sequencer <hwsq-mmio>`
001580:0015a0 NV17:NV20 CLOCK_GATE - clock gating registers [see below]
              NV25:NVC0
0015b0:0015c0 NV43:NV50 THERM - thermal sensor [pm/nv43-therm.txt]
0015f4:001604 NV41:NV50 PWM - PWM generators [io/nv10-gpio.txt]
001700:001800 TC        HOST_MEM - host memory access setup [memory/nv44-host-mem.txt]
001700:001800 NV50:NVC0 HOST_MEM - host memory access setup [memory/nv50-host-mem.txt]
001700:001800 NVC0-     HOST_MEM - host memory access setup [memory/nvc0-host-mem.txt]
001800:001a00 NV01:NV50 PCI - PCI configuration space [bus/pci.txt]
001900:001980 NV50:NVC0 REMAP - BAR1 remapping circuitry [memory/nv50-remap.txt]
001980:001a00 NV50:NVC0 P2P - NV50 P2P slave [memory/nv50-p2p.txt]
001a14        NVA3:NVC0 :ref:`IBUS_TIMEOUT - controls timeout length for accessing MMIO via IBUS <pbus-mmio-ibus-timeout>`
============= ========= ===============

.. todo:: loads and loads of unknown registers not shown


.. _pbus-mmio-debug:

DEBUG registers
===============

DEBUG registers store misc hardware control bits. They're mostly unknown, and
usually group together unrelated bits. The known bits include:

MMIO 0x001084: DEBUG_1 [NV04-]
  bit 11: FUSE_READOUT_ENABLE - enables reads from fuses in PFUSE [NV50:NVC0]
          [bus/pfuse.txt]
  bit 28: HEADS_TIED - mirrors writes to CRTC/RAMDAC registers on any head to
          the other head too [NV11:NV20, NV25:NV50] [display/nv03/vga.txt]

MMIO 0x001098: DEBUG_6 [NV17:NV20, NV25-]
  bit 3: :ref:`HWSQ_ENABLE - enables HWSQ effects <hwsq-mmio>`
  bit 4: :ref:`HWSQ_OVERRIDE_MODE - selects read value for HWSQ-overriden registers <hwsq-mmio>`

.. todo:: document other known stuff


.. _pbus-intr:
.. _pbus-mmio-intr:

Interrupts
==========

The following registers deal with PBUS interrupts:

- 001100 INTR - interrupt status [NV03-]
- 001104 INTR_GPIO - GPIO interrupt status [NV31:NV50] [io/nv10-gpio.txt]
- 001140 INTR - interrupt enable [NV03-]
- 001144 INTR_GPIO_EN - GPIO interrupt enable [NV31:NV50] [io/nv10-gpio.txt]
- 001144 INTE_EN_NRHOST - interrupt enable for the NRHOST line [NVC0-]
- 001150 INTR_USER0_TRIGGER - user interrupt generation [NV50-]
- 001154+i*4, i<4 INTR_USER0_SCRATCH - user interrupt generation [NV50-]
- 001170 INTR_USER1_TRIGGER - user interrupt generation [NVC0-]
- 001174+i*4, i<4 INTR_USER1_SCRATCH - user interrupt generation [NVC0-]

.. todo:: cleanup

On NV03+, PMC interrupt line 28 is connected to PBUS. On NVC0+, there are
actually two lines: the normal line and the NRHOST line [see :ref:`pmc-intr`
for a description of them]. PBUS has many subinterrupts. The PBUS->PMC interrupt
line is active when any PBUS interrupt is both active [the bit in INTR
or INTR_GPIO is 1] and enabled [the bit in INTR_EN or INTR_GPIO_EN is 1].
The NRHOST PBUS->PMC interrupt line is active when any PBUS interrupt is both
active and enabled for NRHOST [the bit in INTR_EN_NRHOST is 1].

Most PBUS interrupts are reported via INTR register and enabled via INTR_EN
and INTR_EN_NRHOST registers:

MMIO 0x001100: INTR [NV03-]
  - bit 0: BUS_ERROR - ??? [NV03:NV50]
  - bit 1: MMIO_DISABLED_ENG - MMIO access from host failed due to accessing
    an area disabled via PMC.ENABLE [NVC0-] [XXX: document]
  - bit 2: MMIO_IBUS_ERR - MMIO access from host failed due to some error in
    IBUS [NVC0-] [see bus/pibus.txt]
  - bit 3: MMIO_FAULT - MMIO access from host failed due to other reasons
    [NV41-] [XXX: document]
  - bit 4: GPIO_0_RISE - GPIO #0 went from 0 to 1 [NV10:NV31] [io/nv10-gpio.txt]
  - bit 7: HOST_MEM_TIMEOUT - an access to memory from host timed out [NVC0-]
    [see memory/nvc0-host-mem.txt]
  - bit 8: GPIO_0_FALL - GPIO #0 went from 1 to 0 [NV10:NV31] [io/nv10-gpio.txt]
  - bit 8: HOST_MEM_ZOMBIE - an access to memory from host thought to have timed
    out has finally succeeded [NVC0-] [see memory/nvc0-host-mem.txt]
  - bit 12: PEEPHOLE_W_PAIR_MISMATCH - violation of PEEPHOLE write port protocol
    [NV30:NVC0] [see memory/peephole.txt]
  - bit 16: THERM_ALARM - Temperature is critical and requires actions
    [NV43-] [pm/nv43-therm.txt, pm/ptherm.txt]
  - bit 17: THERM_THRS_LOW - Temperature is lower than TEMP_RANGE.LOW
    [NV43:NV50] [pm/nv43-therm.txt]
  - bit 18: THERM_THRS_HIGH - Temperature is higher than TEMP_RANGE.HIGH
    [NV43:NV50] [pm/nv43-therm.txt]
  - bit 26: USER0 - user interrupt #0 [NV50-] [see below]
  - bit 28: USER1 - user interrupt #1. Note that this interrupt cannot be
    enabled for delivery to NRHOST line. [NVC0-] [see below]

Writing the INTR register clears interrupts that correspond to bits that
are set in the written value.

MMIO 0x001140: INTR_EN [NV03-]
  Same bitfields as in INTR.

MMIO 0x001144: INTR_EN_NRHOST [NVC0-]
  Same bitfields as in INTR, except USER1 is not present.

On NV40:NV50 GPUs, the PBUS additionally deals with GPIO change interrupts,
which are reported via INTR_GPIO register and enabled via INTR_GPIO_EN
register. These registers effectively function as extra bits to INTR and
INTR_EN. For description of these registrers and GPIO interupts, see
io/nv10-gpio.txt .


User interrupts
---------------

NV50+ PBUS has one [NV50:NVC0] or two [NVC0-] user-triggerable interupts.
These interrupts are triggered by writing any value to a trigger register:

MMIO 0x001150: INTR_USER0_TRIGGER [NV50-]
  Writing any value triggers the USER0 interrupt. This register is write-only.

MMIO 0x001170: INTR_USER1_TRIGGER [NVC0-]
  Writing any value triggers the USER1 interrupt. This register is write-only.

There are also 4 scratch registers per interrupt provided for software use.
The hardware doesn't use their contents for anything:

MMIO 0x001154+i*4, i < 4: INTR_USER0_SCRATCH[i] [NV50-]
  32-bit scratch registers for USER0 interrupt.

MMIO 0x001174+i*4, i < 4: INTR_USER1_SCRATCH[i] [NVC0-]
  32-bit scratch registers for USER1 interrupt.


.. _pbus-mmio-ibus-timeout:

NVA3 IBUS timeout
=================

.. todo:: description, maybe move somewhere else

On NVA3:NVC0, the IBUS timeout is controlled by:

MMIO 0x001a14: IBUS_TIMEOUT [NVA3:NVC0]
  Specifies how many host cycles to wait for response on MMIO accesses
  forwarded to the IBUS.

.. todo:: verify that it's host cycles

Reads that time out return a value of 0. Note that using too long timeout
value will result in PCIE master timeouts instead, with possibly quite bad
consequences. An IBUS timeout will cause the MMIO_FAULT interrupt to be lit.
