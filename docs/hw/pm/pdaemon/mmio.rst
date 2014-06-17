.. _pdaemon-mmio:
.. _pdaemon-subintr-mmio:

============================
General MMIO register access
============================

PDAEMON can access the whole MMIO range through the IO space.

To read from a MMIO address, poke the address into MMIO_ADDR then trigger a read
by poking 0x100f1 to MMIO_CTRL. Wait for MMIO_CTRL's bits 12-14 to be cleared
then read the value from MMIO_VALUE.

To write to a MMIO address, poke the address into MMIO_ADDR, poke the value
to be written into MMIO_VALUE then trigger a write by poking 0x100f2 to
MMIO_CTRL. Wait for MMIO_CTRL's bits 12-14 to be cleared if you want to
make sure the write has been completed.

Accessing an unexisting address will set MMIO_CTRL's bit 13 after MMIO_TIMEOUT
cycles have passed.

NVD9 introduced the possibility to choose from which access point should
the MMIO request be sent. ROOT can access everything, IBUS accesses everything
minus PMC, PBUS, PFIFO, PPCI and a few other top-level MMIO range.
On nvd9+, accessing an un-existing address with the ROOT access point can lead
to a hard-lock.
XXX: What's the point of this feature?

It is possible to get an interrupt when an error occurs by poking 1 to
MMIO_INTR_EN. The interrupt will be fired on line 11. The error is described
in MMIO_ERR.

MMIO 0x7a0 / I[0x1e800]: MMIO_ADDR
  Specifies the MMIO address that will be written to/read from by MMIO_CTRL.

  On nva3-d9, this register only contains the address to be accessed.

  On nvd9, this register became a bitfield:
  bits 0-25: ADDR
  bit 27: ACCESS_POINT
    0: ROOT
    1: IBUS

MMIO 0x7a4 / I[0x1e900]: MMIO_VALUE
  The value that will be written to / is read from MMIO_ADDR when an operation
  is triggered by MMIO_CTRL.

MMIO 0x7a8 / I[0x1e900]: MMIO_TIMEOUT
  Specifies the timeout for MMIO access.
  XXX: Clock source? PDAEMON's core clock, PTIMER's, Host's?

MMIO 0x7ac / I[0x1eb00]: MMIO_CTRL
  Process the MMIO request with given params (MMIO_ADDR, MMIO_VALUE).
  bits 0-1: request
    0: XXX
    1: read
    2: write
    3: XXX
  bits 4-7: BYTE_MASK
  bit 12: BUSY [RO]
  bit 13: TIMEOUT [RO]
  bit 14: FAULT [RO]
  bit 16: TRIGGER

MMIO 0x7b0 / I[0x1ec00] : MMIO_ERR
  Specifies the MMIO error status:
    - TIMEOUT: ROOT/IBUS has not answered PDAEMON's request
    - CMD_WHILE_BUSY: a request has been fired while being busy
    - WRITE: set if the request was a write, cleared if it was a read
    - FAULT: No engine answered ROOT/IBUS's request
  On nva3-d9, clearing MMIO_INTR's bit 0 will also clear MMIO_ERR.
  On nvd9+, clearing MMIO_ERR is done by poking 0xffffffff.

  Nva3-c0:
  bit 0: TIMEOUT
  bit 1: CMD_WHILE_BUSY
  bit 2: WRITE
  bits 3-31: ADDR

  Nvc0-d9:
  bit 0: TIMEOUT
  bit 1: CMD_WHILE_BUSY
  bit 2: WRITE
  bits 3-30: ADDR
  bit 31: FAULT

  Nvd9+:
  bit 0: TIMEOUT_ROOT
  bit 1: TIMEOUT_IBUS
  bit 2: CMD_WHILE_BUSY
  bit 3: WRITE
  bits 4-29: ADDR
  bit 30: FAULT_ROOT
  bit 31: FAULT_IBUS

MMIO 0x7b4 / I[0x1ed00] : MMIO_INTR
  Specifies which MMIO interrupts are active. Clear the associated bit to ACK.
  bit 0: ERR
    Clearing this bit will also clear MMIO_ERR on nva3-d9.

MMIO 0x7b8 / I[0x1ee00] : MMIO_INTR_EN
  Specifies which MMIO interrupts are enabled. Interrupts will be fired on
  SUBINTR #4.
  bit 0: ERR
