.. _falcon-perf:

==============================
Performance monitoring signals
==============================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


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
