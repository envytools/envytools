.. _pdaemon-host:

==============================
Host <-> PDAEMON communication
==============================

.. contents::


Introduction
============

There are 4 PDAEMON-specific channels that can be used for communication
between the host and PDAEMON:

- FIFO: data submission from host to PDAEMON on 4 independent FIFOs in data
  segment, with interrupts generated whenever the PUT register is written
- RFIFO: data submission from PDAEMON to host on through a FIFO in data
  segment
- H2D: a single scratch register for host -> PDAEMON communication, with
  interrupts generated whenever it's written
- D2H: a single scratch register for PDAEMON -> host communication
- DSCRATCH: 4 scratch registers


.. _pdaemon-fifo:
.. _pdaemon-subintr-fifo:
.. _pdaemon-perf-fifo:
.. _pdaemon-io-fifo:
.. _pdaemon-io-fifo-intr:

Submitting data to PDAEMON: FIFO
================================

These registers are meant to be used for submitting data from host to PDAEMON.
The PUT register is FIFO head, written by host, and GET register is FIFO tail,
written by PDAEMON. Interrupts can be generated whenever the PUT register is
written. How exactly the data buffer works is software's business. Note that
due to very limitted special semantics for FIFO uage, these registers may as
well be used as [possibly interruptful] scratch registers.

MMIO 0x4a0+i*4 / I[0x12800+i*0x100], i<4: FIFO_PUT[i]
  The FIFO head pointer, effectively a 32-bit scratch register. Writing it
  causes bit i of FIFO_INTR to be set.

MMIO 0x4b0+i*4 / I[0x12c00+i*0x100], i<4: FIFO_GET[i]
  The FIFO tail pointer, effectively a 32-bit scratch register.

MMIO 0x4c0 / I[0x13000]: FIFO_INTR
  The status register for FIFO_PUT write interrupts. Write a bit with 1 to
  clear it. Whenever a bit is set both in FIFO_INTR and FIFO_INTR_EN, the FIFO
  [#1] second-level interrupt line to SUBINTR is asserted. Bit i corresponds to
  FIFO #i, and only bits 0-3 are valid.

MMIO 0x4c4 / I[0x13100]: FIFO_INTR_EN
  The enable register for FIFO_PUT write interrupts. Read/write, only 4 low
  bits are valid. Bit assignment is the same as in FIFO_INTR.

In addition, the FIFO circuitry exports four signals to PCOUNTER:

- FIFO_PUT_0_WRITE: pulses for one cycle whenever FIFO_PUT[0] is written
- FIFO_PUT_1_WRITE: pulses for one cycle whenever FIFO_PUT[1] is written
- FIFO_PUT_2_WRITE: pulses for one cycle whenever FIFO_PUT[2] is written
- FIFO_PUT_3_WRITE: pulses for one cycle whenever FIFO_PUT[3] is written


.. _pdaemon-rfifo:
.. _pdaemon-io-rfifo:

Submitting data to host: RFIFO
==============================

The RFIFO is like one of the 4 FIFOs, except it's supposed to go from PDAEMON
to the host and doesn't have the interupt generation powers.

MMIO 0x4c8 / I[0x13200]: RFIFO_PUT
MMIO 0x4cc / I[0x13300]: RFIFO_GET
  The RFIFO head and tail pointers. Both are effectively 32-bit scratch
  registers.


.. _pdaemon-h2d:
.. _pdaemon-subintr-h2d:
.. _pdaemon-io-h2d:
.. _pdaemon-io-h2d-intr:

Host to PDAEMON scratch register: H2D
=====================================

H2D is a scratch register supposed to be written by the host and read by
PDAEMON. It generates an interrupt when written.

MMIO 0x4d0 / I[0x13400]: H2D
  A 32-bit scratch register. Sets H2D_INTR when written.

MMIO 0x4d4 / I[0x13500]: H2D_INTR
  The status register for H2D write interrupt. Only bit 0 is valid. Set when
  H2D register is written, cleared when 1 is written to bit 0. When this and
  H2D_INTR_EN are both set, the H2D [#0] second-level interrupt line to
  SUBINTR is asserted.

MMIO 0x4d8 / I[0x13600]: H2D_INTR_EN
  The enable register for H2D write interrupt. Only bit 0 is valid.


.. _pdaemon-d2h:
.. _pdaemon-io-d2h:

PDAEMON to host scratch register: D2H
=====================================

D2H is just a scratch register supposed to be written by PDAEMON and read by
the host. It has no interrupt genration powers.

MMIO 0x4dc / I[0x13700]: D2H
  A 32-bit scratch register.


.. _pdaemon-dscratch:
.. _pdaemon-io-dscratch:

Scratch registers: DSCRATCH
===========================

DSCRATCH[] are just 4 32-bit scratch registers usable for PDAEMON<->HOST
communication or any other purposes.

MMIO 0x5d0+i*4 / I[0x17400+i*0x100], i<4: DSCRATCH[i]
  A 32-bit scratch register.


