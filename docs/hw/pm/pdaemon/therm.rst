.. _pdaemon-therm:
.. _pdaemon-io-therm:
.. _pdaemon-intr-therm:
.. _pdaemon-perf-therm:

================
PTHERM interface
================

PDAEMON can access all PTHERM registers directly, without having to go through
the generic MMIO access functionality. The THERM range in the PDAEMON register
space is mapped straight to PTHERM MMIO register range.

On NVA3:NVD9, PTHERM registers are mapped into the I[] space at addresses
0x20000:0x40000, with addresses being shifted left by 6 bits wrt their address
in PTHERM - PTHERM register 0x20000+x would be visible at I[0x20000 + x * 0x40]
by falcon, or at 0x10a800+x in MMIO [assuming it wouldn't fall into the reserved
0x10afe0:0x10b000 range]. On NVD9+, the PTHERM registers are instead mapped
into the I[] space at addresses 0x1000:0x1800, without shifting - PTHERM reg
0x20000+x is visible at I[0x1000+x]. On NVD9+, the alias area is not visible
via MMIO [just access PTHERM registers directly instead].

Reads to the PTHERM-mapped area will always perform 32-bit reads to the
corresponding PTHERM regs. Writes, however, have their byte enable mask
controlled via a PDAEMON register, enabling writes with sizes other than
32-bit:

MMIO 0x5f4 / I[0x17d00]: THERM_BYTE_MASK
  Read/write, only low 4 bits are valid, initialised to 0xf on reset. Selects
  the byte mask to use when writing the THERM range. Bit i corresponds to
  bits i*8..i*8+7 of the written 32-bit value.

The PTHERM access circuitry also exports a signal to PCOUNTER:

- THERM_ACCESS_BUSY: 1 while a THERM range read/write is in progress - will
  light up for a dozen or so cycles per access, depending on relative clock
  speeds.

In addition to direct register access to PTHERM, PDAEMON also has direct access
to PTHERM interrupts - falcon interrupt #12 [THERM] comes from PTHERM interrupt
aggregator. PTHERM subinterrupts can be individually assigned for PMC or
PDAEMON delivery - see :ref:`ptherm-intr` for more information.
