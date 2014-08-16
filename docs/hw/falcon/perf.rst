.. _falcon-perf:

==============================
Performance monitoring signals
==============================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


Main PCOUNTER signals
=====================

The main signals exported by falcon to PCOUNTER are:

.. todo:: docs & RE, please

- 0x00: SLEEPING
- 0x01: ??? fifo idle?
- 0x02: IDLE
- 0x03: ???
- 0x04: ???
- 0x05: TA
- 0x06: ???
- 0x07: ???
- 0x08: ???
- 0x09: ???
- 0x0a: ???
- 0x0b: ???
- 0x0c: PM_TRIGGER
- 0x0d: WRCACHE_FLUSH
- 0x0e-0x13: USER


.. _falcon-io-perf-user:

User signals
============

MMIO 0x088 / I[0x02200]: PM_TRIGGER
  A WO "trigger" register for various things. write 1 to a bit to trigger
  the relevant event, 0 to do nothing.

  - bits 0-5: ??? [perf counters?]
  - bit 16: WRCACHE_FLUSH
  - bit 17: ??? [PM_TRIGGER?]

MMIO 0x08c / I[0x02300]: PM_MODE
  bits 0-5: ??? [perf counters?]

.. todo:: write me
