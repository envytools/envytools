.. _pdaemon-subintr:
.. _pdaemon-intr-subintr:
.. _pdaemon-io-subintr:

=======================
Second-level interrupts
=======================

Because falcon has space for only 8 engine interrupts and PDAEMON needs many
more, a second-level interrupt register was introduced:

MMIO 0x688 / I[0x1a200]: SUBINTR
  - bit 0: H2D - :ref:`host to PDAEMON scratch register written <pdaemon-subintr-h2d>`
  - bit 1: FIFO - :ref:`host to PDAEMON fifo pointer updated <pdaemon-subintr-fifo>`
  - bit 2: EPWR_GRAPH - :ref:`PGRAPH engine power control <pdaemon-subintr-epwr>`
  - bit 3: EPWR_VDEC - :ref:`video decoding engine power control <pdaemon-subintr-epwr>`
  - bit 4: MMIO - :ref:`indirect MMIO access error <pdaemon-subintr-mmio>`
  - bit 5: IREDIR_ERR - :ref:`interrupt redirection error <pdaemon-subintr-iredir-err>`
  - bit 6: IREDIR_HOST_REQ - :ref:`interrupt redirection request <pdaemon-subintr-host-irq>`
  - bit 7: ???
  - bit 8: ??? - goes to 0x670
  - bit 9: EPWR_VCOMP [NVAF] - :ref:`PVCOMP engine power control <pdaemon-subintr-epwr>`
  - bit 13: ??? [NVD9-] - goes to 0x888

.. todo:: figure out bits 7, 8
.. todo:: more bits in 10-12?

The second-level interrupts are merged into a single level-triggered interrupt
and delivered to falcon interrupt line 11. This line is asserted whenever any bit
of SUBINTR register is non-0. A given SUBINTR bit is set to 1 whenever the
input second-level interrupt line is 1, but will not auto-clear when the input
line goes back to 0 - only writing 1 to that bit in SUBINTR will clear it.
This effectively means that SUBINTR bits have to be cleared after the
downstream interrupt. Note that SUBINTR has no corresponding enable bit - if
an interrupt needs to be disabled, software should use the enable registers
corresponding to individual second-level interrupts instead.

Note that IREDIR_HOST_REQ interrupt has special semantics when cleared - see
IREDIR_TRIGGER documentation.
